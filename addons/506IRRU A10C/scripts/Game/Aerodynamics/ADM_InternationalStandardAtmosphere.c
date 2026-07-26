enum ADM_ISAProperties
{
	Temperature,
	Pressure,
	Density,
	SpeedOfSound
}

// International Standard Atmosphere Model
// Arrays are split into chunks to stay within EnforceScript's per-function bytecode limit.
class ADM_InternationalStandardAtmosphere
{
	private static ref array<float> altitude;
	private static ref array<float> temperature;
	private static ref array<float> pressure;
	private static ref array<float> density;
	private static ref array<float> speed_of_sound;
	private static bool m_bInitialized = false;

	// =====================================================================
	// ALTITUDE (km) - 201 entries, split into 4 chunks
	// =====================================================================
	private static void InitAltitude_A()
	{
		altitude = {};
		altitude.Insert(0.0);   altitude.Insert(5.0);   altitude.Insert(10.0);  altitude.Insert(15.0);  altitude.Insert(20.0);
		altitude.Insert(25.0);  altitude.Insert(30.0);  altitude.Insert(35.0);  altitude.Insert(40.0);  altitude.Insert(45.0);
		altitude.Insert(50.0);  altitude.Insert(55.0);  altitude.Insert(60.0);  altitude.Insert(65.0);  altitude.Insert(70.0);
		altitude.Insert(75.0);  altitude.Insert(80.0);  altitude.Insert(85.0);  altitude.Insert(90.0);  altitude.Insert(95.0);
		altitude.Insert(100.0); altitude.Insert(105.0); altitude.Insert(110.0); altitude.Insert(115.0); altitude.Insert(120.0);
		altitude.Insert(125.0); altitude.Insert(130.0); altitude.Insert(135.0); altitude.Insert(140.0); altitude.Insert(145.0);
		altitude.Insert(150.0); altitude.Insert(155.0); altitude.Insert(160.0); altitude.Insert(165.0); altitude.Insert(170.0);
		altitude.Insert(175.0); altitude.Insert(180.0); altitude.Insert(185.0); altitude.Insert(190.0); altitude.Insert(195.0);
		altitude.Insert(200.0); altitude.Insert(205.0); altitude.Insert(210.0); altitude.Insert(215.0); altitude.Insert(220.0);
		altitude.Insert(225.0); altitude.Insert(230.0); altitude.Insert(235.0); altitude.Insert(240.0); altitude.Insert(245.0);
	}
	private static void InitAltitude_B()
	{
		altitude.Insert(250.0); altitude.Insert(255.0); altitude.Insert(260.0); altitude.Insert(265.0); altitude.Insert(270.0);
		altitude.Insert(275.0); altitude.Insert(280.0); altitude.Insert(285.0); altitude.Insert(290.0); altitude.Insert(295.0);
		altitude.Insert(300.0); altitude.Insert(305.0); altitude.Insert(310.0); altitude.Insert(315.0); altitude.Insert(320.0);
		altitude.Insert(325.0); altitude.Insert(330.0); altitude.Insert(335.0); altitude.Insert(340.0); altitude.Insert(345.0);
		altitude.Insert(350.0); altitude.Insert(355.0); altitude.Insert(360.0); altitude.Insert(365.0); altitude.Insert(370.0);
		altitude.Insert(375.0); altitude.Insert(380.0); altitude.Insert(385.0); altitude.Insert(390.0); altitude.Insert(395.0);
		altitude.Insert(400.0); altitude.Insert(405.0); altitude.Insert(410.0); altitude.Insert(415.0); altitude.Insert(420.0);
		altitude.Insert(425.0); altitude.Insert(430.0); altitude.Insert(435.0); altitude.Insert(440.0); altitude.Insert(445.0);
		altitude.Insert(450.0); altitude.Insert(455.0); altitude.Insert(460.0); altitude.Insert(465.0); altitude.Insert(470.0);
		altitude.Insert(475.0); altitude.Insert(480.0); altitude.Insert(485.0); altitude.Insert(490.0); altitude.Insert(495.0);
	}
	private static void InitAltitude_C()
	{
		altitude.Insert(500.0); altitude.Insert(505.0); altitude.Insert(510.0); altitude.Insert(515.0); altitude.Insert(520.0);
		altitude.Insert(525.0); altitude.Insert(530.0); altitude.Insert(535.0); altitude.Insert(540.0); altitude.Insert(545.0);
		altitude.Insert(550.0); altitude.Insert(555.0); altitude.Insert(560.0); altitude.Insert(565.0); altitude.Insert(570.0);
		altitude.Insert(575.0); altitude.Insert(580.0); altitude.Insert(585.0); altitude.Insert(590.0); altitude.Insert(595.0);
		altitude.Insert(600.0); altitude.Insert(605.0); altitude.Insert(610.0); altitude.Insert(615.0); altitude.Insert(620.0);
		altitude.Insert(625.0); altitude.Insert(630.0); altitude.Insert(635.0); altitude.Insert(640.0); altitude.Insert(645.0);
		altitude.Insert(650.0); altitude.Insert(655.0); altitude.Insert(660.0); altitude.Insert(665.0); altitude.Insert(670.0);
		altitude.Insert(675.0); altitude.Insert(680.0); altitude.Insert(685.0); altitude.Insert(690.0); altitude.Insert(695.0);
		altitude.Insert(700.0); altitude.Insert(705.0); altitude.Insert(710.0); altitude.Insert(715.0); altitude.Insert(720.0);
		altitude.Insert(725.0); altitude.Insert(730.0); altitude.Insert(735.0); altitude.Insert(740.0); altitude.Insert(745.0);
	}
	private static void InitAltitude_D()
	{
		altitude.Insert(750.0); altitude.Insert(755.0); altitude.Insert(760.0); altitude.Insert(765.0); altitude.Insert(770.0);
		altitude.Insert(775.0); altitude.Insert(780.0); altitude.Insert(785.0); altitude.Insert(790.0); altitude.Insert(795.0);
		altitude.Insert(800.0); altitude.Insert(805.0); altitude.Insert(810.0); altitude.Insert(815.0); altitude.Insert(820.0);
		altitude.Insert(825.0); altitude.Insert(830.0); altitude.Insert(835.0); altitude.Insert(840.0); altitude.Insert(845.0);
		altitude.Insert(850.0); altitude.Insert(855.0); altitude.Insert(860.0); altitude.Insert(865.0); altitude.Insert(870.0);
		altitude.Insert(875.0); altitude.Insert(880.0); altitude.Insert(885.0); altitude.Insert(890.0); altitude.Insert(895.0);
		altitude.Insert(900.0); altitude.Insert(905.0); altitude.Insert(910.0); altitude.Insert(915.0); altitude.Insert(920.0);
		altitude.Insert(925.0); altitude.Insert(930.0); altitude.Insert(935.0); altitude.Insert(940.0); altitude.Insert(945.0);
		altitude.Insert(950.0); altitude.Insert(955.0); altitude.Insert(960.0); altitude.Insert(965.0); altitude.Insert(970.0);
		altitude.Insert(975.0); altitude.Insert(980.0); altitude.Insert(985.0); altitude.Insert(990.0); altitude.Insert(995.0);
		altitude.Insert(1000.0);
	}

	// =====================================================================
	// TEMPERATURE (K) - 201 entries, split into 4 chunks
	// =====================================================================
	private static void InitTemperature_A()
	{
		temperature = {};
		temperature.Insert(288.150); temperature.Insert(255.676); temperature.Insert(223.252); temperature.Insert(216.650); temperature.Insert(216.650);
		temperature.Insert(221.552); temperature.Insert(226.509); temperature.Insert(236.513); temperature.Insert(250.350); temperature.Insert(264.164);
		temperature.Insert(270.650); temperature.Insert(260.771); temperature.Insert(247.021); temperature.Insert(233.292); temperature.Insert(219.585);
		temperature.Insert(208.399); temperature.Insert(198.639); temperature.Insert(188.893); temperature.Insert(186.867); temperature.Insert(188.418);
		temperature.Insert(195.081); temperature.Insert(208.835); temperature.Insert(240.000); temperature.Insert(300.000); temperature.Insert(360.000);
		temperature.Insert(417.231); temperature.Insert(469.268); temperature.Insert(516.589); temperature.Insert(559.627); temperature.Insert(598.776);
		temperature.Insert(634.392); temperature.Insert(666.799); temperature.Insert(696.290); temperature.Insert(723.132); temperature.Insert(747.566);
		temperature.Insert(769.811); temperature.Insert(790.066); temperature.Insert(808.511); temperature.Insert(825.312); temperature.Insert(840.616);
		temperature.Insert(854.559); temperature.Insert(867.264); temperature.Insert(878.842); temperature.Insert(889.395); temperature.Insert(899.014);
		temperature.Insert(907.785); temperature.Insert(915.782); temperature.Insert(923.075); temperature.Insert(929.726); temperature.Insert(935.794);
	}
	private static void InitTemperature_B()
	{
		temperature.Insert(941.330); temperature.Insert(946.381); temperature.Insert(950.991); temperature.Insert(955.198); temperature.Insert(959.039);
		temperature.Insert(962.545); temperature.Insert(965.746); temperature.Insert(968.670); temperature.Insert(971.340); temperature.Insert(973.779);
		temperature.Insert(976.008); temperature.Insert(978.044); temperature.Insert(979.904); temperature.Insert(981.605); temperature.Insert(983.159);
		temperature.Insert(984.580); temperature.Insert(985.880); temperature.Insert(987.068); temperature.Insert(988.154); temperature.Insert(989.148);
		temperature.Insert(990.057); temperature.Insert(990.889); temperature.Insert(991.650); temperature.Insert(992.347); temperature.Insert(992.984);
		temperature.Insert(993.568); temperature.Insert(994.102); temperature.Insert(994.591); temperature.Insert(995.039); temperature.Insert(995.450);
		temperature.Insert(995.825); temperature.Insert(996.170); temperature.Insert(996.485); temperature.Insert(996.774); temperature.Insert(997.039);
		temperature.Insert(997.282); temperature.Insert(997.505); temperature.Insert(997.709); temperature.Insert(997.896); temperature.Insert(998.067);
		temperature.Insert(998.225); temperature.Insert(998.369); temperature.Insert(998.502); temperature.Insert(998.623); temperature.Insert(998.735);
		temperature.Insert(998.837); temperature.Insert(998.931); temperature.Insert(999.017); temperature.Insert(999.096); temperature.Insert(999.169);
	}
	private static void InitTemperature_C()
	{
		temperature.Insert(999.236); temperature.Insert(999.297); temperature.Insert(999.353); temperature.Insert(999.405); temperature.Insert(999.452);
		temperature.Insert(999.496); temperature.Insert(999.536); temperature.Insert(999.573); temperature.Insert(999.607); temperature.Insert(999.638);
		temperature.Insert(999.667); temperature.Insert(999.693); temperature.Insert(999.717); temperature.Insert(999.740); temperature.Insert(999.760);
		temperature.Insert(999.779); temperature.Insert(999.796); temperature.Insert(999.812); temperature.Insert(999.827); temperature.Insert(999.841);
		temperature.Insert(999.853); temperature.Insert(999.864); temperature.Insert(999.875); temperature.Insert(999.885); temperature.Insert(999.894);
		temperature.Insert(999.902); temperature.Insert(999.910); temperature.Insert(999.917); temperature.Insert(999.923); temperature.Insert(999.929);
		temperature.Insert(999.934); temperature.Insert(999.939); temperature.Insert(999.944); temperature.Insert(999.948); temperature.Insert(999.952);
		temperature.Insert(999.956); temperature.Insert(999.959); temperature.Insert(999.962); temperature.Insert(999.965); temperature.Insert(999.968);
		temperature.Insert(999.970); temperature.Insert(999.973); temperature.Insert(999.975); temperature.Insert(999.977); temperature.Insert(999.978);
		temperature.Insert(999.980); temperature.Insert(999.982); temperature.Insert(999.983); temperature.Insert(999.984); temperature.Insert(999.985);
	}
	private static void InitTemperature_D()
	{
		temperature.Insert(999.986); temperature.Insert(999.987); temperature.Insert(999.988); temperature.Insert(999.989); temperature.Insert(999.990);
		temperature.Insert(999.991); temperature.Insert(999.992); temperature.Insert(999.992); temperature.Insert(999.993); temperature.Insert(999.993);
		temperature.Insert(999.994); temperature.Insert(999.994); temperature.Insert(999.995); temperature.Insert(999.995); temperature.Insert(999.995);
		temperature.Insert(999.996); temperature.Insert(999.996); temperature.Insert(999.996); temperature.Insert(999.997); temperature.Insert(999.997);
		temperature.Insert(999.997); temperature.Insert(999.997); temperature.Insert(999.997); temperature.Insert(999.998); temperature.Insert(999.998);
		temperature.Insert(999.998); temperature.Insert(999.998); temperature.Insert(999.998); temperature.Insert(999.998); temperature.Insert(999.999);
		temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999);
		temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999);
		temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999); temperature.Insert(999.999);
		temperature.Insert(1000.00); temperature.Insert(1000.00); temperature.Insert(1000.00); temperature.Insert(1000.00); temperature.Insert(1000.00);
		temperature.Insert(1000.00);
	}

	// =====================================================================
	// PRESSURE (Pa) - 201 entries, split into 4 chunks
	// =====================================================================
	private static void InitPressure_A()
	{
		pressure = {};
		pressure.Insert(101320.0);      pressure.Insert(54048.0);       pressure.Insert(26500.0);       pressure.Insert(12112.0);       pressure.Insert(5529.3);
		pressure.Insert(2549.2);        pressure.Insert(1197.0);        pressure.Insert(574.59);        pressure.Insert(287.14);        pressure.Insert(149.10);
		pressure.Insert(79.779);        pressure.Insert(42.525);        pressure.Insert(21.959);        pressure.Insert(10.930);        pressure.Insert(5.2209);
		pressure.Insert(2.3881);        pressure.Insert(1.0525);        pressure.Insert(0.44568);       pressure.Insert(0.18435);       pressure.Insert(0.075775);
		pressure.Insert(0.032012);      pressure.Insert(0.014423);      pressure.Insert(0.0071493);     pressure.Insert(0.0040037);     pressure.Insert(0.0025366);
		pressure.Insert(0.0017360);     pressure.Insert(0.0012503);     pressure.Insert(0.00093569);    pressure.Insert(0.00072029);    pressure.Insert(0.00056690);
		pressure.Insert(0.00045422);    pressure.Insert(0.00036929);    pressure.Insert(0.00030394);    pressure.Insert(0.00025277);    pressure.Insert(0.00021210);
		pressure.Insert(0.00017936);    pressure.Insert(0.00015272);    pressure.Insert(0.00013081);    pressure.Insert(0.00011265);    pressure.Insert(0.000097489);
		pressure.Insert(0.000084736);   pressure.Insert(0.000073943);   pressure.Insert(0.000064757);   pressure.Insert(0.000056902);   pressure.Insert(0.000050150);
		pressure.Insert(0.000044321);   pressure.Insert(0.000039270);   pressure.Insert(0.000034879);   pressure.Insert(0.000031051);   pressure.Insert(0.000027701);
	}
	private static void InitPressure_B()
	{
		pressure.Insert(0.000024762);   pressure.Insert(0.000022176);   pressure.Insert(0.000019894);   pressure.Insert(0.000017875);   pressure.Insert(0.000016084);
		pressure.Insert(0.000014494);   pressure.Insert(0.000013078);   pressure.Insert(0.000011815);   pressure.Insert(0.000010686);   pressure.Insert(0.0000096760);
		pressure.Insert(0.0000087704);  pressure.Insert(0.0000079571);  pressure.Insert(0.0000072259);  pressure.Insert(0.0000065678);  pressure.Insert(0.0000059748);
		pressure.Insert(0.0000054400);  pressure.Insert(0.0000049571);  pressure.Insert(0.0000045208);  pressure.Insert(0.0000041260);  pressure.Insert(0.0000037686);
		pressure.Insert(0.0000034446);  pressure.Insert(0.0000031507);  pressure.Insert(0.0000028839);  pressure.Insert(0.0000026414);  pressure.Insert(0.0000024208);
		pressure.Insert(0.0000022200);  pressure.Insert(0.0000020370);  pressure.Insert(0.0000018702);  pressure.Insert(0.0000017179);  pressure.Insert(0.0000015789);
		pressure.Insert(0.0000014518);  pressure.Insert(0.0000013356);  pressure.Insert(0.0000012292);  pressure.Insert(0.0000011319);  pressure.Insert(0.0000010428);
		pressure.Insert(0.00000096119); pressure.Insert(0.00000088642); pressure.Insert(0.00000081789); pressure.Insert(0.00000075505); pressure.Insert(0.00000069741);
		pressure.Insert(0.00000064452); pressure.Insert(0.00000059597); pressure.Insert(0.00000055139); pressure.Insert(0.00000051043); pressure.Insert(0.00000047279);
		pressure.Insert(0.00000043818); pressure.Insert(0.00000040634); pressure.Insert(0.00000037705); pressure.Insert(0.00000035008); pressure.Insert(0.00000032524);
	}
	private static void InitPressure_C()
	{
		pressure.Insert(0.00000030235); pressure.Insert(0.00000028126); pressure.Insert(0.00000026180); pressure.Insert(0.00000024386); pressure.Insert(0.00000022730);
		pressure.Insert(0.00000021202); pressure.Insert(0.00000019790); pressure.Insert(0.00000018486); pressure.Insert(0.00000017281); pressure.Insert(0.00000016168);
		pressure.Insert(0.00000015141); pressure.Insert(0.00000014184); pressure.Insert(0.00000013302); pressure.Insert(0.00000012483); pressure.Insert(0.00000011722);
		pressure.Insert(0.00000011015); pressure.Insert(0.00000010358); pressure.Insert(0.000000097460); pressure.Insert(0.000000091750); pressure.Insert(0.000000086438);
		pressure.Insert(0.000000081480); pressure.Insert(0.000000076851); pressure.Insert(0.000000072524); pressure.Insert(0.000000068481); pressure.Insert(0.000000064702);
		pressure.Insert(0.000000061171); pressure.Insert(0.000000057870); pressure.Insert(0.000000054785); pressure.Insert(0.000000051901); pressure.Insert(0.000000049207);
		pressure.Insert(0.000000046690); pressure.Insert(0.000000044340); pressure.Insert(0.000000042148); pressure.Insert(0.000000040101); pressure.Insert(0.000000038191);
		pressure.Insert(0.000000036409); pressure.Insert(0.000000034746); pressure.Insert(0.000000033193); pressure.Insert(0.000000031743); pressure.Insert(0.000000030390);
		pressure.Insert(0.000000029127); pressure.Insert(0.000000027948); pressure.Insert(0.000000026847); pressure.Insert(0.000000025818); pressure.Insert(0.000000024855);
		pressure.Insert(0.000000023952); pressure.Insert(0.000000023106); pressure.Insert(0.000000022312); pressure.Insert(0.000000021566); pressure.Insert(0.000000020866);
	}
	private static void InitPressure_D()
	{
		pressure.Insert(0.000000020207); pressure.Insert(0.000000019587); pressure.Insert(0.000000019004); pressure.Insert(0.000000018455); pressure.Insert(0.000000017939);
		pressure.Insert(0.000000017452); pressure.Insert(0.000000016993); pressure.Insert(0.000000016560); pressure.Insert(0.000000016151); pressure.Insert(0.000000015764);
		pressure.Insert(0.000000015398); pressure.Insert(0.000000015051); pressure.Insert(0.000000014723); pressure.Insert(0.000000014412); pressure.Insert(0.000000014117);
		pressure.Insert(0.000000013837); pressure.Insert(0.000000013572); pressure.Insert(0.000000013320); pressure.Insert(0.000000013080); pressure.Insert(0.000000012852);
		pressure.Insert(0.000000012635); pressure.Insert(0.000000012428); pressure.Insert(0.000000012231); pressure.Insert(0.000000012043); pressure.Insert(0.000000011863);
		pressure.Insert(0.000000011692); pressure.Insert(0.000000011528); pressure.Insert(0.000000011371); pressure.Insert(0.000000011221); pressure.Insert(0.000000011077);
		pressure.Insert(0.000000010940); pressure.Insert(0.000000010808); pressure.Insert(0.000000010683); pressure.Insert(0.000000010562); pressure.Insert(0.000000010447);
		pressure.Insert(0.000000010336); pressure.Insert(0.0000000101300); pressure.Insert(0.0000000099152); pressure.Insert(0.0000000097059); pressure.Insert(0.0000000094999);
		pressure.Insert(0.0000000093000); pressure.Insert(0.0000000091013); pressure.Insert(0.0000000089317); pressure.Insert(0.0000000087596); pressure.Insert(0.0000000085899);
		pressure.Insert(0.0000000084213); pressure.Insert(0.0000000082518); pressure.Insert(0.0000000080751); pressure.Insert(0.0000000079311); pressure.Insert(0.0000000077513);
		pressure.Insert(0.0000000075138);
	}

	// =====================================================================
	// DENSITY (kg/m^3) - 201 entries, split into 4 chunks
	// =====================================================================
	private static void InitDensity_A()
	{
		density = {};
		density.Insert(1.22500);       density.Insert(0.73643);       density.Insert(0.41351);       density.Insert(0.19476);       density.Insert(0.088910);
		density.Insert(0.040084);      density.Insert(0.018410);      density.Insert(0.0084634);     density.Insert(0.0039957);     density.Insert(0.0019663);
		density.Insert(0.0010269);     density.Insert(0.00056810);    density.Insert(0.00030968);    density.Insert(0.00016321);    density.Insert(0.000082829);
		density.Insert(0.000039921);   density.Insert(0.000018458);   density.Insert(0.0000082195);  density.Insert(0.0000034400);  density.Insert(0.0000013873);
		density.Insert(0.00000056044); density.Insert(0.00000023325); density.Insert(0.000000096734); density.Insert(0.000000042794); density.Insert(0.000000022199);
		density.Insert(0.000000012918); density.Insert(0.0000000081494); density.Insert(0.0000000054647); density.Insert(0.0000000038313); density.Insert(0.0000000027805);
		density.Insert(0.0000000020752); density.Insert(0.0000000015848); density.Insert(0.0000000012336); density.Insert(0.00000000097526); density.Insert(0.00000000078155);
		density.Insert(0.00000000063382); density.Insert(0.00000000051940); density.Insert(0.00000000042952); density.Insert(0.00000000035807); density.Insert(0.00000000030064);
		density.Insert(0.00000000025407); density.Insert(0.00000000021596); density.Insert(0.00000000018456); density.Insert(0.00000000015849); density.Insert(0.00000000013671);
		density.Insert(0.00000000011839); density.Insert(0.00000000010290); density.Insert(0.000000000089757); density.Insert(0.000000000078550); density.Insert(0.000000000068954);
	}
	private static void InitDensity_B()
	{
		density.Insert(0.000000000060706); density.Insert(0.000000000053587); density.Insert(0.000000000047420); density.Insert(0.000000000042058); density.Insert(0.000000000037382);
		density.Insert(0.000000000033294); density.Insert(0.000000000029710); density.Insert(0.000000000026560); density.Insert(0.000000000023783); density.Insert(0.000000000021331);
		density.Insert(0.000000000019159); density.Insert(0.000000000017232); density.Insert(0.000000000015519); density.Insert(0.000000000013994); density.Insert(0.000000000012634);
		density.Insert(0.000000000011419); density.Insert(0.000000000010333); density.Insert(0.0000000000093610); density.Insert(0.0000000000084890); density.Insert(0.0000000000077050);
		density.Insert(0.0000000000070010); density.Insert(0.0000000000063670); density.Insert(0.0000000000057950); density.Insert(0.0000000000052790); density.Insert(0.0000000000048130);
		density.Insert(0.0000000000043910); density.Insert(0.0000000000040090); density.Insert(0.0000000000036630); density.Insert(0.0000000000033480); density.Insert(0.0000000000030630);
		density.Insert(0.0000000000028030); density.Insert(0.0000000000025660); density.Insert(0.0000000000023510); density.Insert(0.0000000000021540); density.Insert(0.0000000000019750);
		density.Insert(0.0000000000018120); density.Insert(0.0000000000016630); density.Insert(0.0000000000015270); density.Insert(0.0000000000014020); density.Insert(0.0000000000012880);
		density.Insert(0.0000000000011840); density.Insert(0.0000000000010890); density.Insert(0.0000000000010020); density.Insert(0.00000000000092200); density.Insert(0.00000000000084900);
		density.Insert(0.00000000000078200); density.Insert(0.00000000000072100); density.Insert(0.00000000000066400); density.Insert(0.00000000000061300); density.Insert(0.00000000000056500);
	}
	private static void InitDensity_C()
	{
		density.Insert(0.00000000000052100); density.Insert(0.00000000000048100); density.Insert(0.00000000000044500); density.Insert(0.00000000000041100); density.Insert(0.00000000000037900);
		density.Insert(0.00000000000035100); density.Insert(0.00000000000032400); density.Insert(0.00000000000030000); density.Insert(0.00000000000027800); density.Insert(0.00000000000025700);
		density.Insert(0.00000000000023800); density.Insert(0.00000000000022100); density.Insert(0.00000000000020500); density.Insert(0.00000000000019000); density.Insert(0.00000000000017600);
		density.Insert(0.00000000000016400); density.Insert(0.00000000000015200); density.Insert(0.00000000000014100); density.Insert(0.00000000000013100); density.Insert(0.00000000000012200);
		density.Insert(0.00000000000011400); density.Insert(0.00000000000010600); density.Insert(0.000000000000099000); density.Insert(0.000000000000092000); density.Insert(0.000000000000086000);
		density.Insert(0.000000000000080000); density.Insert(0.000000000000075000); density.Insert(0.000000000000070000); density.Insert(0.000000000000065000); density.Insert(0.000000000000061000);
		density.Insert(0.000000000000057000); density.Insert(0.000000000000053000); density.Insert(0.000000000000050000); density.Insert(0.000000000000047000); density.Insert(0.000000000000044000);
		density.Insert(0.000000000000041000); density.Insert(0.000000000000039000); density.Insert(0.000000000000037000); density.Insert(0.000000000000035000); density.Insert(0.000000000000033000);
		density.Insert(0.000000000000031000); density.Insert(0.000000000000029000); density.Insert(0.000000000000027000); density.Insert(0.000000000000026000); density.Insert(0.000000000000024000);
		density.Insert(0.000000000000023000); density.Insert(0.000000000000022000); density.Insert(0.000000000000021000); density.Insert(0.000000000000020000); density.Insert(0.000000000000019000);
	}
	private static void InitDensity_D()
	{
		density.Insert(0.000000000000018000); density.Insert(0.000000000000017000); density.Insert(0.000000000000016000); density.Insert(0.000000000000015000); density.Insert(0.000000000000015000);
		density.Insert(0.000000000000014000); density.Insert(0.000000000000013000); density.Insert(0.000000000000013000); density.Insert(0.000000000000012000); density.Insert(0.000000000000012000);
		density.Insert(0.000000000000011000); density.Insert(0.000000000000011000); density.Insert(0.000000000000010000); density.Insert(0.000000000000010000); density.Insert(0.000000000000010000);
		density.Insert(0.0000000000000090000); density.Insert(0.0000000000000090000); density.Insert(0.0000000000000090000); density.Insert(0.0000000000000080000); density.Insert(0.0000000000000080000);
		density.Insert(0.0000000000000080000); density.Insert(0.0000000000000080000); density.Insert(0.0000000000000070000); density.Insert(0.0000000000000070000); density.Insert(0.0000000000000070000);
		density.Insert(0.0000000000000070000); density.Insert(0.0000000000000060000); density.Insert(0.0000000000000060000); density.Insert(0.0000000000000060000); density.Insert(0.0000000000000060000);
		density.Insert(0.0000000000000060000); density.Insert(0.0000000000000060000); density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000);
		density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000); density.Insert(0.0000000000000050000);
		density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000);
		density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000); density.Insert(0.0000000000000040000);
		density.Insert(0.0000000000000040000);
	}

	// =====================================================================
	// SPEED OF SOUND (m/s) - 201 entries, split into 4 chunks
	// =====================================================================
	private static void InitSpeedOfSound_A()
	{
		speed_of_sound = {};
		speed_of_sound.Insert(340.29); speed_of_sound.Insert(320.55); speed_of_sound.Insert(299.53); speed_of_sound.Insert(295.07); speed_of_sound.Insert(295.07);
		speed_of_sound.Insert(298.39); speed_of_sound.Insert(301.71); speed_of_sound.Insert(308.30); speed_of_sound.Insert(317.19); speed_of_sound.Insert(325.82);
		speed_of_sound.Insert(329.80); speed_of_sound.Insert(323.72); speed_of_sound.Insert(315.07); speed_of_sound.Insert(306.19); speed_of_sound.Insert(297.06);
		speed_of_sound.Insert(289.40); speed_of_sound.Insert(282.54); speed_of_sound.Insert(275.52); speed_of_sound.Insert(274.04); speed_of_sound.Insert(275.17);
		speed_of_sound.Insert(280.00); speed_of_sound.Insert(289.70); speed_of_sound.Insert(310.56); speed_of_sound.Insert(347.22); speed_of_sound.Insert(380.36);
		speed_of_sound.Insert(409.48); speed_of_sound.Insert(434.27); speed_of_sound.Insert(455.64); speed_of_sound.Insert(474.24); speed_of_sound.Insert(490.54);
		speed_of_sound.Insert(504.92); speed_of_sound.Insert(517.66); speed_of_sound.Insert(528.98); speed_of_sound.Insert(539.08); speed_of_sound.Insert(548.11);
		speed_of_sound.Insert(556.21); speed_of_sound.Insert(563.48); speed_of_sound.Insert(570.02); speed_of_sound.Insert(575.91); speed_of_sound.Insert(581.22);
		speed_of_sound.Insert(586.02); speed_of_sound.Insert(590.36); speed_of_sound.Insert(594.29); speed_of_sound.Insert(597.85); speed_of_sound.Insert(601.07);
		speed_of_sound.Insert(604.00); speed_of_sound.Insert(606.65); speed_of_sound.Insert(609.06); speed_of_sound.Insert(611.26); speed_of_sound.Insert(613.25);
	}
	private static void InitSpeedOfSound_B()
	{
		speed_of_sound.Insert(615.06); speed_of_sound.Insert(616.71); speed_of_sound.Insert(618.21); speed_of_sound.Insert(619.57); speed_of_sound.Insert(620.82);
		speed_of_sound.Insert(621.95); speed_of_sound.Insert(622.98); speed_of_sound.Insert(623.93); speed_of_sound.Insert(624.79); speed_of_sound.Insert(625.57);
		speed_of_sound.Insert(626.28); speed_of_sound.Insert(626.94); speed_of_sound.Insert(627.53); speed_of_sound.Insert(628.08); speed_of_sound.Insert(628.57);
		speed_of_sound.Insert(629.03); speed_of_sound.Insert(629.44); speed_of_sound.Insert(629.82); speed_of_sound.Insert(630.17); speed_of_sound.Insert(630.49);
		speed_of_sound.Insert(630.78); speed_of_sound.Insert(631.04); speed_of_sound.Insert(631.28); speed_of_sound.Insert(631.50); speed_of_sound.Insert(631.71);
		speed_of_sound.Insert(631.89); speed_of_sound.Insert(632.06); speed_of_sound.Insert(632.22); speed_of_sound.Insert(632.36); speed_of_sound.Insert(632.49);
		speed_of_sound.Insert(632.61); speed_of_sound.Insert(632.72); speed_of_sound.Insert(632.82); speed_of_sound.Insert(632.91); speed_of_sound.Insert(633.00);
		speed_of_sound.Insert(633.07); speed_of_sound.Insert(633.14); speed_of_sound.Insert(633.21); speed_of_sound.Insert(633.27); speed_of_sound.Insert(633.32);
		speed_of_sound.Insert(633.37); speed_of_sound.Insert(633.42); speed_of_sound.Insert(633.46); speed_of_sound.Insert(633.50); speed_of_sound.Insert(633.53);
		speed_of_sound.Insert(633.57); speed_of_sound.Insert(633.60); speed_of_sound.Insert(633.62); speed_of_sound.Insert(633.65); speed_of_sound.Insert(633.67);
	}
	private static void InitSpeedOfSound_C()
	{
		speed_of_sound.Insert(633.69); speed_of_sound.Insert(633.71); speed_of_sound.Insert(633.73); speed_of_sound.Insert(633.75); speed_of_sound.Insert(633.76);
		speed_of_sound.Insert(633.78); speed_of_sound.Insert(633.79); speed_of_sound.Insert(633.80); speed_of_sound.Insert(633.81); speed_of_sound.Insert(633.82);
		speed_of_sound.Insert(633.83); speed_of_sound.Insert(633.84); speed_of_sound.Insert(633.85); speed_of_sound.Insert(633.85); speed_of_sound.Insert(633.86);
		speed_of_sound.Insert(633.87); speed_of_sound.Insert(633.87); speed_of_sound.Insert(633.88); speed_of_sound.Insert(633.88); speed_of_sound.Insert(633.88);
		speed_of_sound.Insert(633.89); speed_of_sound.Insert(633.89); speed_of_sound.Insert(633.90); speed_of_sound.Insert(633.90); speed_of_sound.Insert(633.90);
		speed_of_sound.Insert(633.90); speed_of_sound.Insert(633.91); speed_of_sound.Insert(633.91); speed_of_sound.Insert(633.91); speed_of_sound.Insert(633.91);
		speed_of_sound.Insert(633.91); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92);
		speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.92); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
	}
	private static void InitSpeedOfSound_D()
	{
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93);
		speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.93); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94); speed_of_sound.Insert(633.94);
		speed_of_sound.Insert(633.94);
	}

	// =====================================================================
	// Master lazy-init — called once on first GetValue() use
	// =====================================================================
	private static void Initialize()
	{
		InitAltitude_A();     InitAltitude_B();     InitAltitude_C();     InitAltitude_D();
		InitTemperature_A();  InitTemperature_B();  InitTemperature_C();  InitTemperature_D();
		InitPressure_A();     InitPressure_B();     InitPressure_C();     InitPressure_D();
		InitDensity_A();      InitDensity_B();      InitDensity_C();      InitDensity_D();
		InitSpeedOfSound_A(); InitSpeedOfSound_B(); InitSpeedOfSound_C(); InitSpeedOfSound_D();
		m_bInitialized = true;
	}

	// =====================================================================
	// Public API — identical to original, only prefixes changed to ADM_
	// lookupAltitude is in meters
	// =====================================================================
	static float GetValue(float lookupAltitude, ADM_ISAProperties property)
	{
		if (!m_bInitialized)
			Initialize();

		float returnValue = -999999;
		lookupAltitude /= 1000;

		switch (property)
		{
			case ADM_ISAProperties.Temperature:
			{
				returnValue = ADM_InternationalStandardAtmosphere.Interpolate(altitude, temperature, lookupAltitude, 5);
				break;
			}
			case ADM_ISAProperties.Pressure:
			{
				returnValue = ADM_InternationalStandardAtmosphere.Interpolate(altitude, pressure, lookupAltitude, 5);
				break;
			}
			case ADM_ISAProperties.Density:
			{
				returnValue = ADM_InternationalStandardAtmosphere.Interpolate(altitude, density, lookupAltitude, 5);
				break;
			}
			case ADM_ISAProperties.SpeedOfSound:
			{
				returnValue = ADM_InternationalStandardAtmosphere.Interpolate(altitude, speed_of_sound, lookupAltitude, 5);
				break;
			}
			default:
			{
				Print("Unsupported ISA Property", LogLevel.ERROR);
				break;
			}
		}

		return returnValue;
	}

	static float Interpolate(array<float> lookupArray, array<float> returnValueArray, float lookupValue, float delta)
	{
		float returnValue = -float.MAX;
		if (lookupArray.Count() != returnValueArray.Count())
		{
			Print("Lookup array and return value array are not the same length!", LogLevel.ERROR);
			return returnValue;
		}

		int lookupArrayIndex = Math.Floor(lookupValue / delta);
		if (lookupArrayIndex < 0)
			lookupArrayIndex = 0;

		if (lookupArrayIndex >= lookupArray.Count() - 1)
			lookupArrayIndex = lookupArray.Count() - 2;

		float value1 = returnValueArray[lookupArrayIndex];
		float value2 = returnValueArray[lookupArrayIndex + 1];
		returnValue = (value2 - value1) / delta * (lookupValue - lookupArray[lookupArrayIndex]) + value1;

		return returnValue;
	}

	// https://doc.comsol.com/5.5/doc/com.comsol.help.cfd/cfd_ug_fluidflow_high_mach.08.27.html
	static float GetDynamicViscosity(float lookupAltitude)
	{
		float T = ADM_InternationalStandardAtmosphere.GetValue(lookupAltitude, ADM_ISAProperties.Temperature);

		float Su  = 111;      // [K]
		float T0  = 273;      // [K]
		float mu0 = 1.716e-5; // [Pa*s]
		float muRatio = Math.Pow(T / T0, 3 / 2) * (T0 + Su) / (T + Su);

		return muRatio * mu0;
	}
}
