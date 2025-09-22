//! Mortar ballistic tables for 81mm mortars
//! Data structure for ballistic calculations

//! Ballistic entry for mortar firing calculations
class MortarBallisticEntry
{
    int range;              
    int elevation;          
    float timeOfFlight;     
    int elevationCorrection;
    
    //------------------------------------------------------------------------------------------------
    //! Constructor for ballistic entry
    //! \param r Range in meters
    //! \param e Elevation in mils
    //! \param t Time of flight in seconds
    //! \param ec D ELEV PER 100M - altitude correction factor in mils per 100m elevation difference
    void MortarBallisticEntry(int r, int e, float t, int ec)
    {
        range = r;
        elevation = e;
        timeOfFlight = t;
        elevationCorrection = ec;
    }
}

//! Static class managing mortar ballistic calculations
class MortarBallisticTables
{
    protected static ref map<string, ref array<ref MortarBallisticEntry>> s_Tables;
    
    protected const float MAX_ELEVATION_MILS = 1515.0;
    protected const float MIN_ELEVATION_MILS = 800.0;
    protected const int MAX_CHARGE = 4;
    
    //------------------------------------------------------------------------------------------------
    //! Initialize all ballistic tables
    static void Initialize()
    {
        if (s_Tables)
            return;
            
        s_Tables = new map<string, ref array<ref MortarBallisticEntry>>();
        
        InitializeHETables();
        InitializeSmokeTables();
        InitializeIlluminationTables();
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get ballistic table for specific ammo type and charge
    //! \param ammoType Type of ammunition
    //! \param charge Charge level (0-4)
    //! \return Array of ballistic entries or null
    static array<ref MortarBallisticEntry> GetTable(string ammoType, int charge)
    {
        if (!s_Tables)
            Initialize();
            
        string key = string.Format("%1_%2", ammoType, charge);
        
        if (s_Tables.Contains(key))
            return s_Tables.Get(key);
            
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Calculate best firing solution for given range
    //! \param ammoType Type of ammunition
    //! \param range Target range in meters
    //! \param elevationMils Output elevation in mils
    //! \param timeOfFlight Output time of flight in seconds
    //! \param bestCharge Output best charge level
    //! \param dElevCorrection Output D_ELEV correction factor
    //! \return True if solution found
    static bool CalculateSolution(string ammoType, float range, out float elevationMils, out float timeOfFlight, out int bestCharge, out int dElevCorrection)
    {
        if (!s_Tables)
            Initialize();
        
        float bestTimeOfFlight = 9999.0;
        float bestElevation = 0;
        int bestChargeFound = -1;
        int bestDElev = 0;
        bool solutionFound = false;
        
        for (int charge = 0; charge <= MAX_CHARGE; charge++)
        {
            array<ref MortarBallisticEntry> table = GetTable(ammoType, charge);
            if (!table || table.Count() == 0)
                continue;
                
            MortarBallisticEntry firstEntry = table.Get(0);
            MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
            
            if (range >= firstEntry.range && range <= lastEntry.range)
            {
                float testElevation, testTimeOfFlight;
                int testDElev;
                InterpolateElevation(table, range, testElevation, testTimeOfFlight, testDElev);
                
                if (testElevation <= MAX_ELEVATION_MILS && testElevation >= MIN_ELEVATION_MILS)
                {
                    if (testTimeOfFlight < bestTimeOfFlight)
                    {
                        bestTimeOfFlight = testTimeOfFlight;
                        bestElevation = testElevation;
                        bestChargeFound = charge;
                        bestDElev = testDElev;
                        solutionFound = true;
                    }
                }
            }
        }
        
        if (solutionFound)
        {
            elevationMils = bestElevation;
            timeOfFlight = bestTimeOfFlight;
            bestCharge = bestChargeFound;
            dElevCorrection = bestDElev;
            return true;
        }
        
        bestCharge = -1;
        elevationMils = 0;
        timeOfFlight = 0;
        dElevCorrection = 0;
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get firing solution for specific charge
    //! \param ammoType Type of ammunition
    //! \param range Target range in meters
    //! \param charge Charge level
    //! \param elevationMils Output elevation in mils
    //! \param timeOfFlight Output time of flight in seconds
    //! \return True if solution found
    static bool GetSolutionForCharge(string ammoType, float range, int charge, out float elevationMils, out float timeOfFlight)
    {
        if (!s_Tables)
            Initialize();
            
        array<ref MortarBallisticEntry> table = GetTable(ammoType, charge);
        if (!table || table.Count() == 0)
            return false;
            
        MortarBallisticEntry firstEntry = table.Get(0);
        MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
        
        if (range < firstEntry.range || range > lastEntry.range)
            return false;
            
        int dElev;
        InterpolateElevation(table, range, elevationMils, timeOfFlight, dElev);
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Calculate firing solution for specific charge
    //! \param ammoType Type of ammunition
    //! \param charge Specific charge level
    //! \param range Target range in meters
    //! \param elevationMils Output elevation in mils
    //! \param timeOfFlight Output time of flight in seconds
    //! \param dElevCorrection Output D_ELEV correction factor
    //! \return True if solution found for this charge
    static bool CalculateSolutionForCharge(string ammoType, int charge, float range, out float elevationMils, out float timeOfFlight, out int dElevCorrection)
    {
        if (!s_Tables)
            Initialize();

        array<ref MortarBallisticEntry> table = GetTable(ammoType, charge);
        if (!table || table.Count() == 0)
            return false;

        MortarBallisticEntry firstEntry = table.Get(0);
        MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);

        if (range < firstEntry.range || range > lastEntry.range)
            return false;

        InterpolateElevation(table, range, elevationMils, timeOfFlight, dElevCorrection);
        return true;
    }

    //------------------------------------------------------------------------------------------------
    //! Interpolate elevation between table entries
    //! \param table Ballistic table
    //! \param range Target range
    //! \param elevationMils Output elevation in mils
    //! \param timeOfFlight Output time of flight
    //! \param dElevCorrection Output D_ELEV correction factor
    protected static void InterpolateElevation(array<ref MortarBallisticEntry> table, float range, out float elevationMils, out float timeOfFlight, out int dElevCorrection)
    {
        for (int i = 0; i < table.Count() - 1; i++)
        {
            MortarBallisticEntry entry1 = table.Get(i);
            MortarBallisticEntry entry2 = table.Get(i + 1);
            
            if (range >= entry1.range && range <= entry2.range)
            {
                float rangeDifference = range - entry1.range;
                float ratio = rangeDifference / (entry2.range - entry1.range);
                
                elevationMils = entry1.elevation - (entry1.elevation - entry2.elevation) * ratio;
                timeOfFlight = entry1.timeOfFlight + (entry2.timeOfFlight - entry1.timeOfFlight) * ratio;
                dElevCorrection = entry1.elevationCorrection;
                
                return;
            }
        }
        
        MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
        elevationMils = lastEntry.elevation;
        timeOfFlight = lastEntry.timeOfFlight;
        dElevCorrection = lastEntry.elevationCorrection;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get minimum and maximum range for ammo type
    //! \param ammoType Type of ammunition
    //! \param minRange Output minimum range
    //! \param maxRange Output maximum range
    static void GetMinMaxRange(string ammoType, out float minRange, out float maxRange)
    {
        if (!s_Tables)
            Initialize();
            
        minRange = 9999;
        maxRange = 0;
        
        for (int charge = 0; charge <= MAX_CHARGE; charge++)
        {
            array<ref MortarBallisticEntry> table = GetTable(ammoType, charge);
            if (!table || table.Count() == 0)
                continue;
                
            MortarBallisticEntry firstEntry = table.Get(0);
            MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
            
            if (firstEntry.range < minRange)
                minRange = firstEntry.range;
                
            if (lastEntry.range > maxRange)
                maxRange = lastEntry.range;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    //! Initialize HE ammunition tables
    protected static void InitializeHETables()
    {
        array<ref MortarBallisticEntry> heCharge4 = new array<ref MortarBallisticEntry>();
        heCharge4.Insert(new MortarBallisticEntry(400, 1531, 36.3, 9));
        heCharge4.Insert(new MortarBallisticEntry(500, 1514, 36.2, 9));
        heCharge4.Insert(new MortarBallisticEntry(600, 1496, 36.2, 9));
        heCharge4.Insert(new MortarBallisticEntry(700, 1478, 36.1, 9));
        heCharge4.Insert(new MortarBallisticEntry(800, 1460, 36.0, 9));
        heCharge4.Insert(new MortarBallisticEntry(900, 1442, 35.9, 9));
        heCharge4.Insert(new MortarBallisticEntry(1000, 1424, 35.8, 9));
        heCharge4.Insert(new MortarBallisticEntry(1100, 1405, 35.7, 10));
        heCharge4.Insert(new MortarBallisticEntry(1200, 1385, 35.6, 9));
        heCharge4.Insert(new MortarBallisticEntry(1300, 1366, 35.4, 10));
        heCharge4.Insert(new MortarBallisticEntry(1400, 1346, 35.3, 10));
        heCharge4.Insert(new MortarBallisticEntry(1500, 1326, 35.1, 11));
        heCharge4.Insert(new MortarBallisticEntry(1600, 1305, 34.9, 11));
        heCharge4.Insert(new MortarBallisticEntry(1700, 1283, 34.6, 11));
        heCharge4.Insert(new MortarBallisticEntry(1800, 1261, 34.4, 11));
        heCharge4.Insert(new MortarBallisticEntry(1900, 1238, 34.1, 12));
        heCharge4.Insert(new MortarBallisticEntry(2000, 1214, 33.8, 12));
        heCharge4.Insert(new MortarBallisticEntry(2100, 1188, 33.5, 13));
        heCharge4.Insert(new MortarBallisticEntry(2200, 1162, 33.1, 14));
        heCharge4.Insert(new MortarBallisticEntry(2300, 1134, 32.7, 15));
        heCharge4.Insert(new MortarBallisticEntry(2400, 1104, 32.2, 17));
        heCharge4.Insert(new MortarBallisticEntry(2500, 1070, 31.7, 17));
        heCharge4.Insert(new MortarBallisticEntry(2600, 1034, 31.0, 20));
        heCharge4.Insert(new MortarBallisticEntry(2700, 993, 30.3, 20));
        heCharge4.Insert(new MortarBallisticEntry(2800, 942, 29.2, 25));
        heCharge4.Insert(new MortarBallisticEntry(2900, 870, 27.7, 31));
        s_Tables.Insert("HE_4", heCharge4);
        
        array<ref MortarBallisticEntry> heCharge3 = new array<ref MortarBallisticEntry>();
        heCharge3.Insert(new MortarBallisticEntry(300, 1534, 31.7, 12));
        heCharge3.Insert(new MortarBallisticEntry(400, 1511, 31.6, 11));
        heCharge3.Insert(new MortarBallisticEntry(500, 1489, 31.6, 12));
        heCharge3.Insert(new MortarBallisticEntry(600, 1466, 31.5, 12));
        heCharge3.Insert(new MortarBallisticEntry(700, 1442, 31.4, 12));
        heCharge3.Insert(new MortarBallisticEntry(800, 1419, 31.3, 12));
        heCharge3.Insert(new MortarBallisticEntry(900, 1395, 31.1, 13));
        heCharge3.Insert(new MortarBallisticEntry(1000, 1370, 31.0, 13));
        heCharge3.Insert(new MortarBallisticEntry(1100, 1344, 30.8, 13));
        heCharge3.Insert(new MortarBallisticEntry(1200, 1318, 30.6, 13));
        heCharge3.Insert(new MortarBallisticEntry(1300, 1291, 30.3, 14));
        heCharge3.Insert(new MortarBallisticEntry(1400, 1263, 30.1, 15));
        heCharge3.Insert(new MortarBallisticEntry(1500, 1233, 29.7, 15));
        heCharge3.Insert(new MortarBallisticEntry(1600, 1202, 29.4, 16));
        heCharge3.Insert(new MortarBallisticEntry(1700, 1169, 29.0, 17));
        heCharge3.Insert(new MortarBallisticEntry(1800, 1133, 28.5, 19));
        heCharge3.Insert(new MortarBallisticEntry(1900, 1094, 28.0, 21));
        heCharge3.Insert(new MortarBallisticEntry(2000, 1051, 27.3, 26));
        heCharge3.Insert(new MortarBallisticEntry(2100, 999, 26.5, 31));
        heCharge3.Insert(new MortarBallisticEntry(2200, 931, 25.3, 46));
        heCharge3.Insert(new MortarBallisticEntry(2300, 801, 22.7, 0));
        s_Tables.Insert("HE_3", heCharge3);
        
        array<ref MortarBallisticEntry> heCharge2 = new array<ref MortarBallisticEntry>();
        heCharge2.Insert(new MortarBallisticEntry(200, 1538, 26.6, 15));
        heCharge2.Insert(new MortarBallisticEntry(300, 1507, 26.5, 15));
        heCharge2.Insert(new MortarBallisticEntry(400, 1475, 26.4, 16));
        heCharge2.Insert(new MortarBallisticEntry(500, 1443, 26.3, 16));
        heCharge2.Insert(new MortarBallisticEntry(600, 1410, 26.2, 16));
        heCharge2.Insert(new MortarBallisticEntry(700, 1376, 26.0, 17));
        heCharge2.Insert(new MortarBallisticEntry(800, 1341, 25.8, 18));
        heCharge2.Insert(new MortarBallisticEntry(900, 1305, 25.5, 20));
        heCharge2.Insert(new MortarBallisticEntry(1000, 1266, 25.2, 20));
        heCharge2.Insert(new MortarBallisticEntry(1100, 1225, 24.9, 22));
        heCharge2.Insert(new MortarBallisticEntry(1200, 1180, 24.4, 23));
        heCharge2.Insert(new MortarBallisticEntry(1300, 1132, 23.9, 27));
        heCharge2.Insert(new MortarBallisticEntry(1400, 1076, 23.2, 31));
        heCharge2.Insert(new MortarBallisticEntry(1500, 1009, 22.3, 43));
        heCharge2.Insert(new MortarBallisticEntry(1600, 912, 20.9, 109));
        s_Tables.Insert("HE_2", heCharge2);
        
        array<ref MortarBallisticEntry> heCharge1 = new array<ref MortarBallisticEntry>();
        heCharge1.Insert(new MortarBallisticEntry(100, 1547, 20.0, 28));
        heCharge1.Insert(new MortarBallisticEntry(200, 1492, 19.9, 28));
        heCharge1.Insert(new MortarBallisticEntry(300, 1437, 19.7, 27));
        heCharge1.Insert(new MortarBallisticEntry(400, 1378, 19.5, 29));
        heCharge1.Insert(new MortarBallisticEntry(500, 1317, 19.2, 31));
        heCharge1.Insert(new MortarBallisticEntry(600, 1249, 18.8, 35));
        heCharge1.Insert(new MortarBallisticEntry(700, 1174, 18.3, 42));
        heCharge1.Insert(new MortarBallisticEntry(800, 1085, 17.5, 52));
        heCharge1.Insert(new MortarBallisticEntry(900, 954, 16.1, 148));
        s_Tables.Insert("HE_1", heCharge1);
        
        array<ref MortarBallisticEntry> heCharge0 = new array<ref MortarBallisticEntry>();
        heCharge0.Insert(new MortarBallisticEntry(50, 1540, 13.2, 61));
        heCharge0.Insert(new MortarBallisticEntry(100, 1479, 13.2, 63));
        heCharge0.Insert(new MortarBallisticEntry(150, 1416, 13.0, 66));
        heCharge0.Insert(new MortarBallisticEntry(200, 1350, 12.8, 71));
        heCharge0.Insert(new MortarBallisticEntry(250, 1279, 12.6, 78));
        heCharge0.Insert(new MortarBallisticEntry(300, 1201, 12.3, 95));
        heCharge0.Insert(new MortarBallisticEntry(350, 1106, 11.7, 151));
        heCharge0.Insert(new MortarBallisticEntry(400, 955, 10.7, 0));
        s_Tables.Insert("HE_0", heCharge0);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Initialize smoke ammunition tables
    protected static void InitializeSmokeTables()
    {
        array<ref MortarBallisticEntry> smokeCharge4 = new array<ref MortarBallisticEntry>();
        smokeCharge4.Insert(new MortarBallisticEntry(400, 1517, 33.6, 11));
        smokeCharge4.Insert(new MortarBallisticEntry(500, 1495, 33.5, 10));
        smokeCharge4.Insert(new MortarBallisticEntry(600, 1474, 33.5, 10));
        smokeCharge4.Insert(new MortarBallisticEntry(700, 1452, 33.4, 11));
        smokeCharge4.Insert(new MortarBallisticEntry(800, 1429, 33.2, 11));
        smokeCharge4.Insert(new MortarBallisticEntry(900, 1407, 33.1, 12));
        smokeCharge4.Insert(new MortarBallisticEntry(1000, 1383, 33.0, 12));
        smokeCharge4.Insert(new MortarBallisticEntry(1100, 1360, 32.8, 12));
        smokeCharge4.Insert(new MortarBallisticEntry(1200, 1335, 32.6, 12));
        smokeCharge4.Insert(new MortarBallisticEntry(1300, 1310, 32.4, 13));
        smokeCharge4.Insert(new MortarBallisticEntry(1400, 1284, 32.1, 14));
        smokeCharge4.Insert(new MortarBallisticEntry(1500, 1257, 31.9, 14));
        smokeCharge4.Insert(new MortarBallisticEntry(1600, 1228, 31.5, 15));
        smokeCharge4.Insert(new MortarBallisticEntry(1700, 1199, 31.2, 17));
        smokeCharge4.Insert(new MortarBallisticEntry(1800, 1166, 30.8, 16));
        smokeCharge4.Insert(new MortarBallisticEntry(1900, 1132, 30.3, 18));
        smokeCharge4.Insert(new MortarBallisticEntry(2000, 1096, 29.8, 21));
        smokeCharge4.Insert(new MortarBallisticEntry(2100, 1055, 29.1, 23));
        smokeCharge4.Insert(new MortarBallisticEntry(2200, 1008, 28.4, 28));
        smokeCharge4.Insert(new MortarBallisticEntry(2300, 952, 27.4, 36));
        smokeCharge4.Insert(new MortarBallisticEntry(2400, 871, 25.8, 67));
        s_Tables.Insert("Smoke_4", smokeCharge4);

        array<ref MortarBallisticEntry> smokeCharge3 = new array<ref MortarBallisticEntry>();
        smokeCharge3.Insert(new MortarBallisticEntry(300, 1522, 29.6, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(400, 1495, 29.6, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(500, 1468, 29.5, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(600, 1440, 29.3, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(700, 1412, 29.2, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(800, 1383, 29.0, 14));
        smokeCharge3.Insert(new MortarBallisticEntry(900, 1354, 28.9, 16));
        smokeCharge3.Insert(new MortarBallisticEntry(1000, 1323, 28.6, 16));
        smokeCharge3.Insert(new MortarBallisticEntry(1100, 1291, 28.4, 17));
        smokeCharge3.Insert(new MortarBallisticEntry(1200, 1257, 28.1, 18));
        smokeCharge3.Insert(new MortarBallisticEntry(1300, 1221, 27.7, 18));
        smokeCharge3.Insert(new MortarBallisticEntry(1400, 1183, 27.3, 20));
        smokeCharge3.Insert(new MortarBallisticEntry(1500, 1142, 26.8, 23));
        smokeCharge3.Insert(new MortarBallisticEntry(1600, 1096, 26.2, 25));
        smokeCharge3.Insert(new MortarBallisticEntry(1700, 1044, 25.5, 30));
        smokeCharge3.Insert(new MortarBallisticEntry(1800, 980, 24.5, 38));
        smokeCharge3.Insert(new MortarBallisticEntry(1900, 892, 23.0, 84));
        s_Tables.Insert("Smoke_3", smokeCharge3);

        array<ref MortarBallisticEntry> smokeCharge2 = new array<ref MortarBallisticEntry>();
        smokeCharge2.Insert(new MortarBallisticEntry(200, 1528, 24.8, 19));
        smokeCharge2.Insert(new MortarBallisticEntry(300, 1491, 24.7, 19));
        smokeCharge2.Insert(new MortarBallisticEntry(400, 1453, 24.6, 19));
        smokeCharge2.Insert(new MortarBallisticEntry(500, 1414, 24.4, 19));
        smokeCharge2.Insert(new MortarBallisticEntry(600, 1374, 24.2, 20));
        smokeCharge2.Insert(new MortarBallisticEntry(700, 1333, 24.0, 22));
        smokeCharge2.Insert(new MortarBallisticEntry(800, 1289, 23.7, 23));
        smokeCharge2.Insert(new MortarBallisticEntry(900, 1242, 23.3, 25));
        smokeCharge2.Insert(new MortarBallisticEntry(1000, 1191, 22.9, 28));
        smokeCharge2.Insert(new MortarBallisticEntry(1100, 1133, 22.3, 31));
        smokeCharge2.Insert(new MortarBallisticEntry(1200, 1067, 21.6, 39));
        smokeCharge2.Insert(new MortarBallisticEntry(1300, 980, 20.5, 58));
        smokeCharge2.Insert(new MortarBallisticEntry(1400, 818, 18.0, 0));
        s_Tables.Insert("Smoke_2", smokeCharge2);

        array<ref MortarBallisticEntry> smokeCharge1 = new array<ref MortarBallisticEntry>();
        smokeCharge1.Insert(new MortarBallisticEntry(200, 1463, 17.7, 36));
        smokeCharge1.Insert(new MortarBallisticEntry(250, 1427, 17.6, 36));
        smokeCharge1.Insert(new MortarBallisticEntry(300, 1391, 17.5, 39));
        smokeCharge1.Insert(new MortarBallisticEntry(350, 1352, 17.3, 38));
        smokeCharge1.Insert(new MortarBallisticEntry(400, 1314, 17.2, 43));
        smokeCharge1.Insert(new MortarBallisticEntry(450, 1271, 16.9, 44));
        smokeCharge1.Insert(new MortarBallisticEntry(500, 1227, 16.7, 49));
        smokeCharge1.Insert(new MortarBallisticEntry(550, 1178, 16.4, 54));
        smokeCharge1.Insert(new MortarBallisticEntry(600, 1124, 16.0, 64));
        smokeCharge1.Insert(new MortarBallisticEntry(650, 1060, 15.4, 78));
        smokeCharge1.Insert(new MortarBallisticEntry(700, 982, 14.7, 160));
        smokeCharge1.Insert(new MortarBallisticEntry(750, 822, 13.0, 0));
        s_Tables.Insert("Smoke_1", smokeCharge1);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Initialize illumination ammunition tables
    protected static void InitializeIlluminationTables()
    {
        array<ref MortarBallisticEntry> illumCharge4 = new array<ref MortarBallisticEntry>();
        illumCharge4.Insert(new MortarBallisticEntry(400, 1515, 35.7, 11));
        illumCharge4.Insert(new MortarBallisticEntry(500, 1493, 35.7, 11));
        illumCharge4.Insert(new MortarBallisticEntry(600, 1471, 35.6, 11));
        illumCharge4.Insert(new MortarBallisticEntry(700, 1448, 35.5, 11));
        illumCharge4.Insert(new MortarBallisticEntry(800, 1426, 35.4, 11));
        illumCharge4.Insert(new MortarBallisticEntry(900, 1402, 35.2, 12));
        illumCharge4.Insert(new MortarBallisticEntry(1000, 1378, 35.0, 12));
        illumCharge4.Insert(new MortarBallisticEntry(1100, 1353, 34.8, 13));
        illumCharge4.Insert(new MortarBallisticEntry(1200, 1328, 34.6, 13));
        illumCharge4.Insert(new MortarBallisticEntry(1300, 1301, 34.4, 14));
        illumCharge4.Insert(new MortarBallisticEntry(1400, 1274, 34.1, 14));
        illumCharge4.Insert(new MortarBallisticEntry(1500, 1245, 33.8, 15));
        illumCharge4.Insert(new MortarBallisticEntry(1600, 1215, 33.4, 17));
        illumCharge4.Insert(new MortarBallisticEntry(1700, 1184, 33.0, 17));
        illumCharge4.Insert(new MortarBallisticEntry(1800, 1151, 32.6, 18));
        illumCharge4.Insert(new MortarBallisticEntry(1900, 1115, 32.1, 19));
        illumCharge4.Insert(new MortarBallisticEntry(2000, 1076, 31.5, 21));
        illumCharge4.Insert(new MortarBallisticEntry(2100, 1033, 30.9, 23));
        illumCharge4.Insert(new MortarBallisticEntry(2200, 985, 29.8, 27));
        illumCharge4.Insert(new MortarBallisticEntry(2300, 928, 28.8, 33));
        illumCharge4.Insert(new MortarBallisticEntry(2400, 855, 27.4, 52));
        s_Tables.Insert("Illumination_4", illumCharge4);

        array<ref MortarBallisticEntry> illumCharge3 = new array<ref MortarBallisticEntry>();
        illumCharge3.Insert(new MortarBallisticEntry(300, 1521, 31.1, 14));
        illumCharge3.Insert(new MortarBallisticEntry(400, 1494, 31.1, 14));
        illumCharge3.Insert(new MortarBallisticEntry(500, 1466, 31.0, 14));
        illumCharge3.Insert(new MortarBallisticEntry(600, 1438, 30.8, 14));
        illumCharge3.Insert(new MortarBallisticEntry(700, 1409, 30.7, 14));
        illumCharge3.Insert(new MortarBallisticEntry(800, 1380, 30.5, 16));
        illumCharge3.Insert(new MortarBallisticEntry(900, 1349, 30.3, 16));
        illumCharge3.Insert(new MortarBallisticEntry(1000, 1317, 30.1, 16));
        illumCharge3.Insert(new MortarBallisticEntry(1100, 1284, 29.8, 18));
        illumCharge3.Insert(new MortarBallisticEntry(1200, 1249, 29.4, 19));
        illumCharge3.Insert(new MortarBallisticEntry(1300, 1212, 29.1, 20));
        illumCharge3.Insert(new MortarBallisticEntry(1400, 1172, 28.6, 21));
        illumCharge3.Insert(new MortarBallisticEntry(1500, 1128, 28.1, 22));
        illumCharge3.Insert(new MortarBallisticEntry(1600, 1081, 27.4, 26));
        illumCharge3.Insert(new MortarBallisticEntry(1700, 1027, 26.6, 30));
        illumCharge3.Insert(new MortarBallisticEntry(1800, 962, 25.6, 39));
        illumCharge3.Insert(new MortarBallisticEntry(1900, 875, 24.1, 67));
        s_Tables.Insert("Illumination_3", illumCharge3);

        array<ref MortarBallisticEntry> illumCharge2 = new array<ref MortarBallisticEntry>();
        illumCharge2.Insert(new MortarBallisticEntry(200, 1529, 26.2, 17));
        illumCharge2.Insert(new MortarBallisticEntry(300, 1493, 26.1, 18));
        illumCharge2.Insert(new MortarBallisticEntry(400, 1457, 26.0, 19));
        illumCharge2.Insert(new MortarBallisticEntry(500, 1419, 25.8, 19));
        illumCharge2.Insert(new MortarBallisticEntry(600, 1379, 25.6, 20));
        illumCharge2.Insert(new MortarBallisticEntry(700, 1338, 25.4, 21));
        illumCharge2.Insert(new MortarBallisticEntry(800, 1295, 25.1, 23));
        illumCharge2.Insert(new MortarBallisticEntry(900, 1249, 24.7, 25));
        illumCharge2.Insert(new MortarBallisticEntry(1000, 1199, 24.3, 27));
        illumCharge2.Insert(new MortarBallisticEntry(1100, 1144, 23.7, 30));
        illumCharge2.Insert(new MortarBallisticEntry(1200, 1081, 23.0, 35));
        illumCharge2.Insert(new MortarBallisticEntry(1300, 1005, 22.0, 47));
        illumCharge2.Insert(new MortarBallisticEntry(1400, 900, 20.5, 98));
        s_Tables.Insert("Illumination_2", illumCharge2);

        array<ref MortarBallisticEntry> illumCharge1 = new array<ref MortarBallisticEntry>();
        illumCharge1.Insert(new MortarBallisticEntry(200, 1463, 18.1, 35));
        illumCharge1.Insert(new MortarBallisticEntry(250, 1428, 18.0, 37));
        illumCharge1.Insert(new MortarBallisticEntry(300, 1391, 17.9, 39));
        illumCharge1.Insert(new MortarBallisticEntry(350, 1352, 17.7, 40));
        illumCharge1.Insert(new MortarBallisticEntry(400, 1312, 17.5, 43));
        illumCharge1.Insert(new MortarBallisticEntry(450, 1269, 17.3, 45));
        illumCharge1.Insert(new MortarBallisticEntry(500, 1224, 17.0, 49));
        illumCharge1.Insert(new MortarBallisticEntry(550, 1175, 16.7, 55));
        illumCharge1.Insert(new MortarBallisticEntry(600, 1120, 16.3, 65));
        illumCharge1.Insert(new MortarBallisticEntry(650, 1055, 15.7, 81));
        illumCharge1.Insert(new MortarBallisticEntry(700, 974, 15.0, 151));
        illumCharge1.Insert(new MortarBallisticEntry(750, 823, 13.3, 0));
        s_Tables.Insert("Illumination_1", illumCharge1);
    }
}