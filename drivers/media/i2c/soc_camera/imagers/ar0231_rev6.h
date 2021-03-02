/*
 * ON Semiconductor AR0231 sensor camera wizard 1920x1080@30/BGGR/MIPI
 *
 * Copyright (C) 2018-2020 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* Parallel Timing Setup 27MHz In 88 MHz Out */
static const struct ar0231_reg ar0231_regs_wizard_rev6_dvp[] = {
{0x301A, 0x0001},	// reset
{0x301A, 0x10D8},	// Stream off and setup parallel
{0x3070, 0x0000},	//  1: Solid color test pattern,
			//  2: Full color bar test pattern,
			//  3: Fade to grey color bar test pattern,
			//256: Walking 1 test pattern (12 bit)
{0x3072, 0x0123},	// R
{0x3074, 0x0456},	// G(GR row)
{0x3076, 0x0abc},	// B
{0x3078, 0x0def},	// G(GB row)
#ifdef AR0231_DISPLAY_PATTERN_FIXED
{0x3070, 0x0001},
#endif
#ifdef AR0231_DISPLAY_PATTERN_COLOR_BAR
{0x3070, 0x0002},
#endif

//Recommended Settings
{0x3056, 0x0080}, // GREEN1_GAIN
{0x305C, 0x0080}, // GREEN2_GAIN
{0x3058, 0x0080}, // BLUE_GAIN
{0x305A, 0x0080}, // RED_GAIN
{0x3138, 0x000B}, // OTPM_TCFG_OPT
{0x3372, 0xF54F}, // RESERVED_MFR_3372
{0x337A, 0x0D70}, // RESERVED_MFR_337A
{0x337E, 0x1FFD}, // RESERVED_MFR_337E
{0x3382, 0x00C0}, // RESERVED_MFR_3382
{0x3C04, 0x0E80}, // RESERVED_MFR_3C04
{0x3F90, 0x06E1}, // RESERVED_MFR_3F90
{0x3F92, 0x06E1}, // RESERVED_MFR_3F92
{0x350E, 0x1F14}, // RESERVED_MFR_350E
{0x350E, 0xFF10}, // RESERVED_MFR_350E
{0x3506, 0x4444}, // RESERVED_MFR_3506
{0x3508, 0x4444}, // RESERVED_MFR_3508
{0x350A, 0x4465}, // RESERVED_MFR_350A
{0x350C, 0x055F}, // RESERVED_MFR_350C
{0x3566, 0x9D38}, // RESERVED_MFR_3566
{0x3518, 0x1FFE}, // RESERVED_MFR_3518
{0x3520, 0xC688}, // RESERVED_MFR_3520
{0x3522, 0x88C0}, // RESERVED_MFR_3522
{0x3524, 0xC0C6}, // RESERVED_MFR_3524
{0x352C, 0xC6C6}, // RESERVED_MFR_352C
{0x3528, 0x0900}, // RESERVED_MFR_3528
{0x3528, 0x9900}, // RESERVED_MFR_3528
{0x3528, 0x9909}, // RESERVED_MFR_3528
{0x3528, 0x9999}, // RESERVED_MFR_3528
{0x352A, 0x089F}, // RESERVED_MFR_352A
{0x352E, 0x0001}, // RESERVED_MFR_352E
{0x352E, 0x0011}, // RESERVED_MFR_352E
{0x3530, 0x0400}, // RESERVED_MFR_3530
{0x3530, 0x4400}, // RESERVED_MFR_3530
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF00}, // RESERVED_MFR_3536
{0x3536, 0xFF02}, // RESERVED_MFR_3536
{0x3536, 0xFF06}, // RESERVED_MFR_3536
{0x3536, 0xFF06}, // RESERVED_MFR_3536
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x3538, 0xFFFF}, // RESERVED_MFR_3538
{0x353A, 0x9000}, // RESERVED_MFR_353A
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x353C, 0x3F00}, // RESERVED_MFR_353C
{0x32EC, 0x72A1}, // RESERVED_MFR_32EC
{0x3540, 0xC637}, // RESERVED_MFR_3540
{0x3540, 0xC637}, // RESERVED_MFR_3540
{0x3540, 0xC637}, // RESERVED_MFR_3540
{0x3542, 0x584B}, // RESERVED_MFR_3542
{0x3542, 0x464B}, // RESERVED_MFR_3542
{0x3544, 0x565A}, // RESERVED_MFR_3544
{0x3544, 0x4B5A}, // RESERVED_MFR_3544
{0x3546, 0x545A}, // RESERVED_MFR_3546
{0x3546, 0x5A5A}, // RESERVED_MFR_3546
{0x3548, 0x6400}, // RESERVED_MFR_3548
{0x3556, 0x101F}, // RESERVED_MFR_3556
{0x3566, 0x9D38}, // RESERVED_MFR_3566
{0x3566, 0x1D38}, // RESERVED_MFR_3566
{0x3566, 0x1D28}, // RESERVED_MFR_3566
{0x3566, 0x1128}, // RESERVED_MFR_3566
{0x3566, 0x1328}, // RESERVED_MFR_3566
{0x3566, 0x3328}, // RESERVED_MFR_3566
{0x3528, 0xDDDD}, // RESERVED_MFR_3528

//Sequencer Update
{0x2512, 0x8000}, // SEQ_CTRL_PORT
{0x2510, 0x0905}, // SEQ_DATA_PORT
{0x2510, 0x3350}, // SEQ_DATA_PORT
{0x2510, 0x2004}, // SEQ_DATA_PORT
{0x2510, 0x1460}, // SEQ_DATA_PORT
{0x2510, 0x1578}, // SEQ_DATA_PORT
{0x2510, 0x1360}, // SEQ_DATA_PORT
{0x2510, 0x7B24}, // SEQ_DATA_PORT
{0x2510, 0xFF24}, // SEQ_DATA_PORT
{0x2510, 0xFF24}, // SEQ_DATA_PORT
{0x2510, 0xEA24}, // SEQ_DATA_PORT
{0x2510, 0x1022}, // SEQ_DATA_PORT
{0x2510, 0x2410}, // SEQ_DATA_PORT
{0x2510, 0x155A}, // SEQ_DATA_PORT
{0x2510, 0x1342}, // SEQ_DATA_PORT
{0x2510, 0x1400}, // SEQ_DATA_PORT
{0x2510, 0x24FF}, // SEQ_DATA_PORT
{0x2510, 0x24FF}, // SEQ_DATA_PORT
{0x2510, 0x24EA}, // SEQ_DATA_PORT
{0x2510, 0x2324}, // SEQ_DATA_PORT
{0x2510, 0x647A}, // SEQ_DATA_PORT
{0x2510, 0x2404}, // SEQ_DATA_PORT
{0x2510, 0x052C}, // SEQ_DATA_PORT
{0x2510, 0x400A}, // SEQ_DATA_PORT
{0x2510, 0xFF0A}, // SEQ_DATA_PORT
{0x2510, 0xFF0A}, // SEQ_DATA_PORT
{0x2510, 0x1808}, // SEQ_DATA_PORT
{0x2510, 0x3851}, // SEQ_DATA_PORT
{0x2510, 0x1440}, // SEQ_DATA_PORT
{0x2510, 0x0004}, // SEQ_DATA_PORT
{0x2510, 0x0801}, // SEQ_DATA_PORT
{0x2510, 0x0408}, // SEQ_DATA_PORT
{0x2510, 0x1180}, // SEQ_DATA_PORT
{0x2510, 0x15DC}, // SEQ_DATA_PORT
{0x2510, 0x134C}, // SEQ_DATA_PORT
{0x2510, 0x1002}, // SEQ_DATA_PORT
{0x2510, 0x1016}, // SEQ_DATA_PORT
{0x2510, 0x1181}, // SEQ_DATA_PORT
{0x2510, 0x1189}, // SEQ_DATA_PORT
{0x2510, 0x1056}, // SEQ_DATA_PORT
{0x2510, 0x1210}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x0D08}, // SEQ_DATA_PORT
{0x2510, 0x0913}, // SEQ_DATA_PORT
{0x2510, 0x13C8}, // SEQ_DATA_PORT
{0x2510, 0x092B}, // SEQ_DATA_PORT
{0x2510, 0x1588}, // SEQ_DATA_PORT
{0x2510, 0x1388}, // SEQ_DATA_PORT
{0x2510, 0x090B}, // SEQ_DATA_PORT
{0x2510, 0x11D9}, // SEQ_DATA_PORT
{0x2510, 0x091D}, // SEQ_DATA_PORT
{0x2510, 0x1441}, // SEQ_DATA_PORT
{0x2510, 0x0903}, // SEQ_DATA_PORT
{0x2510, 0x1214}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x10D6}, // SEQ_DATA_PORT
{0x2510, 0x1210}, // SEQ_DATA_PORT
{0x2510, 0x1212}, // SEQ_DATA_PORT
{0x2510, 0x1210}, // SEQ_DATA_PORT
{0x2510, 0x11DD}, // SEQ_DATA_PORT
{0x2510, 0x11D9}, // SEQ_DATA_PORT
{0x2510, 0x1056}, // SEQ_DATA_PORT
{0x2510, 0x090B}, // SEQ_DATA_PORT
{0x2510, 0x11DB}, // SEQ_DATA_PORT
{0x2510, 0x0915}, // SEQ_DATA_PORT
{0x2510, 0x119B}, // SEQ_DATA_PORT
{0x2510, 0x090F}, // SEQ_DATA_PORT
{0x2510, 0x11BB}, // SEQ_DATA_PORT
{0x2510, 0x121A}, // SEQ_DATA_PORT
{0x2510, 0x1210}, // SEQ_DATA_PORT
{0x2510, 0x1460}, // SEQ_DATA_PORT
{0x2510, 0x1250}, // SEQ_DATA_PORT
{0x2510, 0x1076}, // SEQ_DATA_PORT
{0x2510, 0x10E6}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x15AB}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x13A8}, // SEQ_DATA_PORT
{0x2510, 0x1240}, // SEQ_DATA_PORT
{0x2510, 0x1260}, // SEQ_DATA_PORT
{0x2510, 0x0923}, // SEQ_DATA_PORT
{0x2510, 0x158D}, // SEQ_DATA_PORT
{0x2510, 0x138D}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x0B09}, // SEQ_DATA_PORT
{0x2510, 0x0108}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x1440}, // SEQ_DATA_PORT
{0x2510, 0x091D}, // SEQ_DATA_PORT
{0x2510, 0x1588}, // SEQ_DATA_PORT
{0x2510, 0x1388}, // SEQ_DATA_PORT
{0x2510, 0x092D}, // SEQ_DATA_PORT
{0x2510, 0x1066}, // SEQ_DATA_PORT
{0x2510, 0x0905}, // SEQ_DATA_PORT
{0x2510, 0x0C08}, // SEQ_DATA_PORT
{0x2510, 0x090B}, // SEQ_DATA_PORT
{0x2510, 0x1441}, // SEQ_DATA_PORT
{0x2510, 0x090D}, // SEQ_DATA_PORT
{0x2510, 0x10E6}, // SEQ_DATA_PORT
{0x2510, 0x0901}, // SEQ_DATA_PORT
{0x2510, 0x1262}, // SEQ_DATA_PORT
{0x2510, 0x1260}, // SEQ_DATA_PORT
{0x2510, 0x11BF}, // SEQ_DATA_PORT
{0x2510, 0x11BB}, // SEQ_DATA_PORT
{0x2510, 0x1066}, // SEQ_DATA_PORT
{0x2510, 0x11FB}, // SEQ_DATA_PORT
{0x2510, 0x0935}, // SEQ_DATA_PORT
{0x2510, 0x11BB}, // SEQ_DATA_PORT
{0x2510, 0x1263}, // SEQ_DATA_PORT
{0x2510, 0x1260}, // SEQ_DATA_PORT
{0x2510, 0x1400}, // SEQ_DATA_PORT
{0x2510, 0x1510}, // SEQ_DATA_PORT
{0x2510, 0x11B8}, // SEQ_DATA_PORT
{0x2510, 0x12A0}, // SEQ_DATA_PORT
{0x2510, 0x1200}, // SEQ_DATA_PORT
{0x2510, 0x1026}, // SEQ_DATA_PORT
{0x2510, 0x1000}, // SEQ_DATA_PORT
{0x2510, 0x1342}, // SEQ_DATA_PORT
{0x2510, 0x1100}, // SEQ_DATA_PORT
{0x2510, 0x7A06}, // SEQ_DATA_PORT
{0x2510, 0x0915}, // SEQ_DATA_PORT
{0x2510, 0x0507}, // SEQ_DATA_PORT
{0x2510, 0x0841}, // SEQ_DATA_PORT
{0x2510, 0x3750}, // SEQ_DATA_PORT
{0x2510, 0x2C2C}, // SEQ_DATA_PORT
{0x2510, 0xFE05}, // SEQ_DATA_PORT
{0x2510, 0xFE13}, // SEQ_DATA_PORT
{0x1008, 0x0361}, // FINE_INTEGRATION_TIME_MIN
{0x100C, 0x0589}, // FINE_INTEGRATION_TIME2_MIN
{0x100E, 0x07B1}, // FINE_INTEGRATION_TIME3_MIN
{0x1010, 0x0139}, // FINE_INTEGRATION_TIME4_MIN
{0x3230, 0x0304}, // FINE_CORRECTION
{0x3232, 0x052C}, // FINE_CORRECTION2
{0x3234, 0x0754}, // FINE_CORRECTION3
{0x3236, 0x00DC}, // FINE_CORRECTION4
{0x3566, 0x3328}, // RESERVED_MFR_3566
{0x350C, 0x055F}, // RESERVED_MFR_350C
{0x32D0, 0x3A02}, // RESERVED_MFR_32D0
{0x32D2, 0x3508}, // RESERVED_MFR_32D2
{0x32D4, 0x3702}, // RESERVED_MFR_32D4
{0x32D6, 0x3C04}, // RESERVED_MFR_32D6
{0x32DC, 0x370A}, // RESERVED_MFR_32DC

//Parallel Timing Setup 27MHz In 88 MHz Out
{0x302A, 0x0009}, // VT_PIX_CLK_DIV
{0x302C, 0x0001}, // VT_SYS_CLK_DIV
{0x302E, 0x0003}, // PRE_PLL_CLK_DIV
{0x3030, 0x0058}, // PLL_MULTIPLIER
{0x3036, 0x0008}, // OP_WORD_CLK_DIV
{0x3038, 0x0001}, // OP_SYS_CLK_DIV
{0x30B0, 0x0B02}, // DIGITAL_TEST

//Readout Mode Configuration
{0x30A2, 0x0001}, // X_ODD_INC_
{0x30A6, 0x0001}, // Y_ODD_INC_
{0x3040, 0x0000}, // READ_MODE
{0x3082, 0x0008}, // OPERATION_MODE_CTRL
{0x30BA, 0x11E2}, // DIGITAL_CTRL
{0x3044, 0x0400}, // DARK_CONTROL
#ifdef AR0231_EMBEDDED_LINE
{0x3064, 0x1982}, // SMIA_TEST
#else
{0x3064, 0x1802}, // SMIA_TEST
#endif
{0x33E0, 0x0880}, // RESERVED_MFR_33E0
{0x3180, 0x0080}, // RESERVED_MFR_3180
{0x33E4, 0x0080}, // RESERVED_MFR_33E4
{0x33E0, 0x0C80}, // RESERVED_MFR_33E0

#if 1
{0x3004, AR0231_X_START}, // X_ADDR_START_
{0x3008, AR0231_X_END}, // X_ADDR_END_
{0x3002, AR0231_Y_START}, // Y_ADDR_START_
{0x3006, AR0231_Y_END}, // Y_ADDR_END_
{0x3402, 0x0000 | AR0231_MAX_WIDTH}, // X_OUTPUT_CONTROL
{0x3404, 0x0000 | AR0231_MAX_HEIGHT}, // Y_OUTPUT_CONTROL
#else
{0x3004, 0}, // X_ADDR_START_
{0x3008, 0x0787}, // X_ADDR_END_
{0x3002, 0x0000}, // Y_ADDR_START_
{0x3006, 0x04B7}, // Y_ADDR_END_
{0x3402, 0x0788}, // RESERVED_MFR_3402
{0x3402, 0x0F10}, // RESERVED_MFR_3402
{0x3404, 0x0440}, // RESERVED_MFR_3404
{0x3404, 0x0970}, // RESERVED_MFR_3404
#endif
{0x3032, 0x0000}, // SCALING_MODE
{0x3400, 0x0010}, // RESERVED_MFR_3400

//3exp Timing and Exposure
{0x3082, 0x0008}, // OPERATION_MODE_CTRL
{0x30BA, 0x11E2}, // DIGITAL_CTRL
{0x300A, 0x05CA}, // FRAME_LENGTH_LINES_
{0x300C, 0x07BA}, // LINE_LENGTH_PCK_
{0x3042, 0x0000}, // EXTRA_DELAY
{0x3238, 0x0222}, // EXPOSURE_RATIO
{0x3012, 0x0163}, // COARSE_INTEGRATION_TIME_
{0x3014, 0x08CC}, // FINE_INTEGRATION_TIME_
{0x321E, 0x08CC}, // FINE_INTEGRATION_TIME2
{0x3222, 0x0254}, // FINE_INTEGRATION_TIME3
{0x30B0, 0x0A00}, // DIGITAL_TEST
{0x32EA, 0x3C0E}, // RESERVED_MFR_32EA
{0x32EC, 0x72A1}, // RESERVED_MFR_32EC

//Parallel HDR 12 bit Output
{0x31D0, 0x0001}, // COMPANDING
{0x31AE, 0x0001}, // SERIAL_FORMAT
{0x31AC, 0x140C}, // DATA_FORMAT_BITS

#if 0 // no need for front only camera
/* Enable trigger input */
{0x340A, 0x00E0}, // GPIO_CONTROL1: GPIO1 is trigger
{0x340C, 0x0002}, // GPIO_CONTROL2: GPIO1 is trigger
{0x30CE, 0x0120}, // TRIGGER_MODE
//{0x30DC, 0x0120}, // TRIGGER_DELAY
{0x301A, 0x01D8}, // GPI pins enable
#endif

{0x301A, 0x01DC}, // RESET_REGISTER - stream on

#if 1
{0x300A, AR0231_SENSOR_HEIGHT + 225}, // FRAME_LENGTH_LINES_
{0x300C, AR0231_SENSOR_WIDTH + 120}, // LINE_LENGTH_PCK_
/* the sequence must be updated to use following timings, now it is a hack */
{0x1008, 0x0fff}, // FINE_INTEGRATION_TIME_MIN
{0x100C, 0x0fff}, // FINE_INTEGRATION_TIME2_MIN
{0x100E, 0x0fff}, // FINE_INTEGRATION_TIME3_MIN
{0x1010, 0x0fff}, // FINE_INTEGRATION_TIME4_MIN
#endif
};