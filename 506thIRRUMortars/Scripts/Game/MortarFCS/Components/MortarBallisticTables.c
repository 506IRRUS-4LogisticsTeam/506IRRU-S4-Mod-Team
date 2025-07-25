// Mortar Ballistic Tables for 81mm mortars
// Data structure for ballistic calculations

class MortarBallisticEntry
{
    int range;          // Range in meters
    int elevation;      // Elevation in mils
    float timeOfFlight; // Time of flight in seconds
    
    void MortarBallisticEntry(int r, int e, float t)
    {
        range = r;
        elevation = e;
        timeOfFlight = t;
    }
}

class MortarBallisticTables
{
    protected static ref map<string, ref array<ref MortarBallisticEntry>> s_Tables;
    
    //------------------------------------------------------------------------------------------------
    static void Initialize()
    {
        if (s_Tables)
            return; // Already initialized
            
        s_Tables = new map<string, ref array<ref MortarBallisticEntry>>();
        
        // Initialize all tables
        InitializeHETables();
        InitializeSmokeTables();
        InitializeIlluminationTables();
    }
    
    //------------------------------------------------------------------------------------------------
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
    static bool CalculateSolution(string ammoType, float range, out float elevationMils, out float timeOfFlight, out int bestCharge)
    {
        if (!s_Tables)
            Initialize();
            
        const float MAX_ELEVATION_MILS = 1515.0; // Physical mortar elevation limit
        
        float bestTimeOfFlight = 9999.0;
        float bestElevation = 0;
        int bestChargeFound = -1;
        bool solutionFound = false;
        
        // Check all charges and find the one with shortest flight time that respects elevation limit
        for (int charge = 0; charge <= 4; charge++)
        {
            array<ref MortarBallisticEntry> table = GetTable(ammoType, charge);
            if (!table || table.Count() == 0)
                continue;
                
            // Check if this charge can reach the target
            MortarBallisticEntry firstEntry = table.Get(0);
            MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
            
            if (range >= firstEntry.range && range <= lastEntry.range)
            {
                // Calculate the elevation and time for this charge
                float testElevation, testTimeOfFlight;
                InterpolateElevation(table, range, testElevation, testTimeOfFlight);
                
                // Check if elevation is within physical limits
                if (testElevation <= MAX_ELEVATION_MILS)
                {
                    // This is a valid solution - check if it's better (shorter flight time)
                    if (testTimeOfFlight < bestTimeOfFlight)
                    {
                        bestTimeOfFlight = testTimeOfFlight;
                        bestElevation = testElevation;
                        bestChargeFound = charge;
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
            return true;
        }
        
        // No valid solution found within elevation limits
        bestCharge = -1;
        elevationMils = 0;
        timeOfFlight = 0;
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    protected static void InterpolateElevation(array<ref MortarBallisticEntry> table, float range, out float elevationMils, out float timeOfFlight)
    {
        // Find the two entries to interpolate between
        for (int i = 0; i < table.Count() - 1; i++)
        {
            MortarBallisticEntry entry1 = table.Get(i);
            MortarBallisticEntry entry2 = table.Get(i + 1);
            
            if (range >= entry1.range && range <= entry2.range)
            {
                // Linear interpolation
                float ratio = (range - entry1.range) / (entry2.range - entry1.range);
                elevationMils = entry1.elevation - (entry1.elevation - entry2.elevation) * ratio;
                timeOfFlight = entry1.timeOfFlight + (entry2.timeOfFlight - entry1.timeOfFlight) * ratio;
                return;
            }
        }
        
        // Edge case - exact match with last entry
        MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
        elevationMils = lastEntry.elevation;
        timeOfFlight = lastEntry.timeOfFlight;
    }
    
    //------------------------------------------------------------------------------------------------
    protected static void InitializeHETables()
    {
        // HE Charge 4 (4 rings) - Range 400-2900m
        array<ref MortarBallisticEntry> heCharge4 = new array<ref MortarBallisticEntry>();
        heCharge4.Insert(new MortarBallisticEntry(400, 1531, 36.3));
        heCharge4.Insert(new MortarBallisticEntry(500, 1514, 36.2));
        heCharge4.Insert(new MortarBallisticEntry(600, 1496, 36.2));
        heCharge4.Insert(new MortarBallisticEntry(700, 1478, 36.1));
        heCharge4.Insert(new MortarBallisticEntry(800, 1460, 36.0));
        heCharge4.Insert(new MortarBallisticEntry(900, 1442, 35.9));
        heCharge4.Insert(new MortarBallisticEntry(1000, 1424, 35.8));
        heCharge4.Insert(new MortarBallisticEntry(1100, 1405, 35.7));
        heCharge4.Insert(new MortarBallisticEntry(1200, 1385, 35.6));
        heCharge4.Insert(new MortarBallisticEntry(1300, 1366, 35.4));
        heCharge4.Insert(new MortarBallisticEntry(1400, 1346, 35.3));
        heCharge4.Insert(new MortarBallisticEntry(1500, 1326, 35.1));
        heCharge4.Insert(new MortarBallisticEntry(1600, 1305, 34.9));
        heCharge4.Insert(new MortarBallisticEntry(1700, 1283, 34.6));
        heCharge4.Insert(new MortarBallisticEntry(1800, 1261, 34.4));
        heCharge4.Insert(new MortarBallisticEntry(1900, 1238, 34.1));
        heCharge4.Insert(new MortarBallisticEntry(2000, 1214, 33.8));
        heCharge4.Insert(new MortarBallisticEntry(2100, 1188, 33.5));
        heCharge4.Insert(new MortarBallisticEntry(2200, 1162, 33.1));
        heCharge4.Insert(new MortarBallisticEntry(2300, 1134, 32.7));
        heCharge4.Insert(new MortarBallisticEntry(2400, 1104, 32.2));
        heCharge4.Insert(new MortarBallisticEntry(2500, 1070, 31.7));
        heCharge4.Insert(new MortarBallisticEntry(2600, 1034, 31.0));
        heCharge4.Insert(new MortarBallisticEntry(2700, 993, 30.3));
        heCharge4.Insert(new MortarBallisticEntry(2800, 942, 29.2));
        heCharge4.Insert(new MortarBallisticEntry(2900, 870, 27.7));
        s_Tables.Insert("HE_4", heCharge4);
        
        // HE Charge 3 (3 rings) - Range 300-2300m
        array<ref MortarBallisticEntry> heCharge3 = new array<ref MortarBallisticEntry>();
        heCharge3.Insert(new MortarBallisticEntry(300, 1534, 31.7));
        heCharge3.Insert(new MortarBallisticEntry(400, 1511, 31.6));
        heCharge3.Insert(new MortarBallisticEntry(500, 1489, 31.6));
        heCharge3.Insert(new MortarBallisticEntry(600, 1466, 31.5));
        heCharge3.Insert(new MortarBallisticEntry(700, 1442, 31.4));
        heCharge3.Insert(new MortarBallisticEntry(800, 1419, 31.3));
        heCharge3.Insert(new MortarBallisticEntry(900, 1395, 31.1));
        heCharge3.Insert(new MortarBallisticEntry(1000, 1370, 31.0));
        heCharge3.Insert(new MortarBallisticEntry(1100, 1344, 30.8));
        heCharge3.Insert(new MortarBallisticEntry(1200, 1318, 30.6));
        heCharge3.Insert(new MortarBallisticEntry(1300, 1291, 30.3));
        heCharge3.Insert(new MortarBallisticEntry(1400, 1263, 30.1));
        heCharge3.Insert(new MortarBallisticEntry(1500, 1233, 29.7));
        heCharge3.Insert(new MortarBallisticEntry(1600, 1202, 29.4));
        heCharge3.Insert(new MortarBallisticEntry(1700, 1169, 29.0));
        heCharge3.Insert(new MortarBallisticEntry(1800, 1133, 28.5));
        heCharge3.Insert(new MortarBallisticEntry(1900, 1094, 28.0));
        heCharge3.Insert(new MortarBallisticEntry(2000, 1051, 27.3));
        heCharge3.Insert(new MortarBallisticEntry(2100, 999, 26.5));
        heCharge3.Insert(new MortarBallisticEntry(2200, 931, 25.3));
        heCharge3.Insert(new MortarBallisticEntry(2300, 801, 22.7));
        s_Tables.Insert("HE_3", heCharge3);
        
        // HE Charge 2 (2 rings) - Range 200-1600m
        array<ref MortarBallisticEntry> heCharge2 = new array<ref MortarBallisticEntry>();
        heCharge2.Insert(new MortarBallisticEntry(200, 1538, 26.6));
        heCharge2.Insert(new MortarBallisticEntry(300, 1507, 26.5));
        heCharge2.Insert(new MortarBallisticEntry(400, 1475, 26.4));
        heCharge2.Insert(new MortarBallisticEntry(500, 1443, 26.3));
        heCharge2.Insert(new MortarBallisticEntry(600, 1410, 26.2));
        heCharge2.Insert(new MortarBallisticEntry(700, 1376, 26.0));
        heCharge2.Insert(new MortarBallisticEntry(800, 1341, 25.8));
        heCharge2.Insert(new MortarBallisticEntry(900, 1305, 25.5));
        heCharge2.Insert(new MortarBallisticEntry(1000, 1266, 25.2));
        heCharge2.Insert(new MortarBallisticEntry(1100, 1225, 24.9));
        heCharge2.Insert(new MortarBallisticEntry(1200, 1180, 24.4));
        heCharge2.Insert(new MortarBallisticEntry(1300, 1132, 23.9));
        heCharge2.Insert(new MortarBallisticEntry(1400, 1076, 23.2));
        heCharge2.Insert(new MortarBallisticEntry(1500, 1009, 22.3));
        heCharge2.Insert(new MortarBallisticEntry(1600, 912, 20.9));
        s_Tables.Insert("HE_2", heCharge2);
        
        // HE Charge 1 (1 ring) - Range 100-900m
        array<ref MortarBallisticEntry> heCharge1 = new array<ref MortarBallisticEntry>();
        heCharge1.Insert(new MortarBallisticEntry(100, 1547, 20.0));
        heCharge1.Insert(new MortarBallisticEntry(200, 1492, 19.9));
        heCharge1.Insert(new MortarBallisticEntry(300, 1437, 19.7));
        heCharge1.Insert(new MortarBallisticEntry(400, 1378, 19.5));
        heCharge1.Insert(new MortarBallisticEntry(500, 1317, 19.2));
        heCharge1.Insert(new MortarBallisticEntry(600, 1249, 18.8));
        heCharge1.Insert(new MortarBallisticEntry(700, 1174, 18.3));
        heCharge1.Insert(new MortarBallisticEntry(800, 1085, 17.5));
        heCharge1.Insert(new MortarBallisticEntry(900, 954, 16.1));
        s_Tables.Insert("HE_1", heCharge1);
        
        // HE Charge 0 (0 rings) - Range 50-400m
        array<ref MortarBallisticEntry> heCharge0 = new array<ref MortarBallisticEntry>();
        heCharge0.Insert(new MortarBallisticEntry(50, 1540, 13.2));
        heCharge0.Insert(new MortarBallisticEntry(100, 1479, 13.2));
        heCharge0.Insert(new MortarBallisticEntry(150, 1416, 13.0));
        heCharge0.Insert(new MortarBallisticEntry(200, 1350, 12.8));
        heCharge0.Insert(new MortarBallisticEntry(250, 1279, 12.6));
        heCharge0.Insert(new MortarBallisticEntry(300, 1201, 12.3));
        heCharge0.Insert(new MortarBallisticEntry(350, 1106, 11.7));
        heCharge0.Insert(new MortarBallisticEntry(400, 955, 10.7));
        s_Tables.Insert("HE_0", heCharge0);
    }
    
    //------------------------------------------------------------------------------------------------
    protected static void InitializeSmokeTables()
    {
        // SMOKE Charge 4
        array<ref MortarBallisticEntry> smokeCharge4 = new array<ref MortarBallisticEntry>();
        smokeCharge4.Insert(new MortarBallisticEntry(400, 1517, 33.6));
        smokeCharge4.Insert(new MortarBallisticEntry(500, 1495, 33.5));
        smokeCharge4.Insert(new MortarBallisticEntry(600, 1474, 33.5));
        smokeCharge4.Insert(new MortarBallisticEntry(700, 1452, 33.4));
        smokeCharge4.Insert(new MortarBallisticEntry(800, 1429, 33.2));
        smokeCharge4.Insert(new MortarBallisticEntry(900, 1407, 33.1));
        smokeCharge4.Insert(new MortarBallisticEntry(1000, 1383, 33.0));
        smokeCharge4.Insert(new MortarBallisticEntry(1100, 1360, 32.8));
        smokeCharge4.Insert(new MortarBallisticEntry(1200, 1335, 32.6));
        smokeCharge4.Insert(new MortarBallisticEntry(1300, 1310, 32.4));
        smokeCharge4.Insert(new MortarBallisticEntry(1400, 1284, 32.1));
        smokeCharge4.Insert(new MortarBallisticEntry(1500, 1257, 31.9));
        smokeCharge4.Insert(new MortarBallisticEntry(1600, 1228, 31.5));
        smokeCharge4.Insert(new MortarBallisticEntry(1700, 1199, 31.2));
        smokeCharge4.Insert(new MortarBallisticEntry(1800, 1166, 30.8));
        smokeCharge4.Insert(new MortarBallisticEntry(1900, 1132, 30.3));
        smokeCharge4.Insert(new MortarBallisticEntry(2000, 1096, 29.8));
        smokeCharge4.Insert(new MortarBallisticEntry(2100, 1055, 29.1));
        smokeCharge4.Insert(new MortarBallisticEntry(2200, 1008, 28.4));
        smokeCharge4.Insert(new MortarBallisticEntry(2300, 952, 27.4));
        smokeCharge4.Insert(new MortarBallisticEntry(2400, 871, 25.8));
        s_Tables.Insert("Smoke_4", smokeCharge4);

        // SMOKE Charge 3
        array<ref MortarBallisticEntry> smokeCharge3 = new array<ref MortarBallisticEntry>();
        smokeCharge3.Insert(new MortarBallisticEntry(300, 1522, 29.6));
        smokeCharge3.Insert(new MortarBallisticEntry(400, 1495, 29.6));
        smokeCharge3.Insert(new MortarBallisticEntry(500, 1468, 29.5));
        smokeCharge3.Insert(new MortarBallisticEntry(600, 1440, 29.3));
        smokeCharge3.Insert(new MortarBallisticEntry(700, 1412, 29.2));
        smokeCharge3.Insert(new MortarBallisticEntry(800, 1383, 29.0));
        smokeCharge3.Insert(new MortarBallisticEntry(900, 1354, 28.9));
        smokeCharge3.Insert(new MortarBallisticEntry(1000, 1323, 28.6));
        smokeCharge3.Insert(new MortarBallisticEntry(1100, 1291, 28.4));
        smokeCharge3.Insert(new MortarBallisticEntry(1200, 1257, 28.1));
        smokeCharge3.Insert(new MortarBallisticEntry(1300, 1221, 27.7));
        smokeCharge3.Insert(new MortarBallisticEntry(1400, 1183, 27.3));
        smokeCharge3.Insert(new MortarBallisticEntry(1500, 1142, 26.8));
        smokeCharge3.Insert(new MortarBallisticEntry(1600, 1096, 26.2));
        smokeCharge3.Insert(new MortarBallisticEntry(1700, 1044, 25.5));
        smokeCharge3.Insert(new MortarBallisticEntry(1800, 980, 24.5));
        smokeCharge3.Insert(new MortarBallisticEntry(1900, 892, 23.0));
        s_Tables.Insert("Smoke_3", smokeCharge3);

        // SMOKE Charge 2
        array<ref MortarBallisticEntry> smokeCharge2 = new array<ref MortarBallisticEntry>();
        smokeCharge2.Insert(new MortarBallisticEntry(200, 1528, 24.8));
        smokeCharge2.Insert(new MortarBallisticEntry(300, 1491, 24.7));
        smokeCharge2.Insert(new MortarBallisticEntry(400, 1453, 24.6));
        smokeCharge2.Insert(new MortarBallisticEntry(500, 1414, 24.4));
        smokeCharge2.Insert(new MortarBallisticEntry(600, 1374, 24.2));
        smokeCharge2.Insert(new MortarBallisticEntry(700, 1333, 24.0));
        smokeCharge2.Insert(new MortarBallisticEntry(800, 1289, 23.7));
        smokeCharge2.Insert(new MortarBallisticEntry(900, 1242, 23.3));
        smokeCharge2.Insert(new MortarBallisticEntry(1000, 1191, 22.9));
        smokeCharge2.Insert(new MortarBallisticEntry(1100, 1133, 22.3));
        smokeCharge2.Insert(new MortarBallisticEntry(1200, 1067, 21.6));
        smokeCharge2.Insert(new MortarBallisticEntry(1300, 980, 20.5));
        smokeCharge2.Insert(new MortarBallisticEntry(1400, 818, 18.0));
        s_Tables.Insert("Smoke_2", smokeCharge2);

        // SMOKE Charge 1
        array<ref MortarBallisticEntry> smokeCharge1 = new array<ref MortarBallisticEntry>();
        smokeCharge1.Insert(new MortarBallisticEntry(200, 1463, 17.7));
        smokeCharge1.Insert(new MortarBallisticEntry(250, 1427, 17.6));
        smokeCharge1.Insert(new MortarBallisticEntry(300, 1391, 17.5));
        smokeCharge1.Insert(new MortarBallisticEntry(350, 1352, 17.3));
        smokeCharge1.Insert(new MortarBallisticEntry(400, 1314, 17.2));
        smokeCharge1.Insert(new MortarBallisticEntry(450, 1271, 16.9));
        smokeCharge1.Insert(new MortarBallisticEntry(500, 1227, 16.7));
        smokeCharge1.Insert(new MortarBallisticEntry(550, 1178, 16.4));
        smokeCharge1.Insert(new MortarBallisticEntry(600, 1124, 16.0));
        smokeCharge1.Insert(new MortarBallisticEntry(650, 1060, 15.4));
        smokeCharge1.Insert(new MortarBallisticEntry(700, 982, 14.7));
        smokeCharge1.Insert(new MortarBallisticEntry(750, 822, 13.0));
        s_Tables.Insert("Smoke_1", smokeCharge1);
    }

    
    //------------------------------------------------------------------------------------------------
    protected static void InitializeIlluminationTables()
    {
        // ILLUMINATION Charge 4
        array<ref MortarBallisticEntry> illumCharge4 = new array<ref MortarBallisticEntry>();
        illumCharge4.Insert(new MortarBallisticEntry(400, 1515, 35.7));
        illumCharge4.Insert(new MortarBallisticEntry(500, 1493, 35.7));
        illumCharge4.Insert(new MortarBallisticEntry(600, 1471, 35.6));
        illumCharge4.Insert(new MortarBallisticEntry(700, 1448, 35.5));
        illumCharge4.Insert(new MortarBallisticEntry(800, 1426, 35.4));
        illumCharge4.Insert(new MortarBallisticEntry(900, 1402, 35.2));
        illumCharge4.Insert(new MortarBallisticEntry(1000, 1378, 35.0));
        illumCharge4.Insert(new MortarBallisticEntry(1100, 1353, 34.8));
        illumCharge4.Insert(new MortarBallisticEntry(1200, 1328, 34.6));
        illumCharge4.Insert(new MortarBallisticEntry(1300, 1301, 34.4));
        illumCharge4.Insert(new MortarBallisticEntry(1400, 1274, 34.1));
        illumCharge4.Insert(new MortarBallisticEntry(1500, 1245, 33.8));
        illumCharge4.Insert(new MortarBallisticEntry(1600, 1215, 33.4));
        illumCharge4.Insert(new MortarBallisticEntry(1700, 1184, 33.0));
        illumCharge4.Insert(new MortarBallisticEntry(1800, 1151, 32.6));
        illumCharge4.Insert(new MortarBallisticEntry(1900, 1115, 32.1));
        illumCharge4.Insert(new MortarBallisticEntry(2000, 1076, 31.5));
        illumCharge4.Insert(new MortarBallisticEntry(2100, 1033, 30.9));
        illumCharge4.Insert(new MortarBallisticEntry(2200, 985, 29.8));
        illumCharge4.Insert(new MortarBallisticEntry(2300, 928, 28.8));
        illumCharge4.Insert(new MortarBallisticEntry(2400, 855, 27.4));
        s_Tables.Insert("Illumination_4", illumCharge4);

        // ILLUMINATION Charge 3
        array<ref MortarBallisticEntry> illumCharge3 = new array<ref MortarBallisticEntry>();
        illumCharge3.Insert(new MortarBallisticEntry(300, 1521, 31.1));
        illumCharge3.Insert(new MortarBallisticEntry(400, 1494, 31.1));
        illumCharge3.Insert(new MortarBallisticEntry(500, 1466, 31.0));
        illumCharge3.Insert(new MortarBallisticEntry(600, 1438, 30.8));
        illumCharge3.Insert(new MortarBallisticEntry(700, 1409, 30.7));
        illumCharge3.Insert(new MortarBallisticEntry(800, 1380, 30.5));
        illumCharge3.Insert(new MortarBallisticEntry(900, 1349, 30.3));
        illumCharge3.Insert(new MortarBallisticEntry(1000, 1317, 30.1));
        illumCharge3.Insert(new MortarBallisticEntry(1100, 1284, 29.8));
        illumCharge3.Insert(new MortarBallisticEntry(1200, 1249, 29.4));
        illumCharge3.Insert(new MortarBallisticEntry(1300, 1212, 29.1));
        illumCharge3.Insert(new MortarBallisticEntry(1400, 1172, 28.6));
        illumCharge3.Insert(new MortarBallisticEntry(1500, 1128, 28.1));
        illumCharge3.Insert(new MortarBallisticEntry(1600, 1081, 27.4));
        illumCharge3.Insert(new MortarBallisticEntry(1700, 1027, 26.6));
        illumCharge3.Insert(new MortarBallisticEntry(1800, 962, 25.6));
        illumCharge3.Insert(new MortarBallisticEntry(1900, 875, 24.1));
        s_Tables.Insert("Illumination_3", illumCharge3);

        // ILLUMINATION Charge 2
        array<ref MortarBallisticEntry> illumCharge2 = new array<ref MortarBallisticEntry>();
        illumCharge2.Insert(new MortarBallisticEntry(200, 1529, 26.2));
        illumCharge2.Insert(new MortarBallisticEntry(300, 1493, 26.1));
        illumCharge2.Insert(new MortarBallisticEntry(400, 1457, 26.0));
        illumCharge2.Insert(new MortarBallisticEntry(500, 1419, 25.8));
        illumCharge2.Insert(new MortarBallisticEntry(600, 1379, 25.6));
        illumCharge2.Insert(new MortarBallisticEntry(700, 1338, 25.4));
        illumCharge2.Insert(new MortarBallisticEntry(800, 1295, 25.1));
        illumCharge2.Insert(new MortarBallisticEntry(900, 1249, 24.7));
        illumCharge2.Insert(new MortarBallisticEntry(1000, 1199, 24.3));
        illumCharge2.Insert(new MortarBallisticEntry(1100, 1144, 23.7));
        illumCharge2.Insert(new MortarBallisticEntry(1200, 1081, 23.0));
        illumCharge2.Insert(new MortarBallisticEntry(1300, 1005, 22.0));
        illumCharge2.Insert(new MortarBallisticEntry(1400, 900, 20.5));
        s_Tables.Insert("Illumination_2", illumCharge2);

        // ILLUMINATION Charge 1
        array<ref MortarBallisticEntry> illumCharge1 = new array<ref MortarBallisticEntry>();
        illumCharge1.Insert(new MortarBallisticEntry(200, 1463, 18.1));
        illumCharge1.Insert(new MortarBallisticEntry(250, 1428, 18.0));
        illumCharge1.Insert(new MortarBallisticEntry(300, 1391, 17.9));
        illumCharge1.Insert(new MortarBallisticEntry(350, 1352, 17.7));
        illumCharge1.Insert(new MortarBallisticEntry(400, 1312, 17.5));
        illumCharge1.Insert(new MortarBallisticEntry(450, 1269, 17.3));
        illumCharge1.Insert(new MortarBallisticEntry(500, 1224, 17.0));
        illumCharge1.Insert(new MortarBallisticEntry(550, 1175, 16.7));
        illumCharge1.Insert(new MortarBallisticEntry(600, 1120, 16.3));
        illumCharge1.Insert(new MortarBallisticEntry(650, 1055, 15.7));
        illumCharge1.Insert(new MortarBallisticEntry(700, 974, 15.0));
        illumCharge1.Insert(new MortarBallisticEntry(750, 823, 13.3));
        s_Tables.Insert("Illumination_1", illumCharge1);
    }

    
    //------------------------------------------------------------------------------------------------
    static void GetMinMaxRange(string ammoType, out float minRange, out float maxRange)
    {
        if (!s_Tables)
            Initialize();
            
        minRange = 9999;
        maxRange = 0;
        
        // Check all charges for this ammo type
        for (int charge = 0; charge <= 4; charge++)
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
}