/*
 * ON Semiconductor AR0140 sensor camera wizard 1344x968@30/BGGR/BT601/RAW12
 *
 * Copyright (C) 2018 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

//#define AR0140_DISPLAY_PATTERN_FIXED
//#define AR0140_DISPLAY_PATTERN_COLOR_BAR

#define AR0140_DEFAULT_WIDTH	1280
#define AR0140_DEFAULT_HEIGHT	800

#define AR0140_EMB_LINES	4
#define AR0140_EMB_PADDED	(priv->emb_enable ? AR0140_EMB_LINES + 30 : 0) /* embedded data (SOF) and stats (EOF) + post padding */

#define AR0140_DELAY		0xffff

#define AR0140_MAX_WIDTH	1280
#define AR0140_MAX_HEIGHT	800
#define AR0140_SENSOR_WIDTH	1280
#define AR0140_SENSOR_HEIGHT	800

#define AR0140_X_START		((AR0140_SENSOR_WIDTH - AR0140_DEFAULT_WIDTH) / 2)
#define AR0140_Y_START		((AR0140_SENSOR_HEIGHT - AR0140_DEFAULT_HEIGHT) / 2)
#define AR0140_X_END		(AR0140_X_START + AR0140_DEFAULT_WIDTH - 1)
#define AR0140_Y_END		(AR0140_Y_START + AR0140_DEFAULT_HEIGHT + 1) /* must be +1 and not -1 or 2 lines missed - bug in imager? */

struct ar0140_reg {
	u16	reg;
	u16	val;
};

static const struct ar0140_reg ar0140_regs_wizard[] = {
{0x301A, 0x0001},	// reset
{AR0140_DELAY, 100},
{0x301A, 0x10D8},	// Stream off and setup parallel
{0x3070, 0x0001},
{0x3070, 0x0000},	//  1: Solid color test pattern,
			//  2: Full color bar test pattern,
			//  3: Fade to grey color bar test pattern,
			//256: Walking 1 test pattern (12 bit)
#ifdef AR0140_DISPLAY_PATTERN_FIXED
{0x3070, 0x0001},
#endif
{0x3072, 0x0fff},	// R
{0x3074, 0x0fff},	// G(GR row)
{0x3076, 0x0fff},	// B
{0x3078, 0x0fff},	// G(GB row)
#ifdef AR0140_DISPLAY_PATTERN_COLOR_BAR
{0x3070, 0x0002},
#endif
{AR0140_DELAY, 250},
/* SEQ_CTRL_PORT */
{0x3088, 0x8000},
/* SEQ_DATA_PORT */
{0x3086, 0x4558},
{0x3086, 0x6E9B},
{0x3086, 0x4A31},
{0x3086, 0x4342},
{0x3086, 0x8E03},
{0x3086, 0x2714},
{0x3086, 0x4578},
{0x3086, 0x7B3D},
{0x3086, 0xFF3D},
{0x3086, 0xFF3D},
{0x3086, 0xEA27},
{0x3086, 0x043D},
{0x3086, 0x1027},
{0x3086, 0x0527},
{0x3086, 0x1535},
{0x3086, 0x2705},
{0x3086, 0x3D10},
{0x3086, 0x4558},
{0x3086, 0x2704},
{0x3086, 0x2714},
{0x3086, 0x3DFF},
{0x3086, 0x3DFF},
{0x3086, 0x3DEA},
{0x3086, 0x2704},
{0x3086, 0x6227},
{0x3086, 0x288E},
{0x3086, 0x0036},
{0x3086, 0x2708},
{0x3086, 0x3D64},
{0x3086, 0x7A3D},
{0x3086, 0x0444},
{0x3086, 0x2C4B},
{0x3086, 0x8F00},
{0x3086, 0x4372},
{0x3086, 0x719F},
{0x3086, 0x6343},
{0x3086, 0x166F},
{0x3086, 0x9F92},
{0x3086, 0x1244},
{0x3086, 0x1663},
{0x3086, 0x4316},
{0x3086, 0x9326},
{0x3086, 0x0426},
{0x3086, 0x848E},
{0x3086, 0x0327},
{0x3086, 0xFC5C},
{0x3086, 0x0D57},
{0x3086, 0x5417},
{0x3086, 0x0955},
{0x3086, 0x5649},
{0x3086, 0x5F53},
{0x3086, 0x0553},
{0x3086, 0x0728},
{0x3086, 0x6C4C},
{0x3086, 0x0928},
{0x3086, 0x2C72},
{0x3086, 0xAD7C},
{0x3086, 0xA928},
{0x3086, 0xA879},
{0x3086, 0x6026},
{0x3086, 0x9C5C},
{0x3086, 0x1B45},
{0x3086, 0x4845},
{0x3086, 0x0845},
{0x3086, 0x8826},
{0x3086, 0xBE8E},
{0x3086, 0x0127},
{0x3086, 0xF817},
{0x3086, 0x0227},
{0x3086, 0xFA17},
{0x3086, 0x095C},
{0x3086, 0x0B17},
{0x3086, 0x1026},
{0x3086, 0xBA5C},
{0x3086, 0x0317},
{0x3086, 0x1026},
{0x3086, 0xB217},
{0x3086, 0x065F},
{0x3086, 0x2888},
{0x3086, 0x9060},
{0x3086, 0x27F2},
{0x3086, 0x1710},
{0x3086, 0x26A2},
{0x3086, 0x26A3},
{0x3086, 0x5F4D},
{0x3086, 0x2808},
{0x3086, 0x1927},
{0x3086, 0xFA84},
{0x3086, 0x69A0},
{0x3086, 0x785D},
{0x3086, 0x2888},
{0x3086, 0x8710},
{0x3086, 0x8C82},
{0x3086, 0x8926},
{0x3086, 0xB217},
{0x3086, 0x036B},
{0x3086, 0x9C60},
{0x3086, 0x9417},
{0x3086, 0x2926},
{0x3086, 0x8345},
{0x3086, 0xA817},
{0x3086, 0x0727},
{0x3086, 0xFB17},
{0x3086, 0x2945},
{0x3086, 0x881F},
{0x3086, 0x1708},
{0x3086, 0x27FA},
{0x3086, 0x5D87},
{0x3086, 0x108C},
{0x3086, 0x8289},
{0x3086, 0x170E},
{0x3086, 0x4826},
{0x3086, 0x9A28},
{0x3086, 0x884C},
{0x3086, 0x0B79},
{0x3086, 0x1730},
{0x3086, 0x2692},
{0x3086, 0x1709},
{0x3086, 0x9160},
{0x3086, 0x27F2},
{0x3086, 0x1710},
{0x3086, 0x2682},
{0x3086, 0x2683},
{0x3086, 0x5F4D},
{0x3086, 0x2808},
{0x3086, 0x1927},
{0x3086, 0xFA84},
{0x3086, 0x69A1},
{0x3086, 0x785D},
{0x3086, 0x2888},
{0x3086, 0x8710},
{0x3086, 0x8C80},
{0x3086, 0x8A26},
{0x3086, 0x9217},
{0x3086, 0x036B},
{0x3086, 0x9D95},
{0x3086, 0x2603},
{0x3086, 0x5C01},
{0x3086, 0x4558},
{0x3086, 0x8E00},
{0x3086, 0x2798},
{0x3086, 0x170A},
{0x3086, 0x4A65},
{0x3086, 0x4316},
{0x3086, 0x6643},
{0x3086, 0x165B},
{0x3086, 0x4316},
{0x3086, 0x5943},
{0x3086, 0x168E},
{0x3086, 0x0327},
{0x3086, 0x9C45},
{0x3086, 0x7817},
{0x3086, 0x0727},
{0x3086, 0x9D17},
{0x3086, 0x225D},
{0x3086, 0x8710},
{0x3086, 0x2808},
{0x3086, 0x530D},
{0x3086, 0x8C80},
{0x3086, 0x8A45},
{0x3086, 0x5823},
{0x3086, 0x1708},
{0x3086, 0x8E01},
{0x3086, 0x2798},
{0x3086, 0x8E00},
{0x3086, 0x2644},
{0x3086, 0x5C05},
{0x3086, 0x1244},
{0x3086, 0x4B71},
{0x3086, 0x759E},
{0x3086, 0x8B85},
{0x3086, 0x0143},
{0x3086, 0x7271},
{0x3086, 0xA346},
{0x3086, 0x4316},
{0x3086, 0x6FA3},
{0x3086, 0x9612},
{0x3086, 0x4416},
{0x3086, 0x4643},
{0x3086, 0x1697},
{0x3086, 0x2604},
{0x3086, 0x2684},
{0x3086, 0x8E03},
{0x3086, 0x27FC},
{0x3086, 0x5C0D},
{0x3086, 0x5754},
{0x3086, 0x1709},
{0x3086, 0x5556},
{0x3086, 0x495F},
{0x3086, 0x5305},
{0x3086, 0x5307},
{0x3086, 0x286C},
{0x3086, 0x4C09},
{0x3086, 0x282C},
{0x3086, 0x72AE},
{0x3086, 0x7CAA},
{0x3086, 0x28A8},
{0x3086, 0x7960},
{0x3086, 0x269C},
{0x3086, 0x5C1B},
{0x3086, 0x4548},
{0x3086, 0x4508},
{0x3086, 0x4588},
{0x3086, 0x26BE},
{0x3086, 0x8E01},
{0x3086, 0x27F8},
{0x3086, 0x1702},
{0x3086, 0x27FA},
{0x3086, 0x1709},
{0x3086, 0x5C0B},
{0x3086, 0x1710},
{0x3086, 0x26BA},
{0x3086, 0x5C03},
{0x3086, 0x1710},
{0x3086, 0x26B2},
{0x3086, 0x1706},
{0x3086, 0x5F28},
{0x3086, 0x8898},
{0x3086, 0x6027},
{0x3086, 0xF217},
{0x3086, 0x1026},
{0x3086, 0xA226},
{0x3086, 0xA35F},
{0x3086, 0x4D28},
{0x3086, 0x081A},
{0x3086, 0x27FA},
{0x3086, 0x8469},
{0x3086, 0xA578},
{0x3086, 0x5D28},
{0x3086, 0x8887},
{0x3086, 0x108C},
{0x3086, 0x8289},
{0x3086, 0x26B2},
{0x3086, 0x1703},
{0x3086, 0x6BA4},
{0x3086, 0x6099},
{0x3086, 0x1729},
{0x3086, 0x2683},
{0x3086, 0x45A8},
{0x3086, 0x1707},
{0x3086, 0x27FB},
{0x3086, 0x1729},
{0x3086, 0x4588},
{0x3086, 0x2017},
{0x3086, 0x0827},
{0x3086, 0xFA5D},
{0x3086, 0x8710},
{0x3086, 0x8C82},
{0x3086, 0x8917},
{0x3086, 0x0E48},
{0x3086, 0x269A},
{0x3086, 0x2888},
{0x3086, 0x4C0B},
{0x3086, 0x7917},
{0x3086, 0x3026},
{0x3086, 0x9217},
{0x3086, 0x099A},
{0x3086, 0x6027},
{0x3086, 0xF217},
{0x3086, 0x1026},
{0x3086, 0x8226},
{0x3086, 0x835F},
{0x3086, 0x4D28},
{0x3086, 0x081A},
{0x3086, 0x27FA},
{0x3086, 0x8469},
{0x3086, 0xAB78},
{0x3086, 0x5D28},
{0x3086, 0x8887},
{0x3086, 0x108C},
{0x3086, 0x808A},
{0x3086, 0x2692},
{0x3086, 0x1703},
{0x3086, 0x6BA6},
{0x3086, 0xA726},
{0x3086, 0x035C},
{0x3086, 0x0145},
{0x3086, 0x588E},
{0x3086, 0x0027},
{0x3086, 0x9817},
{0x3086, 0x0A4A},
{0x3086, 0x0A43},
{0x3086, 0x160B},
{0x3086, 0x438E},
{0x3086, 0x0327},
{0x3086, 0x9C45},
{0x3086, 0x7817},
{0x3086, 0x0727},
{0x3086, 0x9D17},
{0x3086, 0x225D},
{0x3086, 0x8710},
{0x3086, 0x2808},
{0x3086, 0x530D},
{0x3086, 0x8C80},
{0x3086, 0x8A45},
{0x3086, 0x5817},
{0x3086, 0x088E},
{0x3086, 0x0127},
{0x3086, 0x988E},
{0x3086, 0x0076},
{0x3086, 0xAC77},
{0x3086, 0xAC46},
{0x3086, 0x4416},
{0x3086, 0x16A8},
{0x3086, 0x7A26},
{0x3086, 0x445C},
{0x3086, 0x0512},
{0x3086, 0x444B},
{0x3086, 0x7175},
{0x3086, 0xA24A},
{0x3086, 0x0343},
{0x3086, 0x1604},
{0x3086, 0x4316},
{0x3086, 0x5843},
{0x3086, 0x165A},
{0x3086, 0x4316},
{0x3086, 0x0643},
{0x3086, 0x1607},
{0x3086, 0x4316},
{0x3086, 0x8E03},
{0x3086, 0x279C},
{0x3086, 0x4578},
{0x3086, 0x7B17},
{0x3086, 0x078B},
{0x3086, 0x8627},
{0x3086, 0x9D17},
{0x3086, 0x2345},
{0x3086, 0x5822},
{0x3086, 0x1708},
{0x3086, 0x8E01},
{0x3086, 0x2798},
{0x3086, 0x8E00},
{0x3086, 0x2644},
{0x3086, 0x5C05},
{0x3086, 0x1244},
{0x3086, 0x4B8D},
{0x3086, 0x602C},
{0x3086, 0x2C2C},
{0x3086, 0x2C00},
/* End Sequencer */
{0x3064, 0x1982}, // SMIA_TEST
/* PCLK=27Mhz/0x2 *0x30 /1/0x10 - TI serializers */
{0x302A, 0x0010}, // vt_pix_clk_div
{0x302E, 0x0002}, // pre_pll_clk_div
{0x3030, 0x0030}, // pll_multiplier
#if 0
/* Resolution: 1284x804x29.98p */
{0x3002, 0x001A}, // y_addr_start 
{0x3004, 0x0014}, // x_addr_start 
{0x3006, 0x033D}, // y_addr_end}, 804 lines
{0x3008, 0x0517}, // x_addr_end}, 1284px
{0x300A, 0x0344}, // frame_length_lines}, 840 lines
{0x300C, 0x0648}, // line_length_pck}, 1608px
#else
{0x31B0, 0x0056}, // FRAME_PREAMBLE
{0x31B2, 0x0045}, // LINE_PREAMBLE
{0x3004, AR0140_X_START}, // X_ADDR_START_
{0x3008, AR0140_X_END}, // X_ADDR_END_
{0x3002, AR0140_Y_START}, // Y_ADDR_START_
{0x3006, AR0140_Y_END}, // Y_ADDR_END_
{0x3402, 0x0000 | AR0140_MAX_WIDTH}, // X_OUTPUT_CONTROL
{0x3404, 0x0000 | AR0140_MAX_HEIGHT}, // Y_OUTPUT_CONTROL
{0x300A, AR0140_SENSOR_HEIGHT + 36}, // FRAME_LENGTH_LINES_
{0x300C, AR0140_SENSOR_WIDTH + 328}, // LINE_LENGTH_PCK_
#endif
/* Rev3 Optimized Settings */
{0x3044, 0x0400},
{0x3052, 0xA134},
{0x3092, 0x010F},
{0x30FE, 0x0080},
{0x3ECE, 0x40FF},
{0x3ED0, 0xFF40},
{0x3ED2, 0xA906},
{0x3ED4, 0x001F},
{0x3ED6, 0x638F},
{0x3ED8, 0xCC99},
{0x3EDA, 0x0888},
{0x3EDE, 0x8878},
{0x3EE0, 0x7744},
{0x3EE2, 0x5563},
{0x3EE4, 0xAAE0},
{0x3EE6, 0x3400},
{0x3EEA, 0xA4FF},
{0x3EEC, 0x80F0},
{0x3EEE, 0x0000},
{0x31E0, 0x1701},
/* HDR mode */
/* 2D Motion Compensation */
{0x318A, 0x0E74}, // hdr_mc_ctrl1
{0x318C, 0xC000}, // hdr_mc_ctrl2
{0x318E, 0x0800}, // hdr_mc_ctrl3: Gain before DLO set to 1
{0x3190, 0x0000}, // hdr_mc_ctrl4: if DLO enabled overrides 2D MC
{0x3192, 0x0400}, // hdr_mc_ctrl5
{0x3194, 0x0BB8}, // hdr_mc_ctrl6: T1 barrier set to 3000
{0x3196, 0x0E74}, // hdr_mc_ctrl7: T2 barrier set to 3700
{0x3198, 0x183C}, // hdr_mc_ctrl8: Motion detect Q1 set to 60}, Q2 set to 24
{0x3200, 0x0002}, // adacd_control
{0x3202, 0x00A0}, // adacd_noise_model1
{0x3206, 0x0A06}, // adacd_noise_floor1
{0x3208, 0x1A12}, // adacd_noise_floor2
{0x320A, 0x0080}, // adacd_pedestal
{0x306E, 0x9010}, // datapath select - LV noncontinuous
{0x31AC, 0x100C}, // DATA_FORMAT_BITS: RAW12
{0x31AE, 0x0301}, // SERIAL_FORMAT
{0x301A, 0x11D8}, // RESET_REGISTER
// patch start
{0x3012, 0x0206}, // COARSE_INTEGRATION_TIME_: T1 exposure - max=0x400
// patch end
// enable trigger (trigger pin is dedicated)
{0x30CE, 0x0120}, // TRIGGER_MODE
//{0x30DC, 0x0120}, // TRIGGER_DELAY
};