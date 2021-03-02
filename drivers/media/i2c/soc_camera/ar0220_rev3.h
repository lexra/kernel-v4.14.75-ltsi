/*
 * ON Semiconductor AR0220 sensor camera wizard 1848x944@30/BGGR/MIPI
 *
 * Copyright (C) 2019 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

static const struct ar0xxx_reg ar0220_rev3_Recommended_Settings[] = {
// Set the SREG_WRITE_ALL bit to remove potential higher current during intial standby
{0x3500, 0x0100},  //This is a self clearing bit
{AR_DELAY, 1},     //Delay => 4100 * Extclk 

{0x3092, 0x0824},  // row noise dither enabled, dither scale=0 udpated 6/30/2017
{0x3096, 0x227C},  // row noise adjust top update 4/27/2017 with Jan-4 timing...YL
{0x3098, 0x227C},  // row noise adjust bot update 4/27/2017 with Jan-4 timing...YL
{0x3750, 0x227C},  // row noise adjust top gain ... these are for T1, 4/27/2017 with Jan-4 timing...YL
{0x3752, 0x227C},  // row noise adjust btm gain ... these are for T1, 4/27/2017 with Jan-4 timing...YL
{0x351C, 0x00B3},  // vaapix load cap=11, agnd load cap=3 updated 4/27 with Jan-4 timing...YL

{0x3364, 172},     // set dual conversion gain ratio (HCG/LCG) to 5.3x  (~172/32) 	updated 3/29
{0x337A, 0x0BB8},  // dblc scale factor 0.733x, 					updated 4/25

//  DLO settings
{0x3110, 0x0011}, // Pre-HDR gain enable; Pixel_width_enable
//  These settings match AR0138 REV3 settings
{0x3100, 0x4000},  // dlo control.  turn on dlo noise filter.  set barrier dither select to 0 to match AR0138
{0x3102, 0x6060},  // "better" filter settings.  This is S2=96 and range=6
{0x3104, 0x6060},  // T3
{0x3106, 0x6060},  // T4

{0x32EC, 0x72A2},  // shut_ctrl2_t3_sh_adcance=2
{0x350E, 0x0F14},  // adc_pedestal_dither_en=15 (from 231) to minimize RTN periodical bumps... updated 8/9/17 

// boost settings
// Load booster settings. Early CREV4 headboards and parts will require booster settings to be manually programmed, 
// while CREV4 ES samples have part-specific booster settings programmed in OTPM. Booster settings are instored in
// following 4 registers: 0x3520, 0x3522, 0x3524, and 0x352C. (9/22/2017 Lin)
// Remove soft trim booster settings for CRev3-1 sensors as they are boosters trimmed. (01/14/2018 Lin)
//PYTHON= loadBoosterSettings() 
//{0x3520, 0x1288  // RSTHI=3.9v, use fine_code=18 and coarse_code=0, Casey's rec'd, updated 12/16, 
//{0x3522, 0x860C  // ROWHI=3.6v, Vrst_lo for hcg_lg change from 8 to 6 for eclipse margin 9/27, HTOL updated 12/16, use fine_code=12 and coarse_code=0, Casey's rec'd
//{0x3524, 0x0C12  // DCGHI=3.6v, TXHI=3.9v updated HTOL voltage 12/16 Casey's rec'd 
//{0x352c, 0x0012  // RSTWELLHI=3.9v using fine_code=18 and coarse_code=0 HTOL voltage updated 12/16  C. Hoekstra rec'd, 
//{0x3532, 0x826C  // updated (12/14)
{0x3528, 0x3633},  //update vtxlo for lcg_lg to 0.3V for blue blooming issue 6/1/2018
{0x3532, 0xAE6C},  // ECL settings updated (1/9 Lin)
{0x353a, 0x9000},  // use AGND based boosters, enable continuous boost for RSTHI_WELL, TXHI_WELL (12/14)

{0x3540, 0xC63C},  // eclipse clamp updated 5/16
{0x3542, 0x4637},  // eclipse clamp updated 5/16
{0x3544, 0x3750},  // eclipse clamp updated 5/16
{0x3546, 0x5656},  // eclipse clamp updated 5/16
{0x3548, 0x5600},  // eclipse clamp updated 5/16

//{0x3566, 0xBF38},  // col_mem settings, sa_park_en
{0x3566, 0x3F28},  // col_mem settings, sa_park_en  5/2/2019 disable READOUT_PARK_ENABLE and SA_PARK_EN

{0x30BA, 0x0112},  // Add dither after delay buffer decompander
{0x30BA, 0x0112},  //stop read back of dtest register from analog core to remove dark row on top 5/9
{ }
}; /* Rev3 Recommended Settings */

static const struct ar0xxx_reg ar0220_rev3_REV3_Optimized_Sequencer[] = {
{0x2512, 0x8000},
{0x2510, 0x0905},
{0x2510, 0x3350},
{0x2510, 0x2004},
{0x2510, 0x1460},
{0x2510, 0x1578},
{0x2510, 0x1360},
{0x2510, 0x7B24},
{0x2510, 0xFF24},
{0x2510, 0xFF24},
{0x2510, 0xEA24},
{0x2510, 0x1022},
{0x2510, 0x2410},
{0x2510, 0x155A},
{0x2510, 0x1342},
{0x2510, 0x1440},
{0x2510, 0x24FF},
{0x2510, 0x24FF},
{0x2510, 0x24EA},
{0x2510, 0x2324},
{0x2510, 0x647A},
{0x2510, 0x2404},
{0x2510, 0x052C},
{0x2510, 0x400A},
{0x2510, 0xFF0A},
{0x2510, 0x850A},
{0x2510, 0x0608},
{0x2510, 0x3851},
{0x2510, 0x0905},
{0x2510, 0x15DC},
{0x2510, 0x134C},
{0x2510, 0x0004},
{0x2510, 0x0801},
{0x2510, 0x0408},
{0x2510, 0x1180},
{0x2510, 0x1002},
{0x2510, 0x1016},
{0x2510, 0x1181},
{0x2510, 0x1189},
{0x2510, 0x1056},
{0x2510, 0x1210},
{0x2510, 0x0D09},
{0x2510, 0x0714},
{0x2510, 0x4114},
{0x2510, 0x4009},
{0x2510, 0x0815},
{0x2510, 0xCC13},
{0x2510, 0xCC15},
{0x2510, 0x8813},
{0x2510, 0x8809},
{0x2510, 0x1111},
{0x2510, 0x9909},
{0x2510, 0x0B11},
{0x2510, 0xD909},
{0x2510, 0x0B12},
{0x2510, 0x1409},
{0x2510, 0x0112},
{0x2510, 0x1010},
{0x2510, 0xD612},
{0x2510, 0x1212},
{0x2510, 0x1011},
{0x2510, 0xDD11},
{0x2510, 0xD910},
{0x2510, 0x5609},
{0x2510, 0x1111},
{0x2510, 0xDB09},
{0x2510, 0x1D11},
{0x2510, 0xFB09},
{0x2510, 0x0911},
{0x2510, 0xBB12},
{0x2510, 0x1A12},
{0x2510, 0x1010},
{0x2510, 0xD612},
{0x2510, 0x5010},
{0x2510, 0xF610},
{0x2510, 0xE609},
{0x2510, 0x0315},
{0x2510, 0xAB13},
{0x2510, 0xAB12},
{0x2510, 0x4012},
{0x2510, 0x6009},
{0x2510, 0x2315},
{0x2510, 0x8809},
{0x2510, 0x0113},
{0x2510, 0x880B},
{0x2510, 0x0906},
{0x2510, 0x158D},
{0x2510, 0x138D},
{0x2510, 0x090B},
{0x2510, 0x1066},
{0x2510, 0x1588},
{0x2510, 0x1388},
{0x2510, 0x0C09},
{0x2510, 0x0410},
{0x2510, 0xE612},
{0x2510, 0x6212},
{0x2510, 0x6011},
{0x2510, 0xBF11},
{0x2510, 0xFB10},
{0x2510, 0x6609},
{0x2510, 0x3B11},
{0x2510, 0xBB12},
{0x2510, 0x6312},
{0x2510, 0x6009},
{0x2510, 0x0115},
{0x2510, 0x5A11},
{0x2510, 0xB812},
{0x2510, 0xA012},
{0x2510, 0x0010},
{0x2510, 0x2610},
{0x2510, 0x0013},
{0x2510, 0x0211},
{0x2510, 0x8014},
{0x2510, 0x007A},
{0x2510, 0x0611},
{0x2510, 0x0005},
{0x2510, 0x0708},
{0x2510, 0x4137},
{0x2510, 0x502C},
{0x2510, 0x2CFE},
{0x2510, 0x112C},
{AR_DELAY, 300},

{0x1008, 0x02B6}, //fine_integration_time_min
{0x100c, 0x0452}, //fine_integration_time2_min
{0x100e, 0x05EE}, //fine_integration_time3_min
{0x1010, 0x011A}, //fine_integration_time4_min

{0x3230, 0x0254}, //fine_correction
{0x3232, 0x03F0}, //fine_correction2
{0x3234, 0x058C}, //fine_correction3
{0x3236, 0x00B8}, //fine_correction4
//{0x32e6, 0x009A}, //min_subrow
{0x32e6, 0x00BC}, //min_subrow 188 2/6/2018 Lin
{ }
}; /* AR0220AT REV3 Optimized Sequencer */

/* 3exp HDR, Full Resolution, MIPI 4-lane 12-bit 36FPS, EXTCLK=23MHz (comes from deser) */
static const struct ar0xxx_reg *ar0220_regs_hdr_mipi_12bit_36fps_rev3[] = {
	ar0220_rev2_Reset,
	ar0220_rev3_Recommended_Settings,
	ar0220_rev3_REV3_Optimized_Sequencer,
	ar0220_rev2_pll_23_4lane_12b,
	ar0220_rev2_Readout_Mode_Configuration,
	ar0220_rev2_Full_Res_FOV,
	ar0220_rev2_hdr_Timing_and_Exposure,
	ar0220_rev2_hdr_12bit_output,
	ar0220_rev2_mipi_12bit_4lane,
	ar0220_rev2_Recommended_HDR_Settings,
	NULL,
};
