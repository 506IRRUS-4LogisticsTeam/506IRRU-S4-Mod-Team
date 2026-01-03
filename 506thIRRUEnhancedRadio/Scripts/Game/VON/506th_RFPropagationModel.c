class IRRU_RFPropagationModel
{
    protected static const float DEFAULT_FREQUENCY_KHZ = 60000.0;
    protected static const float SPEED_OF_LIGHT = 299792458.0;
    protected static const float ANTENNA_HEIGHT = 1.5;

    protected static const float SAMPLE_INTERVAL = 50.0;
    protected static const int MAX_SAMPLES = 200;
    protected static const float MIN_DISTANCE = 50.0;

    protected static const float EXCELLENT_THRESHOLD = 80.0;
    protected static const float GOOD_THRESHOLD = 100.0;
    protected static const float POOR_THRESHOLD = 120.0;
    protected static const float CUTOFF_THRESHOLD = 140.0;

    protected static const int MAX_OBSTACLE_ITERATIONS = 10;
    protected static const float RAYCAST_STEP_OFFSET = 0.5;

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

        float worstDiffractionLoss = CalculateTerrainDiffractionLoss(world, txPos, rxPos, txHeight, rxHeight, horizontalDist, wavelength);
        float freeSpaceLoss = CalculateFreeSpacePathLoss(totalDistance, frequencyMHz);
        float obstacleLoss = CalculateObstacleLoss(world, txPos, rxPos);

        float totalLoss = freeSpaceLoss + worstDiffractionLoss + obstacleLoss;
        float quality = LossToQuality(totalLoss);

        if (IRRU_RFPropagationNetworkComponent.IsDebugEnabled())
        {
            Print(string.Format("[RFPropagation] Dist: %1m | FSPL: %2dB | Diffraction: %3dB | Obstacle: %4dB | Total: %5dB | Quality: %6",
                Math.Round(totalDistance), Math.Round(freeSpaceLoss), Math.Round(worstDiffractionLoss), Math.Round(obstacleLoss), Math.Round(totalLoss), quality));
        }

        return quality;
    }

    //------------------------------------------------------------------------------------------------
    // too much math
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

        float worstObstruction = 0.0;
        float worstD1 = 0.0;
        float worstD2 = 0.0;
        float worstFresnelIntrusion = 0.0;
        float worstFresnelRadius = 0.0;

        for (int i = 1; i < numSamples; i++)
        {
            float d1 = i * SAMPLE_INTERVAL;
            float d2 = horizontalDist - d1;

            float sampleX = txPos[0] + dirX * d1;
            float sampleZ = txPos[2] + dirZ * d1;

            float terrainY = world.GetSurfaceY(sampleX, sampleZ);
            float losHeight = txHeight + (rxHeight - txHeight) * (d1 / horizontalDist);
            float fresnelRadius = CalculateFresnelRadius(d1, d2, horizontalDist, wavelength);
            float heightAboveLOS = terrainY - losHeight;

            if (heightAboveLOS > worstObstruction)
            {
                worstObstruction = heightAboveLOS;
                worstD1 = d1;
                worstD2 = d2;
            }

            if (heightAboveLOS <= 0)
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

        if (worstObstruction > 0)
        {
            float diffractionLoss = CalculateDiffractionLoss(worstObstruction, worstD1, worstD2, horizontalDist, wavelength);
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
    protected float CalculateFresnelRadius(float d1, float d2, float totalDist, float wavelength)
    {
        if (totalDist <= 0)
            return 0.0;

        float radius = Math.Sqrt(wavelength * d1 * d2 / totalDist);
        return radius;
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
    protected float CalculateFreeSpacePathLoss(float distance, float frequencyMHz)
    {
        if (distance <= 1.0)
            return 0.0;

        float loss = 20.0 * Math.Log10(distance) + 20.0 * Math.Log10(frequencyMHz) - 27.55; // FSPL formula in dB
        // constant is 27.55 when distance in meters and frequency in MHz

        if (loss < 0)
            loss = 0;

        return loss;
    }

    //------------------------------------------------------------------------------------------------
    protected float CalculateObstacleLoss(BaseWorld world, vector txPos, vector rxPos)
    {
        vector startPos = txPos + Vector(0, ANTENNA_HEIGHT, 0);
        vector endPos = rxPos + Vector(0, ANTENNA_HEIGHT, 0);

        vector dir = (endPos - startPos).Normalized();
        vector currentStart = startPos + dir * 1.0;

        float totalLoss = 0.0;

        for (int iteration = 0; iteration < MAX_OBSTACLE_ITERATIONS; iteration++)
        {
            float remainingDist = vector.Distance(currentStart, endPos);
            if (remainingDist < RAYCAST_STEP_OFFSET)
                break;

            TraceParam trace = new TraceParam();
            trace.Start = currentStart;
            trace.End = endPos;
            trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            trace.LayerMask = EPhysicsLayerPresets.Projectile;

            float result = world.TraceMove(trace, null);

            if (result >= 1.0)
                break;

            vector hitPos = currentStart + (endPos - currentStart) * result;

            float hitLoss = 0.0;

            if (trace.TraceMaterial && trace.TraceMaterial.Length() > 0)
                hitLoss += GetMaterialLoss(trace.TraceMaterial);

            if (trace.TraceEnt)
                hitLoss += GetEntityLoss(trace.TraceEnt);

            if (hitLoss <= 0)
                hitLoss = 10.0;

            totalLoss += hitLoss;

            currentStart = hitPos + dir * RAYCAST_STEP_OFFSET;
        }

        return totalLoss;
    }

    //------------------------------------------------------------------------------------------------
    protected float GetMaterialLoss(string material)
    {
		// there must be a better way to do this....
        if (!material || material.Length() == 0)
            return 0.0;

        string mat = material;
        mat.ToLower();

        if (mat.Contains("concrete") || mat.Contains("stone") || mat.Contains("rock"))
            return 15.0;
        if (mat.Contains("metal") || mat.Contains("steel") || mat.Contains("iron"))
            return 25.0;
        if (mat.Contains("brick"))
            return 12.0;

        if (mat.Contains("wood") || mat.Contains("plank"))
            return 5.0;
        if (mat.Contains("glass"))
            return 2.0;

        if (mat.Contains("grass") || mat.Contains("dirt") || mat.Contains("sand") || mat.Contains("soil"))
            return 0.0;
        if (mat.Contains("gravel") || mat.Contains("asphalt") || mat.Contains("road"))
            return 0.0;

        Print(string.Format("[RF Propagation] Unknown material: %1", material), LogLevel.DEBUG);
        return 3.0;
    }

    //------------------------------------------------------------------------------------------------
    protected float GetEntityLoss(IEntity ent)
    {
        if (!ent)
            return 0.0;

        if (ent.FindComponent(SlotManagerComponent))
            return 15.0;

        string typeName = ent.Type().ToString();
        typeName.ToLower();

        if (typeName.Contains("tree"))
            return 3.0;
        if (typeName.Contains("bush") || typeName.Contains("shrub"))
            return 1.0;

        if (typeName.Contains("building") || typeName.Contains("house") || typeName.Contains("wall"))
            return 15.0;

        if (typeName.Contains("vehicle") || typeName.Contains("car") || typeName.Contains("truck"))
            return 20.0;

        return 5.0;
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

    //------------------------------------------------------------------------------------------------
    static void DebugPropagation()
    {
        Print("------------------- IRRU RF PROPAGATION DEBUG -------------------", LogLevel.NORMAL);

        PlayerController pc = GetGame().GetPlayerController();
        if (!pc)
        {
            Print("[RF Debug] ERROR: No PlayerController found", LogLevel.ERROR);
            return;
        }

        IEntity controlled = pc.GetControlledEntity();
        if (!controlled)
        {
            Print("[RF Debug] ERROR: No controlled entity", LogLevel.ERROR);
            return;
        }

        BaseWorld world = GetGame().GetWorld();
        if (!world)
        {
            Print("[RF Debug] ERROR: No world", LogLevel.ERROR);
            return;
        }

        vector playerPos = controlled.GetOrigin();
        Print(string.Format("[RF Debug] Player position: %1", playerPos), LogLevel.NORMAL);

        float testDistance = 1000.0;
        vector forward = controlled.GetTransformAxis(2);
        vector targetPos = playerPos + forward * testDistance;

        Print(string.Format("[RF Debug] Test target: %1 (%2 m forward)", targetPos, testDistance), LogLevel.NORMAL);

        float playerTerrainY = world.GetSurfaceY(playerPos[0], playerPos[2]);
        float targetTerrainY = world.GetSurfaceY(targetPos[0], targetPos[2]);
        float playerHeight = playerTerrainY + ANTENNA_HEIGHT;
        float targetHeight = targetTerrainY + ANTENNA_HEIGHT;

        Print(string.Format("[RF Debug] Player terrain: %1 m, antenna: %2 m", playerTerrainY.ToString(1), playerHeight.ToString(1)), LogLevel.NORMAL);
        Print(string.Format("[RF Debug] Target terrain: %1 m, antenna: %2 m", targetTerrainY.ToString(1), targetHeight.ToString(1)), LogLevel.NORMAL);

        float dx = targetPos[0] - playerPos[0];
        float dz = targetPos[2] - playerPos[2];
        float horizontalDist = Math.Sqrt(dx * dx + dz * dz);

        int numSamples = Math.Floor(horizontalDist / SAMPLE_INTERVAL);
        if (numSamples > MAX_SAMPLES)
            numSamples = MAX_SAMPLES;

        float debugFreqKHz = DEFAULT_FREQUENCY_KHZ;
        float debugFreqHz = debugFreqKHz * 1000.0;
        float wavelength = SPEED_OF_LIGHT / debugFreqHz;
        float debugFreqMHz = debugFreqKHz / 1000.0;

        Print(string.Format("[RF Debug] Frequency: %1 MHz, Wavelength: %2 m", debugFreqMHz, wavelength), LogLevel.NORMAL);
        Print(string.Format("[RF Debug] Horizontal distance: %1 m, samples: %2", horizontalDist, numSamples), LogLevel.NORMAL);
        Print("[RF Debug] --- Terrain Profile ---", LogLevel.NORMAL);

        float dirX = dx / horizontalDist;
        float dirZ = dz / horizontalDist;

        float worstObstruction = 0.0;
        float worstD1 = 0.0;
        float worstD2 = 0.0;
        int worstSample = -1;
        float worstFresnelIntrusion = 0.0;
        float worstFresnelRadius = 0.0;

        IRRU_RFPropagationModel model = GetInstance();

        for (int i = 1; i < numSamples; i++)
        {
            float d1 = i * SAMPLE_INTERVAL;
            float d2 = horizontalDist - d1;

            float sampleX = playerPos[0] + dirX * d1;
            float sampleZ = playerPos[2] + dirZ * d1;
            float terrainY = world.GetSurfaceY(sampleX, sampleZ);

            float losHeight = playerHeight + (targetHeight - playerHeight) * (d1 / horizontalDist);
            float fresnelRadius = model.CalculateFresnelRadius(d1, d2, horizontalDist, wavelength);
            float heightAboveLOS = terrainY - losHeight;

            string status;
            if (heightAboveLOS > 0)
            {
                status = "BLOCKED";
                if (heightAboveLOS > worstObstruction)
                {
                    worstObstruction = heightAboveLOS;
                    worstD1 = d1;
                    worstD2 = d2;
                    worstSample = i;
                }
            }
            else
            {
                float fresnelClearance = -heightAboveLOS;
                float clearancePercent = fresnelClearance / fresnelRadius * 100.0;

                if (clearancePercent >= 100.0)
                    status = "CLEAR";
                else if (clearancePercent >= 60.0)
                    status = "GOOD";
                else
                {
                    status = "PARTIAL";
                    float fresnelIntrusion = fresnelRadius - fresnelClearance;
                    if (fresnelIntrusion > worstFresnelIntrusion)
                    {
                        worstFresnelIntrusion = fresnelIntrusion;
                        worstFresnelRadius = fresnelRadius;
                    }
                }
            }

            Print(string.Format("  Sample %1: d=%2m, terrain=%3m, LOS=%4m, aboveLOS=%5m, Fresnel=%6m [%7]",
                i, d1, terrainY, losHeight, heightAboveLOS, fresnelRadius, status), LogLevel.NORMAL);
        }

        Print("[RF Debug] --- Calculations ---", LogLevel.NORMAL);

        float dy = targetHeight - playerHeight;
        float totalDistance = Math.Sqrt(horizontalDist * horizontalDist + dy * dy);
        float fspl = model.CalculateFreeSpacePathLoss(totalDistance, debugFreqMHz);
        Print(string.Format("[RF Debug] Free Space Path Loss: %1 dB (3D dist: %2 m)", fspl, totalDistance), LogLevel.NORMAL);

        float diffLoss = 0.0;
        if (worstObstruction > 0)
        {
            diffLoss = model.CalculateDiffractionLoss(worstObstruction, worstD1, worstD2, horizontalDist, wavelength);
            Print(string.Format("[RF Debug] Worst obstruction: Sample %1, h=%2 m above LOS", worstSample, worstObstruction), LogLevel.NORMAL);
            Print(string.Format("[RF Debug] Diffraction Loss: %1 dB", diffLoss), LogLevel.NORMAL);
        }
        else
        {
            Print("[RF Debug] No terrain above LOS (clear line-of-sight)", LogLevel.NORMAL);
        }

        float fresnelLoss = 0.0;
        if (worstFresnelIntrusion > 0 && worstFresnelRadius > 0)
        {
            float clearancePercent = 1.0 - (worstFresnelIntrusion / worstFresnelRadius);
            if (clearancePercent < 0.2)
                fresnelLoss = 3.0;
            else if (clearancePercent < 0.4)
                fresnelLoss = 2.0;
            else if (clearancePercent < 0.6)
                fresnelLoss = 1.0;

            if (fresnelLoss > 0)
                Print(string.Format("[RF Debug] Fresnel Zone Intrusion: %1 dB (clearance %2%%)", fresnelLoss, (clearancePercent * 100.0).ToString(0)), LogLevel.NORMAL);
        }

        Print("[RF Debug] --- Obstacle Raycast (Iterative) ---", LogLevel.NORMAL);
        vector startPos = playerPos + Vector(0, ANTENNA_HEIGHT, 0);
        vector endPos = targetPos + Vector(0, ANTENNA_HEIGHT, 0);

        vector traceDir = (endPos - startPos).Normalized();
        vector currentStart = startPos + traceDir * 1.0;

        float obstLoss = 0.0;
        int hitCount = 0;

        for (int rayIter = 0; rayIter < MAX_OBSTACLE_ITERATIONS; rayIter++)
        {
            float remainingDist = vector.Distance(currentStart, endPos);
            if (remainingDist < RAYCAST_STEP_OFFSET)
                break;

            TraceParam trace = new TraceParam();
            trace.Start = currentStart;
            trace.End = endPos;
            trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            trace.LayerMask = EPhysicsLayerPresets.Projectile;

            float traceResult = world.TraceMove(trace, null);

            if (traceResult >= 1.0)
            {
                if (hitCount == 0)
                    Print("[RF Debug] Raycast: No hit (clear path)", LogLevel.NORMAL);
                break;
            }

            hitCount++;
            vector hitPos = currentStart + (endPos - currentStart) * traceResult;
            float hitDist = vector.Distance(startPos, hitPos);

            Print(string.Format("[RF Debug] Hit #%1 at %2 m", hitCount, hitDist.ToString(1)), LogLevel.NORMAL);

            float hitLoss = 0.0;

            if (trace.TraceMaterial && trace.TraceMaterial.Length() > 0)
            {
                Print(string.Format("[RF Debug]   Material: \"%1\"", trace.TraceMaterial), LogLevel.NORMAL);
                float matLoss = model.GetMaterialLoss(trace.TraceMaterial);
                Print(string.Format("[RF Debug]   Material loss: %1 dB", matLoss), LogLevel.NORMAL);
                hitLoss += matLoss;
            }

            if (trace.TraceEnt)
            {
                string entType = trace.TraceEnt.Type().ToString();
                string entName = trace.TraceEnt.GetName();
                Print(string.Format("[RF Debug]   Entity: %1 (%2)", entType, entName), LogLevel.NORMAL);

                float entLoss = model.GetEntityLoss(trace.TraceEnt);
                Print(string.Format("[RF Debug]   Entity loss: %1 dB", entLoss), LogLevel.NORMAL);
                hitLoss += entLoss;
            }

            if (hitLoss <= 0)
            {
                hitLoss = 10.0;
                Print("[RF Debug]   Using default: 10 dB", LogLevel.NORMAL);
            }

            obstLoss += hitLoss;

            currentStart = hitPos + traceDir * RAYCAST_STEP_OFFSET;
        }

        Print(string.Format("[RF Debug] Total Obstacle Loss: %1 dB (%2 obstacles)", obstLoss, hitCount), LogLevel.NORMAL);

        float totalLoss = fspl + diffLoss + fresnelLoss + obstLoss;
        float quality = model.LossToQuality(totalLoss);

        Print("[RF Debug] === RESULT ===", LogLevel.NORMAL);
        Print(string.Format("[RF Debug] Total Loss: %1 dB", totalLoss), LogLevel.NORMAL);
        Print(string.Format("[RF Debug] Signal Quality: %1 (1.0=perfect, 0.0=no signal)", quality), LogLevel.NORMAL);
        Print("=== END RF DEBUG ===", LogLevel.NORMAL);
    }
}
