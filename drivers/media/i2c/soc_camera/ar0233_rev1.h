/*
 * ON Semiconductor AR0233 sensor camera wizard 2048x1280@30/BGGR/MIPI
 *
 * Copyright (C) 2019 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

static const struct ar0xxx_reg ar0233_rev1_Reset[] = {
{0x301A, 0x0018},	// Stream off and setup MIPI
{AR_DELAY, 200},
{0x3070, 0x0000},	//  1: Solid color test pattern,
			//  2: Full color bar test pattern,
			//  3: Fade to grey color bar test pattern,
			//256: Walking 1 test pattern (12 bit)
{0x3072, 0x0123},	// R
{0x3074, 0x0456},	// G(GR row)
{0x3076, 0x0abc},	// B
{0x3078, 0x0def},	// G(GB row)
#ifdef AR0233_DISPLAY_PATTERN_FIXED
{0x3070, 0x0001},
#endif
#ifdef AR0233_DISPLAY_PATTERN_COLOR_BAR
{0x3070, 0x0002},
#endif
{AR_DELAY, 100},
{ }
}; /* Reset */

static const struct ar0xxx_reg ar0233_rev1_Sequencer_Settings[] = {
/* Design_recommended_settings_v5 */
{0x356C, 0xEA55}, //mte.Sensor.Register("DAC_LD_108_109").Value = 0xEA55& -- ADC write Memory delay 7
{0x3566, 0x2407}, //mte.Sensor.Register("DAC_LD_102_103").Value = 0x2407&  -- Enable column amp bypass for 1x
{0x3562, 0x1C08}, //mte.Sensor.Register("DAC_LD_98_99").Value = 0x1C08&  -- Increase column amp current
{0x3180, 0x1001}, //mte.Sensor.Register("DELTA_DK_CONTROL").Value = 0x1001& -- enable dither
{0x3546, 0x4601}, //MTE.Sensor.Register("DAC_LD_70_71::ANA_SREG_VLN_CURR").Value = 17   -- VLN curr
                  //Sensor.Register(sbit_Dac_Ld_70_71_Ana_Sreg_Ae_Shs_Clamp_En).value = 0 ---- Eclipse
{0x3548, 0x4141}, //MTE.Sensor.Register("DAC_LD_72_73::ANA_SREG_AE_SHR_HCG1").value = 65; //MTE.Sensor.Register("DAC_LD_72_73::ANA_SREG_AE_SHR_HCG0").value = 65
{0x354A, 0x5958}, //MTE.Sensor.Register("DAC_LD_74_75::ANA_SREG_AE_SHR_HCG3TO7").value = 88; //MTE.Sensor.Register("DAC_LD_74_75::ANA_SREG_AE_SHR_HCG2").value = 89
{0x3542, 0x44F0}, //MTE.Sensor.Register(sbit_Dac_Ld_66_67_Ana_Drstlo_Sel_Hcg_Lg_3_0).Value = 4, MTE.Sensor.Register(sbit_Dac_Ld_66_67_Ana_Drstlo_Sel_Hcg_Hg_3_0).Value = 4

// Boosters_Hi_change settings for reduction in DSNU and hot pixels
{0x3518, 0x4444}, //drstlo_lcg_lg(4), drstlo_hcg_lg(4), drstlo_lcg_hg(4), drstlo_hcg_hg(4)
{0x3540, 0x44}, //drstlo_lcg_lg_3_0(4), lcg_hg_3_0(4)
{0x3536, 0x9898}, //booster_ref_Vaa rsthi(1), dcghi(1)
{0x3538, 0x981A}, //booster_ref_vaa rshi(1)
{0x3530, 0x5F98}, //Boost_ref_Vaa Wellhi(1)
{0x353C, 0x9A0A}, //Boost_ref_Vaa lfm_dcghi(1)
{0x3526, 0x9000}, //DWellhi(16)
{0x352E, 0x90D}, //Dlfm_Dcghi(13),(Dlfm_Txhi_Buffer) = 9
/* Design_recommended_settings_v5 */

/* Pixel_char_recommended_settings_v2 */
//TXLO @HCG
{0x3514, 0x555B}, //-0.85V
{0x3578, 0x555B}, //-0.85V
//TXLO @LCG
{0x3514, 0x5B5B}, //-0.85V
{0x3578, 0x5B5B}, //-0.85V
//TXHI
{0x3528, 0xE018}, //2.8V
//RSHI
{0x352A, 0x1533},//3.36V
//DDCGHI
{0x3528, 0xEB0D}, //Ddcghi(13), txhi(11)
//DRSTHI, DRSHI
{0x352A,  0xA27}, //Drsthi (10), Drshi(7)
/* Pixel_char_recommended_settings_v2 */

/* Sequencer_LFM_HDR_v6 */
{0x2512, 0x8000},
{0x2510, 0x070f},
{0x2510, 0x1011},
{0x2510, 0x1216},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0x191b},
{0x2510, 0x2123},
{0x2510, 0x2528},
{0x2510, 0xffff},
{0x2510, 0x2e4a},
{0x2510, 0x5874},
{0x2510, 0x8187},
{0x2510, 0x8b93},
{0x2510, 0x9496},
{0x2510, 0xa1a9},
{0x2510, 0xaaad},
{0x2510, 0xb1b5},
{0x2510, 0xb9bb},
{0x2510, 0xbdff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xc003},
{0x2510, 0x8058},
{0x2510, 0xa0e0},
{0x2510, 0x3041},
{0x2510, 0x3042},
{0x2510, 0x2000},
{0x2510, 0x3048},
{0x2510, 0x3088},
{0x2510, 0x30a0},
{0x2510, 0x3090},
{0x2510, 0xa0c0},
{0x2510, 0x9008},
{0x2510, 0x8802},
{0x2510, 0x20ff},
{0x2510, 0x20ff},
{0x2510, 0x20ff},
{0x2510, 0x20ff},
{0x2510, 0x20ff},
{0x2510, 0x9018},
{0x2510, 0x891a},
{0x2510, 0x807c},
{0x2510, 0x20ff},
{0x2510, 0x895b},
{0x2510, 0x20ff},
{0x2510, 0x897b},
{0x2510, 0x20ff},
{0x2510, 0x897f},
{0x2510, 0x20ff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x20ff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x20ff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x20ff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3081},
{0x2510, 0x3082},
{0x2510, 0xa0c4},
{0x2510, 0x20ff},
{0x2510, 0x8058},
{0x2510, 0x9039},
{0x2510, 0x20ff},
{0x2510, 0x907f},
{0x2510, 0x895b},
{0x2510, 0x2064},
{0x2510, 0x891b},
{0x2510, 0x2010},
{0x2510, 0x8803},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x20ff},
{0x2510, 0x906b},
{0x2510, 0x2064},
{0x2510, 0x3084},
{0x2510, 0x2003},
{0x2510, 0x3044},
{0x2510, 0x2000},
{0x2510, 0xa004},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2400},
{0x2510, 0x2401},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2400},
{0x2510, 0x2401},
{0x2510, 0x2702},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2421},
{0x2510, 0x2703},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2421},
{0x2510, 0x2704},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2421},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2402},
{0x2510, 0x2403},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2741},
{0x2510, 0x2429},
{0x2510, 0x2740},
{0x2510, 0x242a},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2404},
{0x2510, 0x2779},
{0x2510, 0x242c},
{0x2510, 0x2781},
{0x2510, 0x242d},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x3108},
{0x2510, 0x2703},
{0x2510, 0x2432},
{0x2510, 0x2703},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x27bb},
{0x2510, 0x2430},
{0x2510, 0x27bb},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2702},
{0x2510, 0x2431},
{0x2510, 0x2702},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x27c3},
{0x2510, 0x2430},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xb800},
{0x2510, 0x8058},
{0x2510, 0xa005},
{0x2510, 0x3101},
{0x2510, 0x3041},
{0x2510, 0x3104},
{0x2510, 0xb035},
{0x2510, 0xb075},
{0x2510, 0x30c1},
{0x2510, 0x3102},
{0x2510, 0x3041},
{0x2510, 0x3202},
{0x2510, 0xb848},
{0x2510, 0xb84c},
{0x2510, 0x2200},
{0x2510, 0x3141},
{0x2510, 0x3042},
{0x2510, 0xb377},
{0x2510, 0x8843},
{0x2510, 0x916f},
{0x2510, 0x3110},
{0x2510, 0x3042},
{0x2510, 0xb84e},
{0x2510, 0xf905},
{0x2510, 0xf907},
{0x2510, 0x2200},
{0x2510, 0x885b},
{0x2510, 0xa898},
{0x2510, 0xa8d8},
{0x2510, 0xf8e8},
{0x2510, 0x80dc},
{0x2510, 0x9007},
{0x2510, 0x916f},
{0x2510, 0x2206},
{0x2510, 0xb808},
{0x2510, 0xc800},
{0x2510, 0xe809},
{0x2510, 0x88df},
{0x2510, 0xf8a8},
{0x2510, 0xf888},
{0x2510, 0x2203},
{0x2510, 0xb07b},
{0x2510, 0x2000},
{0x2510, 0x80cc},
{0x2510, 0x808c},
{0x2510, 0x220b},
{0x2510, 0xb06a},
{0x2510, 0x88cf},
{0x2510, 0x888f},
{0x2510, 0x2224},
{0x2510, 0xb04a},
{0x2510, 0x2218},
{0x2510, 0x2116},
{0x2510, 0x902f},
{0x2510, 0xb04b},
{0x2510, 0xf880},
{0x2510, 0x2217},
{0x2510, 0x2204},
{0x2510, 0xb043},
{0x2510, 0xa8c9},
{0x2510, 0x31c1},
{0x2510, 0x80ac},
{0x2510, 0x2205},
{0x2510, 0x916f},
{0x2510, 0x2104},
{0x2510, 0x88af},
{0x2510, 0x2440},
{0x2510, 0xf110},
{0x2510, 0xf804},
{0x2510, 0x2000},
{0x2510, 0x8088},
{0x2510, 0x3002},
{0x2510, 0xb838},
{0x2510, 0xa8c8},
{0x2510, 0xb04b},
{0x2510, 0x2442},
{0x2510, 0x3210},
{0x2510, 0x2206},
{0x2510, 0x888b},
{0x2510, 0x2441},
{0x2510, 0x3202},
{0x2510, 0xf108},
{0x2510, 0xf0d7},
{0x2510, 0xb830},
{0x2510, 0xf880},
{0x2510, 0xc801},
{0x2510, 0x30c2},
{0x2510, 0xe80c},
{0x2510, 0x2201},
{0x2510, 0xb04a},
{0x2510, 0x222d},
{0x2510, 0x3241},
{0x2510, 0x2207},
{0x2510, 0x902f},
{0x2510, 0x2214},
{0x2510, 0x2204},
{0x2510, 0xb042},
{0x2510, 0xa9a1},
{0x2510, 0x8008},
{0x2510, 0xb093},
{0x2510, 0x31c1},
{0x2510, 0x916b},
{0x2510, 0x2009},
{0x2510, 0x8803},
{0x2510, 0xa044},
{0x2510, 0x3044},
{0x2510, 0x2000},
{0x2510, 0xa004},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xa084},
{0x2510, 0x30d0},
{0x2510, 0x8078},
{0x2510, 0x3141},
{0x2510, 0x3041},
{0x2510, 0x3042},
{0x2510, 0x2000},
{0x2510, 0x3142},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3110},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3120},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3144},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3148},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x2206},
{0x2510, 0x881b},
{0x2510, 0x887b},
{0x2510, 0xa08c},
{0x2510, 0x221f},
{0x2510, 0xa084},
{0x2510, 0x2440},
{0x2510, 0xb095},
{0x2510, 0xf110},
{0x2510, 0xf864},
{0x2510, 0xf90d},
{0x2510, 0x3084},
{0x2510, 0x3090},
{0x2510, 0x3088},
{0x2510, 0x8058},
{0x2510, 0x3001},
{0x2510, 0x2442},
{0x2510, 0x3220},
{0x2510, 0x2002},
{0x2510, 0x8863},
{0x2510, 0x2004},
{0x2510, 0x8803},
{0x2510, 0x2441},
{0x2510, 0x30c2},
{0x2510, 0xa9a0},
{0x2510, 0xb094},
{0x2510, 0x2201},
{0x2510, 0xa0c4},
{0x2510, 0x3044},
{0x2510, 0x2000},
{0x2510, 0xa004},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xb980},
{0x2510, 0x8108},
{0x2510, 0xa105},
{0x2510, 0x30c1},
{0x2510, 0x2000},
{0x2510, 0x3101},
{0x2510, 0x3041},
{0x2510, 0x3104},
{0x2510, 0x3102},
{0x2510, 0x3041},
{0x2510, 0xf860},
{0x2510, 0xb095},
{0x2510, 0x3141},
{0x2510, 0x3042},
{0x2510, 0xb9f8},
{0x2510, 0xb9fc},
{0x2510, 0x8803},
{0x2510, 0x916f},
{0x2510, 0x3110},
{0x2510, 0x3042},
{0x2510, 0xb9fe},
{0x2510, 0xf905},
{0x2510, 0xf907},
{0x2510, 0x3202},
{0x2510, 0x880b},
{0x2510, 0xa888},
{0x2510, 0xa8c8},
{0x2510, 0xb397},
{0x2510, 0xf8e8},
{0x2510, 0x818c},
{0x2510, 0x9007},
{0x2510, 0x916f},
{0x2510, 0x2204},
{0x2510, 0xb137},
{0x2510, 0xb9b8},
{0x2510, 0xc801},
{0x2510, 0xe809},
{0x2510, 0xb177},
{0x2510, 0x888f},
{0x2510, 0xf8a8},
{0x2510, 0xf888},
{0x2510, 0x2203},
{0x2510, 0xb07b},
{0x2510, 0x2000},
{0x2510, 0x818c},
{0x2510, 0x808c},
{0x2510, 0x220b},
{0x2510, 0xb06a},
{0x2510, 0x888f},
{0x2510, 0x888f},
{0x2510, 0x2224},
{0x2510, 0xb04a},
{0x2510, 0x2218},
{0x2510, 0x2115},
{0x2510, 0xb04b},
{0x2510, 0x902f},
{0x2510, 0xf880},
{0x2510, 0x2217},
{0x2510, 0x2204},
{0x2510, 0xb043},
{0x2510, 0xa8d9},
{0x2510, 0x31c1},
{0x2510, 0x80cc},
{0x2510, 0x2103},
{0x2510, 0x916f},
{0x2510, 0x2106},
{0x2510, 0x88cf},
{0x2510, 0x2440},
{0x2510, 0xf110},
{0x2510, 0xf804},
{0x2510, 0x2000},
{0x2510, 0x8088},
{0x2510, 0x3002},
{0x2510, 0xb988},
{0x2510, 0xa8d8},
{0x2510, 0xb04b},
{0x2510, 0x2442},
{0x2510, 0x3210},
{0x2510, 0x2206},
{0x2510, 0x888b},
{0x2510, 0x2441},
{0x2510, 0x3202},
{0x2510, 0xf108},
{0x2510, 0xf0d7},
{0x2510, 0xb980},
{0x2510, 0xf880},
{0x2510, 0xc800},
{0x2510, 0x30c2},
{0x2510, 0xe80c},
{0x2510, 0x2201},
{0x2510, 0xb04a},
{0x2510, 0x2230},
{0x2510, 0x3241},
{0x2510, 0x902f},
{0x2510, 0x221b},
{0x2510, 0x2204},
{0x2510, 0xb042},
{0x2510, 0xa9a1},
{0x2510, 0x8058},
{0x2510, 0xb093},
{0x2510, 0x31c1},
{0x2510, 0x916b},
{0x2510, 0x2009},
{0x2510, 0x8803},
{0x2510, 0xa144},
{0x2510, 0x3044},
{0x2510, 0x2000},
{0x2510, 0xa004},
{0x2510, 0xb800},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x8078},
{0x2510, 0x30d0},
{0x2510, 0xa184},
{0x2510, 0xb980},
{0x2510, 0x3141},
{0x2510, 0x3041},
{0x2510, 0x3042},
{0x2510, 0x2000},
{0x2510, 0x3142},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3110},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3120},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x2206},
{0x2510, 0x881b},
{0x2510, 0x887b},
{0x2510, 0x2440},
{0x2510, 0xb095},
{0x2510, 0xf110},
{0x2510, 0xf864},
{0x2510, 0xf90d},
{0x2510, 0x30a0},
{0x2510, 0x3090},
{0x2510, 0x3088},
{0x2510, 0x8058},
{0x2510, 0x3001},
{0x2510, 0x2202},
{0x2510, 0x2442},
{0x2510, 0x3220},
{0x2510, 0x2002},
{0x2510, 0x885b},
{0x2510, 0x2441},
{0x2510, 0x30c2},
{0x2510, 0x8018},
{0x2510, 0x2000},
{0x2510, 0x881b},
{0x2510, 0x2008},
{0x2510, 0x8000},
{0x2510, 0xa9a0},
{0x2510, 0xb094},
{0x2510, 0x2201},
{0x2510, 0x8803},
{0x2510, 0xa1c4},
{0x2510, 0x3044},
{0x2510, 0xb800},
{0x2510, 0xa004},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x9818},
{0x2510, 0x3101},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3102},
{0x2510, 0x3041},
{0x2510, 0x8008},
{0x2510, 0x2002},
{0x2510, 0x8028},
{0x2510, 0x2205},
{0x2510, 0x880b},
{0x2510, 0x882b},
{0x2510, 0x213e},
{0x2510, 0x8008},
{0x2510, 0x2202},
{0x2510, 0x8000},
{0x2510, 0x2202},
{0x2510, 0xa044},
{0x2510, 0x3044},
{0x2510, 0x8803},
{0x2510, 0x9800},
{0x2510, 0xa004},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2440},
{0x2510, 0xb095},
{0x2510, 0xf110},
{0x2510, 0xf864},
{0x2510, 0xf90d},
{0x2510, 0x2442},
{0x2510, 0x3220},
{0x2510, 0x2007},
{0x2510, 0x2441},
{0x2510, 0x30c2},
{0x2510, 0xa9a0},
{0x2510, 0xb094},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xb980},
{0x2510, 0x3101},
{0x2510, 0x3041},
{0x2510, 0x2000},
{0x2510, 0x3102},
{0x2510, 0x3041},
{0x2510, 0x8028},
{0x2510, 0x220a},
{0x2510, 0x880b},
{0x2510, 0x882b},
{0x2510, 0x2440},
{0x2510, 0xb095},
{0x2510, 0xf110},
{0x2510, 0xf864},
{0x2510, 0xf90d},
{0x2510, 0x8008},
{0x2510, 0x3001},
{0x2510, 0x2202},
{0x2510, 0x2442},
{0x2510, 0x8823},
{0x2510, 0x3220},
{0x2510, 0x2000},
{0x2510, 0x8803},
{0x2510, 0x2441},
{0x2510, 0x30c2},
{0x2510, 0xa9a0},
{0x2510, 0xb094},
{0x2510, 0x2201},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2000},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2400},
{0x2510, 0x2751},
{0x2510, 0x2423},
{0x2510, 0x2750},
{0x2510, 0x2421},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2749},
{0x2510, 0x2422},
{0x2510, 0x2749},
{0x2510, 0x2423},
{0x2510, 0x2709},
{0x2510, 0x2420},
{0x2510, 0x2729},
{0x2510, 0x2423},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2722},
{0x2510, 0x2422},
{0x2510, 0x2769},
{0x2510, 0x2421},
{0x2510, 0x2702},
{0x2510, 0x2421},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x276a},
{0x2510, 0x2420},
{0x2510, 0x276a},
{0x2510, 0x2421},
{0x2510, 0x2703},
{0x2510, 0x2420},
{0x2510, 0x2703},
{0x2510, 0x2421},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x276b},
{0x2510, 0x2420},
{0x2510, 0x276b},
{0x2510, 0x2421},
{0x2510, 0x2704},
{0x2510, 0x2420},
{0x2510, 0x2704},
{0x2510, 0x2421},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x276c},
{0x2510, 0x2420},
{0x2510, 0x276c},
{0x2510, 0x2421},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2759},
{0x2510, 0x2422},
{0x2510, 0x2758},
{0x2510, 0x2420},
{0x2510, 0x2403},
{0x2510, 0x2712},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2422},
{0x2510, 0x271a},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2702},
{0x2510, 0x2423},
{0x2510, 0x2703},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2703},
{0x2510, 0x2423},
{0x2510, 0x2704},
{0x2510, 0x3242},
{0x2510, 0x3108},
{0x2510, 0x2420},
{0x2510, 0x2704},
{0x2510, 0x2423},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2400},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xc023},
{0x2510, 0x2402},
{0x2510, 0x2405},
{0x2510, 0x2789},
{0x2510, 0x242e},
{0x2510, 0x2788},
{0x2510, 0x242f},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xc027},
{0x2510, 0x2400},
{0x2510, 0x2406},
{0x2510, 0xc063},
{0x2510, 0x2402},
{0x2510, 0x2751},
{0x2510, 0x2423},
{0x2510, 0x2750},
{0x2510, 0x2421},
{0x2510, 0xc003},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xc021},
{0x2510, 0x2400},
{0x2510, 0x2405},
{0x2510, 0xc062},
{0x2510, 0x2400},
{0x2510, 0xc063},
{0x2510, 0x2751},
{0x2510, 0x2423},
{0x2510, 0x2750},
{0x2510, 0x2421},
{0x2510, 0xc003},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xc0e3},
{0x2510, 0x2400},
{0x2510, 0x27b1},
{0x2510, 0x2425},
{0x2510, 0xc063},
{0x2510, 0x2420},
{0x2510, 0x2751},
{0x2510, 0x2423},
{0x2510, 0x2750},
{0x2510, 0x2421},
{0x2510, 0xc003},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2404},
{0x2510, 0x2779},
{0x2510, 0x242c},
{0x2510, 0x2781},
{0x2510, 0x242d},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x2791},
{0x2510, 0x2430},
{0x2510, 0x2799},
{0x2510, 0x2428},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x27a1},
{0x2510, 0x2430},
{0x2510, 0x27a9},
{0x2510, 0x2428},
{0x2510, 0x3244},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0x7fff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{0x2510, 0xffff},
{AR_DELAY, 100},
/* Sequencer_LFM_HDR_v6 */
{ }
}; /* Sequencer_Settings */

static const struct ar0xxx_reg ar0233_rev1_HDR_3exp_12bit[] = {
{0x3082, 0x8},    //num_exp = 3
{0x3110, 0x11},   //Set bypass pix comb for HDR,Pre_hdr_gain_enable_07Jul
{0x30BA, 0x1122}, //num_exp_max =3
{0x31AC, 0x140C}, //12 bit output
{0x31D0, 0x1}, // Companding

{0x3044, 0x0400}, //Dark_control

// FPS = 103.5MHz / reg0x300A / reg0x300C
{0x300A, AR0233_SENSOR_HEIGHT + 100}, // Frame_length_Lines
{0x300C, AR0233_SENSOR_WIDTH + 400}, // Line_length_pck
{0x3012, 0x144}, //Integration_time
{ }
}; /* HDR_3exp_12bit */

static const struct ar0xxx_reg ar0233_rev1_Serial_12bit_Timing_Setup_103p5[] = {
/* PCLK=DES_REFCLK /PRE_PLL_CLK_DIV *PLL_MULTIPLIER /P1 /P4 */
/* PCLK=23MHz/2 *54/1/6= 103.5Mhz - TI serializers */
{0x3030, 54}, //PLL_MULTIPLIER ; 0x3030 [11:0]
{0x302E, 2}, //PRE_PLL_CLK_DIV ; 0x302E [5:0]
{0x302C, 1}, //P1 divider (vt_sys_clk_div)
{0x302A, 6}, //P2 divider (vt_pix_clk_div); 0x302A [4:0]
{0x3038, 2}, //P3 divider (op_sys_clk_div); 0x3038 [4:0]
{0x3036, 6}, //P4 divider (op_word_clk_div); 0x3036 [4:0]
{0x31DC, 0x1FB0},
{ }
}; /* Serial_12bit_Timing_Setup_103p5 */

static const struct ar0xxx_reg ar0233_rev1_MIPI_4Lane_12BITS[] = {
{0x31AE, 0x204}, //MIPI enable, 4 lanes
{0x31B0, 0x4B}, //frame_preamble
{0x31B2, 0x33}, //line_preamble
{0x31B4, 0x1185}, //mipi_timing_0
{0x31B6, 0x110B}, //mipi_timing_1
{0x31B8, 0x4047}, //mipi_timing_2
{0x31BA, 0x105}, //mipi_timing_3
{0x31BC, 0x704}, //mipi_timing_4
{0x3342, 0x2c2c}, // MIPI_F1_PDT_EDT
{0x3346, 0x2c2c}, // MIPI_F2_PDT_EDT
{0x334A, 0x2c2c}, // MIPI_F3_PDT_EDT
{0x334E, 0x2c2c}, // MIPI_F4_PDT_EDT
{ }
}; /* MIPI_4Lane_12BITS */

static const struct ar0xxx_reg ar0233_rev1_Full_resolution[] = {
{0x3004, AR0233_X_START}, // X_ADDR_START_
{0x3008, AR0233_X_END}, // X_ADDR_END_
{0x3002, AR0233_Y_START}, // Y_ADDR_START_
{0x3006, AR0233_Y_END}, // Y_ADDR_END_
{0x3402, (0x8000 & 0) | AR0233_MAX_WIDTH}, // X_OUTPUT_CONTROL
{0x3404, (0x8000 & 0) | AR0233_MAX_HEIGHT}, // Y_OUTPUT_CONTROL
{ }
}; /* Full_resolution */

static const struct ar0xxx_reg ar0233_rev1_disable_embed_data_stat[] = {
{0x3040, 0xC005}, //Embedded stat4 and data4 rows, hflip/vflip=1
{0x3064, 0x0180}, //Enable embedded data and stat
{ }
}; /* disable_embed_data_stat */

static const struct ar0xxx_reg ar0233_rev1_Gain_3p28x[] = {
{0x3022, 0x01}, // GROUPED_PARAMETER_HOLD_
{0x3362, 0x000F}, // DC_GAIN
{0x3366, 0x1111},
{0x336A, 0x0000},
{0x3022, 0x00}, // GROUPED_PARAMETER_HOLD_
{ }
}; /* Gain_3.28x */

static const struct ar0xxx_reg ar0233_rev1_MEC_DLO_default[] = {
{0x3D00, 0x6F73}, // control
{0x3D02, 0x0033},
{0x3364, 0x068C}, // dcg_trim = 13.1
{0x3D28, 0x09C4}, // weights
{0x3D2A, 0x0DAC},
{0x3D30, 0x0FFF},
{0x3D32, 0x0FFF},
{0x3D34, 0x09C4},
{0x3D36, 0x0DAC},
{0x3D3C, 0x0FFF},
{0x3D3E, 0x0FFF},
{0x3D40, 0x09C4},
{0x3D42, 0x0DAC},
{0x3D48, 0x0FFF},
{0x3D4A, 0x0FFF},
{0x3D4C, 0x0DAC},
{0x3D64, 0x0DAC}, // clip
{0x3D66, 0x0DAC},
{0x3D68, 0x0DAC},
{0x3D6A, 0x0DAC},
{0x3D6C, 0x0DAC},
{0x3D6E, 0x0DAC},
{0x3D70, 0x0DAC},
{0x3D72, 0x0DAC},
{0x3D74, 0x0DAC},
{0x3D76, 0x0DAC},
{0x3D78, 0x0DAC},
{0x3D7A, 0x0DAC},
{0x3D7C, 0x0DAC},
{0x3D7E, 0x0DAC},
{0x3D80, 0x0DAC},
{0x3D82, 0x0DAC},
{0x3D84, 0x0DAC}, // motion_clip
{0x3D86, 0x0DAC},
{0x3D88, 0x0000},
{0x3D8A, 0x0DAC},
{0x3D8C, 0x0DAC},
{0x3D8E, 0x0000},
{0x3DB4, 0x0001}, // motion_q
{0x3DB6, 0x000E},
{0x3DB8, 0x0080},
{0x3DBA, 0x3920}, // t?_s12_k
{0x3DBC, 0x3920},
{0x3DBE, 0x3920},
{0x3DC0, 0x0080}, // wb_gain
{0x3DC2, 0x0080},
{0x3DC4, 0x0080},
{0x3DC6, 0x0080},
{0x3DC8, 0x0000}, // color_th
{0x3DCA, 0x0000},
{0x3DCC, 0x0000},
{0x3DCE, 0x0000},
{0x3DD0, 0x0000},
{0x3DD2, 0x0000},
{0x3DD4, 0x0000},
{0x3DD6, 0x0000},
{0x3DD8, 0x0000},
{0x3DDA, 0x0000},
{0x3DDC, 0x0000},
{0x3DDE, 0x0000},
{0x3DE0, 0x0000},
{0x3290, 0x1B58}, // t3_barrier
{0x3292, 0x1B58},
{0x3294, 0x1B58},
{0x3296, 0x1B58},
{0x3298, 0x2904}, // t4_barrier
{0x329A, 0x2904},
{0x329C, 0x2904},
{0x329E, 0x2904},
{0x32A0, 0x0000},
{0x32A2, 0x0000},
{0x32A4, 0x0000},
{0x32A6, 0x0000},
{0x3D08, 0x0000}, // dtr_bound
{0x3D0A, 0x0000},
{0x3D0C, 0x0000},
{0x3D0E, 0x0000},
{0x3D10, 0x0000}, // vis_bound
{0x3D12, 0x0798},
{0x3D14, 0x001E},
{0x3D16, 0x045E},
{ }
}; /* MEC_DLO_default */

/* 3Exp HDR, 1280P, MIPI 4-lane 12-bit, 30fps, EXTCLK=23MHz (comes from deser) */
static const struct ar0xxx_reg *ar0233_regs_hdr_mipi_12bit_30fps_rev1[] = {
	ar0233_rev1_Reset,
	ar0233_rev1_Sequencer_Settings,
	ar0233_rev1_disable_embed_data_stat,
	ar0233_rev1_HDR_3exp_12bit,
	ar0233_rev1_Serial_12bit_Timing_Setup_103p5,
	ar0233_rev1_MIPI_4Lane_12BITS,
	ar0233_rev1_Full_resolution,
	ar0233_rev1_Gain_3p28x,
	ar0233_rev1_MEC_DLO_default,
	NULL,
};