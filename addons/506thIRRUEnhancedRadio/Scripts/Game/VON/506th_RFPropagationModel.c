class IRRU_RFPropagationModel
{
    protected static const float DEFAULT_FREQUENCY_KHZ = 60000.0;
    protected static const float SPEED_OF_LIGHT = 299792458.0;
    protected static const float ANTENNA_HEIGHT = 1.5;

    protected static const float SAMPLE_INTERVAL = 50.0;
    protected static const int MAX_SAMPLES = 200;
    protected static const float MIN_DISTANCE = 50.0;
    protected static const int MAX_KNIFE_EDGES = 3;

    //! 4/3-earth model: 6371 km mean radius x 1.333 atmospheric K-factor
    protected static const float EFFECTIVE_EARTH_RADIUS = 8492543.0;

    protected static const float EXCELLENT_THRESHOLD = 75.0;
    protected static const float GOOD_THRESHOLD = 85.0;
    protected static const float POOR_THRESHOLD = 92.0;
    protected static const float CUTOFF_THRESHOLD = 100.0;

    private static ref IRRU_RFPropagationModel s_Instance;

    //------------------------------------------------------------------------------------------------
    static IRRU_RFPropagationModel GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_RFPropagationModel();

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    float CalculateSignalQuality(vector txPos, vector rxPos, float frequencyKHz = 0)
    {
        BaseWorld world = GetGame().GetWorld();
        if (!world)
            return 1.0;

        if (frequencyKHz <= 0)
            frequencyKHz = DEFAULT_FREQUENCY_KHZ;

        float frequencyHz = frequencyKHz * 1000.0;
        float wavelength = SPEED_OF_LIGHT / frequencyHz;
        float frequencyMHz = frequencyKHz / 1000.0;

        float dx = rxPos[0] - txPos[0];
        float dz = rxPos[2] - txPos[2];
        float horizontalDist = Math.Sqrt(dx * dx + dz * dz);

        if (horizontalDist < MIN_DISTANCE)
            return 1.0;

        float txTerrainY = world.GetSurfaceY(txPos[0], txPos[2]);
        float rxTerrainY = world.GetSurfaceY(rxPos[0], rxPos[2]);
        float txHeight = txTerrainY + ANTENNA_HEIGHT;
        float rxHeight = rxTerrainY + ANTENNA_HEIGHT;

        float dy = rxHeight - txHeight;
        float totalDistance = Math.Sqrt(horizontalDist * horizontalDist + dy * dy);

        float diffractionLoss = CalculateTerrainDiffractionLoss(world, txPos, rxPos, txHeight, rxHeight, horizontalDist, wavelength);
        float pathLoss = CalculatePathLoss(totalDistance, frequencyMHz);

        float totalLoss = pathLoss + diffractionLoss;
        float quality = LossToQuality(totalLoss);

        if (IRRU_RFPropagationNetworkComponent.IsDebugEnabled())
        {
            Print(string.Format("[RFPropagation] Dist: %1m | PathLoss: %2dB | Diffraction: %3dB | Total: %4dB | Quality: %5",
                Math.Round(totalDistance), Math.Round(pathLoss), Math.Round(diffractionLoss), Math.Round(totalLoss), quality));
        }

        return quality;
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculateTerrainDiffractionLoss(BaseWorld world, vector txPos, vector rxPos, float txHeight, float rxHeight, float horizontalDist, float wavelength)
    {
        int numSamples = Math.Floor(horizontalDist / SAMPLE_INTERVAL);
        if (numSamples < 1)
            return 0.0;
        if (numSamples > MAX_SAMPLES)
            numSamples = MAX_SAMPLES;

        float dx = rxPos[0] - txPos[0];
        float dz = rxPos[2] - txPos[2];
        float dirX = dx / horizontalDist;
        float dirZ = dz / horizontalDist;

        ref array<float> peakHeights = new array<float>();
        ref array<float> peakD1 = new array<float>();
        ref array<float> peakD2 = new array<float>();

        float worstFresnelIntrusion = 0.0;
        float worstFresnelRadius = 0.0;

        for (int i = 1; i < numSamples; i++)
        {
            float d1 = i * SAMPLE_INTERVAL;
            float d2 = horizontalDist - d1;

            float sampleX = txPos[0] + dirX * d1;
            float sampleZ = txPos[2] + dirZ * d1;

            float terrainY = world.GetSurfaceY(sampleX, sampleZ);
            float earthBulge = CalculateEarthBulge(d1, d2);
            float effectiveTerrainY = terrainY + earthBulge;
            float losHeight = txHeight + (rxHeight - txHeight) * (d1 / horizontalDist);
            float fresnelRadius = CalculateFresnelRadius(d1, d2, horizontalDist, wavelength);
            float heightAboveLOS = effectiveTerrainY - losHeight;

            if (heightAboveLOS > 0)
            {
                InsertPeakSorted(peakHeights, peakD1, peakD2, heightAboveLOS, d1, d2);
            }
            else
            {
                float fresnelClearance = -heightAboveLOS;
                float fresnelIntrusion = fresnelRadius - fresnelClearance;
                if (fresnelIntrusion > worstFresnelIntrusion)
                {
                    worstFresnelIntrusion = fresnelIntrusion;
                    worstFresnelRadius = fresnelRadius;
                }
            }
        }

        float totalLoss = 0.0;

        int numPeaks = peakHeights.Count();
        if (numPeaks > MAX_KNIFE_EDGES)
            numPeaks = MAX_KNIFE_EDGES;

        for (int p = 0; p < numPeaks; p++)
        {
            float h = peakHeights.Get(p);
            float d1 = peakD1.Get(p);
            float d2 = peakD2.Get(p);
            float diffractionLoss = CalculateDiffractionLoss(h, d1, d2, horizontalDist, wavelength);
            totalLoss += diffractionLoss;
        }

        if (worstFresnelIntrusion > 0 && worstFresnelRadius > 0)
        {
            float clearancePercent = 1.0 - (worstFresnelIntrusion / worstFresnelRadius);

            float fresnelLoss = 0.0;
            if (clearancePercent < 0.2)
                fresnelLoss = 3.0;
            else if (clearancePercent < 0.4)
                fresnelLoss = 2.0;
            else if (clearancePercent < 0.6)
                fresnelLoss = 1.0;

            totalLoss += fresnelLoss;
        }

        return totalLoss;
    }

    //------------------------------------------------------------------------------------------------
    protected void InsertPeakSorted(array<float> heights, array<float> d1Arr, array<float> d2Arr, float h, float d1, float d2)
    {
        int insertIdx = heights.Count();
        for (int i = 0; i < heights.Count(); i++)
        {
            if (h > heights.Get(i))
            {
                insertIdx = i;
                break;
            }
        }

        heights.InsertAt(h, insertIdx);
        d1Arr.InsertAt(d1, insertIdx);
        d2Arr.InsertAt(d2, insertIdx);

        while (heights.Count() > MAX_KNIFE_EDGES)
        {
            int lastIdx = heights.Count() - 1;
            heights.Remove(lastIdx);
            d1Arr.Remove(lastIdx);
            d2Arr.Remove(lastIdx);
        }
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculateFresnelRadius(float d1, float d2, float totalDist, float wavelength)
    {
        if (totalDist <= 0)
            return 0.0;

        return Math.Sqrt(wavelength * d1 * d2 / totalDist);
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculateEarthBulge(float d1, float d2)
    {
        return (d1 * d2) / (2.0 * EFFECTIVE_EARTH_RADIUS);
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculateDiffractionLoss(float h, float d1, float d2, float totalDist, float wavelength)
    {
        if (d1 <= 0 || d2 <= 0)
            return 0.0;

        float v = h * Math.Sqrt(2.0 * totalDist / (wavelength * d1 * d2));

        if (v < -0.78)
            return 0.0;

        float vMinusPoint1 = v - 0.1;
        float sqrtTerm = Math.Sqrt(vMinusPoint1 * vMinusPoint1 + 1.0);
        float loss = 6.9 + 20.0 * Math.Log10(sqrtTerm + vMinusPoint1);

        if (loss < 0)
            loss = 0;
        if (loss > 40)
            loss = 40;

        return loss;
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculatePathLoss(float distance, float frequencyMHz)
    {
        if (distance <= 1.0)
            return 0.0;

        float loss = 20.0 * Math.Log10(distance) + 20.0 * Math.Log10(frequencyMHz) - 27.55;

        if (loss < 0)
            loss = 0;

        return loss;
    }

    //------------------------------------------------------------------------------------------------
    protected float LossToQuality(float lossdB)
    {
        if (lossdB <= EXCELLENT_THRESHOLD)
            return 1.0;

        if (lossdB <= GOOD_THRESHOLD)
        {
            float t = (lossdB - EXCELLENT_THRESHOLD) / (GOOD_THRESHOLD - EXCELLENT_THRESHOLD);
            return 1.0 - t * 0.5;
        }

        if (lossdB <= POOR_THRESHOLD)
        {
            float t = (lossdB - GOOD_THRESHOLD) / (POOR_THRESHOLD - GOOD_THRESHOLD);
            return 0.5 - t * 0.3;
        }

        if (lossdB <= CUTOFF_THRESHOLD)
        {
            float t = (lossdB - POOR_THRESHOLD) / (CUTOFF_THRESHOLD - POOR_THRESHOLD);
            return 0.2 - t * 0.2;
        }

        return 0.0;
    }
}
