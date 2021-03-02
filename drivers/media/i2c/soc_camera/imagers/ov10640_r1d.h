/*
 * OmniVision ov10640 sensor camera wizard 1280x1080@30/BGGR/BT601/12bit
 *
 * Copyright (C) 2015-2020 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* DVP_1280x960_COMB12_raw 30fps */
static const struct ov10640_reg ov10640_regs_wizard_r1d[] = {
//{0x3013, 0x01},
//{OV10640_DELAY, 10},
{0x328a, 0x01},
{0x313f, 0x80},
{0x3132, 0x24},
{0x3000, 0x03},
{0x3001, 0x38},
{0x3002, 0x07},
{0x3004, 0x03},
{0x3005, 0x38},
{0x3006, 0x07},
{0x3007, 0x01},
{0x3014, 0x03},
{0x3023, 0x05},
{0x3032, 0x34},
{0x3033, 0xfb},
{0x3054, 0x00},
{0x3055, 0x0f},
{0x3056, 0x01},
{0x3057, 0xff},
{0x3058, 0xbf},
{0x3059, 0x44},
{0x305a, 0x02},
{0x305b, 0x00},
{0x305c, 0x30},
{0x305d, 0x1d},
{0x305e, 0x16},
{0x305f, 0x18},
{0x3060, 0xf9},
{0x3061, 0xf0},
#ifdef OV10640_FSIN_ENABLE
{0x308c, 0xb2},
#else
{0x308c, 0x03},
#endif
{0x308f, 0x20},
{0x3090, 0x00},
{0x3091, 0x00},
{0x30eb, 0x00},
{0x30a3, 0x08},
{0x30ad, 0x03},
{0x30ae, 0x80},
{0x30af, 0x80},
{0x30b0, 0xff},
{0x30b1, 0x3f},
{0x30b2, 0x22},
{0x30b9, 0x22},
{0x30bb, 0x00},
{0x30bc, 0x00},
{0x30bd, 0x00},
{0x30be, 0x00},
{0x30bf, 0x00},
{0x30c0, 0x00},
{0x30c1, 0x00},
{0x30c2, 0x00},
{0x30c3, 0x00},
{0x30c4, 0x80},
{0x30c5, 0x00},
{0x30c6, 0x80},
{0x30c7, 0x00},
{0x30c8, 0x80},
{0x3119, 0x45},
{0x311a, 0x01},
{0x311b, 0x4a},
{0x3074, 0x00},
{0x3075, 0x00},
{0x3076, 0x00},
{0x3077, 0x3e},
{0x3078, 0x05},
{0x3079, 0x07},
{0x307a, 0x04},
{0x307b, 0x05},
{0x307c, 0x05},
{0x307d, 0x00},
{0x307e, 0x03},
{0x307f, 0xc0},
{0x3080, 0x07},
{0x3081, 0x43},
{0x3082, 0x03},
{0x3083, 0xec},
{0x3084, 0x00},
{0x3085, 0x02},
{0x3086, 0x00},
{0x3087, 0x04},
{0x3088, 0x00},
{0x3089, 0x40},
{0x308d, 0x92},
{0x3094, 0xa5},
{0x30e6, 0x03},
{0x30e7, 0xe8},
{0x30e8, 0x03},
{0x30e9, 0xe8},
{0x30e9, 0x05},
{0x30ec, 0x01},
{0x30fa, 0x06},
{0x3120, 0x00},
{0x3121, 0x01},
{0x3122, 0x00},
{0x3123, 0x02}, /* invert VSYNC polarity */
{0x3127, 0x63},
{0x3128, 0xc0},
#ifdef OV10640_DISPLAY_PATTERN
{0x3129, 0x80},
#else
{0x3129, 0x00},
#endif
{0x31be, 0x00},
{0x30a5, 0x78},
{0x30a6, 0x40},
{0x30a7, 0x78},
{0x30a8, 0x80},
{0x30a9, 0x78},
{0x30aa, 0xe0},
{0x30ab, 0x79},
{0x30ac, 0xc0},
{0x3440, 0x04},
{0x3444, 0x28},
{0x344e, 0x2c},
{0x3457, 0x33},
{0x345e, 0x38},
{0x3461, 0xa8},
{0x7002, 0xaa},
{0x7001, 0xdf},
{0x7048, 0x00},
{0x7049, 0x02},
{0x704a, 0x02},
{0x704b, 0x00},
{0x704c, 0x01},
{0x704d, 0x00},
{0x7043, 0x04},
{0x7040, 0x3c},
{0x7047, 0x00},
{0x7044, 0x01},
{0x7000, 0x1f},
{0x7084, 0x01},
{0x7085, 0x03},
{0x7086, 0x02},
{0x7087, 0x40},
{0x7088, 0x01},
{0x7089, 0x20},
{0x707f, 0x04},
{0x707c, 0x3c},
{0x7083, 0x00},
{0x7080, 0x01},
{0x7003, 0xdf},
{0x70c0, 0x00},
{0x70c1, 0x02},
{0x70c2, 0x02},
{0x70c3, 0x00},
{0x70c4, 0x01},
{0x70c5, 0x00},
{0x70b8, 0x03},
{0x70b9, 0x98},
{0x70bc, 0x00},
{0x70bd, 0x80},
{0x7004, 0x02},
{0x7005, 0x00},
{0x7006, 0x01},
{0x7007, 0x80},
{0x7008, 0x02},
{0x7009, 0x00},
{0x700a, 0x04},
{0x700b, 0x00},
{0x700e, 0x00},
{0x700f, 0x60},
{0x701a, 0x02},
{0x701b, 0x00},
{0x701c, 0x01},
{0x701d, 0x80},
{0x701e, 0x02},
{0x701f, 0x00},
{0x7020, 0x04},
{0x7021, 0x00},
{0x7024, 0x00},
{0x7025, 0x60},
{0x70e7, 0x00},
{0x70e4, 0x10},
{0x70e5, 0x00},
{0x70e6, 0x00},
{0x70eb, 0x00},
{0x70e8, 0x10},
{0x70e9, 0x00},
{0x70ea, 0x00},
{0x70ef, 0x00},
{0x70ec, 0xfd},
{0x70ed, 0x00},
{0x70ee, 0x00},
{0x70eb, 0x00},
{0x70f0, 0xfd},
{0x70f1, 0x00},
{0x70f2, 0x00},
{0x30fb, 0x06},
{0x30fc, 0x80},
{0x30fd, 0x02},
{0x30fe, 0x93},
{0x6000, 0xc1},
{0x6001, 0xb9},
{0x6002, 0xba},
{0x6003, 0xa4},
{0x6004, 0xb5},
{0x6005, 0xa0},
{0x6006, 0x82},
{0x6007, 0xa7},
{0x6008, 0xb7},
{0x6009, 0x5c},
{0x600a, 0x9e},
{0x600b, 0xc0},
{0x600c, 0xd2},
{0x600d, 0x33},
{0x600e, 0xcc},
{0x600f, 0xde},
{0x6010, 0xc1},
{0x6011, 0xab},
{0x6012, 0xb7},
{0x6013, 0x00},
{0x6014, 0x00},
{0x6015, 0x00},
{0x6016, 0x00},
{0x6017, 0x00},
{0x6018, 0x00},
{0x6019, 0x00},
{0x601a, 0x00},
{0x601b, 0x00},
{0x601c, 0x00},
{0x601d, 0xc5},
{0x601e, 0x54},
{0x601f, 0x9c},
{0x6020, 0x94},
{0x6021, 0x90},
{0x6022, 0x2a},
{0x6023, 0x61},
{0x6024, 0xd2},
{0x6025, 0xcc},
{0x6026, 0x02},
{0x6027, 0x35},
{0x6028, 0xb1},
{0x6029, 0xb2},
{0x602a, 0xb3},
{0x602b, 0xd2},
{0x602c, 0xd3},
{0x602d, 0x0a},
{0x602e, 0x31},
{0x602f, 0xcc},
{0x6030, 0x05},
{0x6031, 0xc4},
{0x6032, 0xd2},
{0x6033, 0xce},
{0x6034, 0x17},
{0x6035, 0xcf},
{0x6036, 0x1d},
{0x6037, 0xd0},
{0x6038, 0x23},
{0x6039, 0xd2},
{0x603a, 0xbc},
{0x603b, 0xcc},
{0x603c, 0x51},
{0x603d, 0xc5},
{0x603e, 0xd2},
{0x603f, 0x00},
{0x6040, 0x2b},
{0x6041, 0xcc},
{0x6042, 0x09},
{0x6043, 0xd2},
{0x6044, 0x1a},
{0x6045, 0xcc},
{0x6046, 0xeb},
{0x6047, 0x12},
{0x6048, 0x2a},
{0x6049, 0xba},
{0x604a, 0x56},
{0x604b, 0xd3},
{0x604c, 0x27},
{0x604d, 0x54},
{0x604e, 0xd4},
{0x604f, 0xc1},
{0x6050, 0x26},
{0x6051, 0xd2},
{0x6052, 0x01},
{0x6053, 0xd3},
{0x6054, 0x2f},
{0x6055, 0x27},
{0x6056, 0x08},
{0x6057, 0x1a},
{0x6058, 0xcc},
{0x6059, 0xd9},
{0x605a, 0x12},
{0x605b, 0x2c},
{0x605c, 0x11},
{0x605d, 0x60},
{0x605e, 0x50},
{0x605f, 0xc2},
{0x6060, 0xb9},
{0x6061, 0xa5},
{0x6062, 0xb5},
{0x6063, 0xa0},
{0x6064, 0x82},
{0x6065, 0x5c},
{0x6066, 0xd4},
{0x6067, 0xc1},
{0x6068, 0xd4},
{0x6069, 0xc1},
{0x606a, 0xd3},
{0x606b, 0x01},
{0x606c, 0x7c},
{0x606d, 0x74},
{0x606e, 0x00},
{0x606f, 0x2a},
{0x6070, 0x61},
{0x6071, 0xd2},
{0x6072, 0xcc},
{0x6073, 0xde},
{0x6074, 0xc6},
{0x6075, 0xd2},
{0x6076, 0xcc},
{0x6077, 0x02},
{0x6078, 0x35},
{0x6079, 0xd3},
{0x607a, 0x0f},
{0x607b, 0x31},
{0x607c, 0xcc},
{0x607d, 0x05},
{0x607e, 0xc5},
{0x607f, 0xd2},
{0x6080, 0xbb},
{0x6081, 0xcc},
{0x6082, 0x17},
{0x6083, 0xd2},
{0x6084, 0xbd},
{0x6085, 0xcc},
{0x6086, 0x51},
{0x6087, 0xc6},
{0x6088, 0xd2},
{0x6089, 0x2b},
{0x608a, 0xcc},
{0x608b, 0x09},
{0x608c, 0xd2},
{0x608d, 0x1a},
{0x608e, 0xcc},
{0x608f, 0xeb},
{0x6090, 0x71},
{0x6091, 0x12},
{0x6092, 0x2a},
{0x6093, 0xd3},
{0x6094, 0x24},
{0x6095, 0x00},
{0x6096, 0x00},
{0x6097, 0x70},
{0x6098, 0xca},
{0x6099, 0x26},
{0x609a, 0xd2},
{0x609b, 0x01},
{0x609c, 0xd3},
{0x609d, 0x2f},
{0x609e, 0x27},
{0x609f, 0x08},
{0x60a0, 0x1a},
{0x60a1, 0x12},
{0x60a2, 0xcc},
{0x60a3, 0xd9},
{0x60a4, 0x60},
{0x60a5, 0x2c},
{0x60a6, 0x11},
{0x60a7, 0x50},
{0x60a8, 0x00},
{0x60a9, 0x00},
{0x60aa, 0xc0},
{0x60ab, 0xb9},
{0x60ac, 0xa3},
{0x60ad, 0xb5},
{0x60ae, 0xb5},
{0x60af, 0x00},
{0x60b0, 0xa0},
{0x60b1, 0x82},
{0x60b2, 0x5c},
{0x60b3, 0xd4},
{0x60b4, 0xa6},
{0x60b5, 0x9d},
{0x60b6, 0xd3},
{0x60b7, 0x34},
{0x60b8, 0xb0},
{0x60b9, 0xb7},
{0x60ba, 0x00},
{0x60bb, 0xd3},
{0x60bc, 0x0a},
{0x60bd, 0xd3},
{0x60be, 0x10},
{0x60bf, 0x9c},
{0x60c0, 0x94},
{0x60c1, 0x90},
{0x60c2, 0xc8},
{0x60c3, 0xba},
{0x60c4, 0x7c},
{0x60c5, 0x74},
{0x60c6, 0x00},
{0x60c7, 0x2a},
{0x60c8, 0x61},
{0x60c9, 0x00},
{0x60ca, 0xd2},
{0x60cb, 0xcc},
{0x60cc, 0xde},
{0x60cd, 0xc4},
{0x60ce, 0xd2},
{0x60cf, 0xcc},
{0x60d0, 0x02},
{0x60d1, 0x35},
{0x60d2, 0xd2},
{0x60d3, 0xcc},
{0x60d4, 0x14},
{0x60d5, 0xd3},
{0x60d6, 0x09},
{0x60d7, 0x31},
{0x60d8, 0xd2},
{0x60d9, 0xcc},
{0x60da, 0x05},
{0x60db, 0xd2},
{0x60dc, 0xbb},
{0x60dd, 0xcc},
{0x60de, 0x19},
{0x60df, 0xd2},
{0x60e0, 0xbe},
{0x60e1, 0xce},
{0x60e2, 0x51},
{0x60e3, 0xcf},
{0x60e4, 0x54},
{0x60e5, 0xd0},
{0x60e6, 0x58},
{0x60e7, 0xd3},
{0x60e8, 0x01},
{0x60e9, 0x2b},
{0x60ea, 0xcc},
{0x60eb, 0x09},
{0x60ec, 0xd2},
{0x60ed, 0xd9},
{0x60ee, 0xd3},
{0x60ef, 0xda},
{0x60f0, 0xd7},
{0x60f1, 0x1a},
{0x60f2, 0xcc},
{0x60f3, 0xeb},
{0x60f4, 0x12},
{0x60f5, 0xd4},
{0x60f6, 0xaf},
{0x60f7, 0x27},
{0x60f8, 0x00},
{0x60f9, 0xd2},
{0x60fa, 0xd3},
{0x60fb, 0x3b},
{0x60fc, 0xd9},
{0x60fd, 0xe0},
{0x60fe, 0xda},
{0x60ff, 0xe4},
{0x6100, 0x1a},
{0x6101, 0x12},
{0x6102, 0xcc},
{0x6103, 0xd9},
{0x6104, 0x60},
{0x6105, 0x10},
{0x6106, 0x2c},
{0x6107, 0x5d},
{0x6108, 0xd3},
{0x6109, 0x0a},
{0x610a, 0x5c},
{0x610b, 0x01},
{0x610c, 0x50},
{0x610d, 0x11},
{0x610e, 0xd6},
{0x610f, 0xb7},
{0x6110, 0xb9},
{0x6111, 0xba},
{0x6112, 0xaf},
{0x6113, 0xdc},
{0x6114, 0xcb},
{0x6115, 0xc3},
{0x6116, 0xb9},
{0x6117, 0xa4},
{0x6118, 0xb5},
{0x6119, 0x5c},
{0x611a, 0x12},
{0x611b, 0x2a},
{0x611c, 0x61},
{0x611d, 0xd2},
{0x611e, 0xcc},
{0x611f, 0xe2},
{0x6120, 0x35},
{0x6121, 0xc7},
{0x6122, 0xd2},
{0x6123, 0x31},
{0x6124, 0xcc},
{0x6125, 0x05},
{0x6126, 0xc6},
{0x6127, 0xbb},
{0x6128, 0xd2},
{0x6129, 0xcc},
{0x612a, 0x17},
{0x612b, 0xd2},
{0x612c, 0xbe},
{0x612d, 0xcc},
{0x612e, 0x51},
{0x612f, 0xc7},
{0x6130, 0xd2},
{0x6131, 0xcc},
{0x6132, 0x09},
{0x6133, 0xb4},
{0x6134, 0xb7},
{0x6135, 0x94},
{0x6136, 0xd2},
{0x6137, 0x12},
{0x6138, 0x26},
{0x6139, 0x42},
{0x613a, 0x46},
{0x613b, 0x42},
{0x613c, 0xd3},
{0x613d, 0x20},
{0x613e, 0x27},
{0x613f, 0x00},
{0x6140, 0x1a},
{0x6141, 0xcc},
{0x6142, 0xd9},
{0x6143, 0x60},
{0x6144, 0x2c},
{0x6145, 0x11},
{0x6146, 0x40},
{0x6147, 0x50},
{0x6148, 0xb8},
{0x6149, 0x90},
{0x614a, 0xd5},
{0x614b, 0x00},
{0x614c, 0xba},
{0x614d, 0x00},
{0x614e, 0x00},
{0x614f, 0x00},
{0x6150, 0x00},
{0x6151, 0x00},
{0x6152, 0x00},
{0x6153, 0xaa},
{0x6154, 0xb7},
{0x6155, 0x00},
{0x6156, 0x00},
{0x6157, 0x00},
{0x6158, 0x00},
{0x6159, 0xa6},
{0x615a, 0xb7},
{0x615b, 0x00},
{0x615c, 0xd5},
{0x615d, 0x00},
{0x615e, 0x71},
{0x615f, 0xd3},
{0x6160, 0x3e},
{0x6161, 0xba},
{0x6162, 0x00},
{0x6163, 0x00},
{0x6164, 0x00},
{0x6165, 0x00},
{0x6166, 0xd3},
{0x6167, 0x10},
{0x6168, 0x70},
{0x6169, 0x00},
{0x616a, 0x00},
{0x616b, 0x00},
{0x616c, 0x00},
{0x616d, 0xd5},
{0x616e, 0xba},
{0x616f, 0xb0},
{0x6170, 0xb7},
{0x6171, 0x00},
{0x6172, 0x9d},
{0x6173, 0xd3},
{0x6174, 0x0a},
{0x6175, 0x9d},
{0x6176, 0x9d},
{0x6177, 0xd3},
{0x6178, 0x10},
{0x6179, 0x9c},
{0x617a, 0x94},
{0x617b, 0x90},
{0x617c, 0xc8},
{0x617d, 0xba},
{0x617e, 0xd2},
{0x617f, 0x30},
{0x6180, 0xd5},
{0x6181, 0x00},
{0x6182, 0xba},
{0x6183, 0xb0},
{0x6184, 0xb7},
{0x6185, 0x00},
{0x6186, 0x9d},
{0x6187, 0xd3},
{0x6188, 0x0a},
{0x6189, 0x9d},
{0x618a, 0x9d},
{0x618b, 0xd3},
{0x618c, 0x10},
{0x618d, 0x9c},
{0x618e, 0x94},
{0x618f, 0x90},
{0x6190, 0xc8},
{0x6191, 0xba},
{0x6192, 0xd5},
{0x6193, 0x00},
{0x6194, 0xba},
{0x6195, 0xb0},
{0x6196, 0xb7},
{0x6197, 0x00},
{0x6198, 0x9d},
{0x6199, 0xd3},
{0x619a, 0x0a},
{0x619b, 0x9d},
{0x619c, 0x9d},
{0x619d, 0xd3},
{0x619e, 0x10},
{0x619f, 0x9c},
{0x61a0, 0x94},
{0x61a1, 0x90},
{0x61a2, 0xc9},
{0x61a3, 0xba},
{0x61a4, 0xd5},
{0x61a5, 0x00},
{0x61a6, 0x00},
{0x61a7, 0x1a},
{0x61a8, 0x12},
{0x61a9, 0xcc},
{0x61aa, 0xeb},
{0x61ab, 0xd2},
{0x61ac, 0xd5},
{0x61ad, 0x00},
{0x61ae, 0x00},
{0x61af, 0x1a},
{0x61b0, 0x12},
{0x61b1, 0xcc},
{0x61b2, 0xeb},
{0x61b3, 0xd2},
{0x61b4, 0x1a},
{0x61b5, 0x12},
{0x61b6, 0xcc},
{0x61b7, 0xeb},
{0x61b8, 0xd2},
{0x61b9, 0x1a},
{0x61ba, 0x12},
{0x61bb, 0xcc},
{0x61bc, 0xeb},
{0x61bd, 0xd2},
{0x61be, 0xd5},
{0x61bf, 0x00},
{0x61c0, 0x00},
{0x61c1, 0x1a},
{0x61c2, 0xcc},
{0x61c3, 0xf0},
{0x61c4, 0x12},
{0x61c5, 0xd2},
{0x61c6, 0xd5},
{0x61c7, 0x00},
{0x61c8, 0x00},
{0x61c9, 0x1a},
{0x61ca, 0xcc},
{0x61cb, 0xf0},
{0x61cc, 0x12},
{0x61cd, 0xd2},
{0x61ce, 0x1a},
{0x61cf, 0xcc},
{0x61d0, 0xf0},
{0x61d1, 0x12},
{0x61d2, 0xd2},
{0x61d3, 0x1a},
{0x61d4, 0xcc},
{0x61d5, 0xf0},
{0x61d6, 0x12},
{0x61d7, 0xd2},
{0x61d8, 0xd5},
{0x6400, 0x00},
{0x6401, 0x08},
{0x6402, 0x00},
{0x6403, 0xff},
{0x6404, 0x04},
{0x6405, 0x61},
{0x6406, 0x04},
{0x6407, 0x70},
{0x6408, 0x00},
{0x6409, 0xff},
{0x640a, 0x05},
{0x640b, 0x14},
{0x640c, 0x04},
{0x640d, 0x70},
{0x640e, 0x05},
{0x640f, 0x74},
{0x6410, 0x00},
{0x6411, 0xff},
{0x6412, 0x05},
{0x6413, 0x54},
{0x6414, 0x04},
{0x6415, 0x30},
{0x6416, 0x05},
{0x6417, 0x44},
{0x6418, 0x05},
{0x6419, 0x47},
{0x641a, 0x00},
{0x641b, 0xff},
{0x641c, 0x04},
{0x641d, 0x31},
{0x641e, 0x04},
{0x641f, 0x30},
{0x6420, 0x00},
{0x6421, 0xff},
{0x6422, 0x04},
{0x6423, 0x20},
{0x6424, 0x05},
{0x6425, 0x06},
{0x6426, 0x00},
{0x6427, 0xff},
{0x6428, 0x08},
{0x6429, 0x29},
{0x642a, 0x08},
{0x642b, 0x30},
{0x642c, 0x00},
{0x642d, 0xff},
{0x642e, 0x08},
{0x642f, 0x29},
{0x6430, 0x08},
{0x6431, 0x30},
{0x6432, 0x06},
{0x6433, 0x20},
{0x6434, 0x07},
{0x6435, 0x00},
{0x6436, 0x08},
{0x6437, 0x3f},
{0x6438, 0x00},
{0x6439, 0xff},
{0x643a, 0x08},
{0x643b, 0x29},
{0x643c, 0x08},
{0x643d, 0x35},
{0x643e, 0x06},
{0x643f, 0x10},
{0x6440, 0x07},
{0x6441, 0x00},
{0x6442, 0x08},
{0x6443, 0x3f},
{0x6444, 0x00},
{0x6445, 0xff},
{0x6446, 0x08},
{0x6447, 0x29},
{0x6448, 0x08},
{0x6449, 0x3a},
{0x644a, 0x06},
{0x644b, 0x00},
{0x644c, 0x07},
{0x644d, 0x00},
{0x644e, 0x08},
{0x644f, 0x3f},
{0x6450, 0x00},
{0x6451, 0xff},
{0x6452, 0x06},
{0x6453, 0x00},
{0x6454, 0x07},
{0x6455, 0x05},
{0x6456, 0x01},
{0x6457, 0xaf},
{0x6458, 0x01},
{0x6459, 0x0f},
{0x645a, 0x01},
{0x645b, 0x90},
{0x645c, 0x01},
{0x645d, 0xc8},
{0x645e, 0x00},
{0x645f, 0xff},
{0x6460, 0x01},
{0x6461, 0xac},
{0x6462, 0x01},
{0x6463, 0x0c},
{0x6464, 0x01},
{0x6465, 0x90},
{0x6466, 0x01},
{0x6467, 0xe8},
{0x6468, 0x00},
{0x6469, 0xff},
{0x646a, 0x01},
{0x646b, 0xad},
{0x646c, 0x01},
{0x646d, 0x0d},
{0x646e, 0x01},
{0x646f, 0x90},
{0x6470, 0x01},
{0x6471, 0xe8},
{0x6472, 0x00},
{0x6473, 0xff},
{0x6474, 0x01},
{0x6475, 0xae},
{0x6476, 0x01},
{0x6477, 0x0e},
{0x6478, 0x01},
{0x6479, 0x90},
{0x647a, 0x01},
{0x647b, 0xe8},
{0x647c, 0x00},
{0x647d, 0xff},
{0x647e, 0x01},
{0x647f, 0xb0},
{0x6480, 0x01},
{0x6481, 0xb1},
{0x6482, 0x01},
{0x6483, 0xb2},
{0x6484, 0x01},
{0x6485, 0xb3},
{0x6486, 0x01},
{0x6487, 0xb4},
{0x6488, 0x01},
{0x6489, 0xb5},
{0x648a, 0x01},
{0x648b, 0xb6},
{0x648c, 0x01},
{0x648d, 0xb7},
{0x648e, 0x01},
{0x648f, 0xb8},
{0x6490, 0x01},
{0x6491, 0xb9},
{0x6492, 0x01},
{0x6493, 0xba},
{0x6494, 0x01},
{0x6495, 0xbb},
{0x6496, 0x01},
{0x6497, 0xbc},
{0x6498, 0x01},
{0x6499, 0xbd},
{0x649a, 0x01},
{0x649b, 0xbe},
{0x649c, 0x01},
{0x649d, 0xbf},
{0x649e, 0x01},
{0x649f, 0xc0},
{0x64a0, 0x00},
{0x64a1, 0xff},
{0x64a2, 0x06},
{0x64a3, 0x00},
{0x64a4, 0x01},
{0x64a5, 0xf6},
{0x64a6, 0x00},
{0x64a7, 0xff},
{0x64a8, 0x06},
{0x64a9, 0x10},
{0x64aa, 0x01},
{0x64ab, 0xf6},
{0x64ac, 0x06},
{0x64ad, 0x00},
{0x64ae, 0x00},
{0x64af, 0xff},
{0x64b0, 0x06},
{0x64b1, 0x20},
{0x64b2, 0x01},
{0x64b3, 0xf6},
{0x64b4, 0x06},
{0x64b5, 0x00},
{0x64b6, 0x00},
{0x64b7, 0xff},
{0x64b8, 0x04},
{0x64b9, 0x31},
{0x64ba, 0x04},
{0x64bb, 0x30},
{0x64bc, 0x01},
{0x64bd, 0x20},
{0x64be, 0x01},
{0x64bf, 0x31},
{0x64c0, 0x01},
{0x64c1, 0x32},
{0x64c2, 0x01},
{0x64c3, 0x33},
{0x64c4, 0x01},
{0x64c5, 0x34},
{0x64c6, 0x01},
{0x64c7, 0x35},
{0x64c8, 0x01},
{0x64c9, 0x36},
{0x64ca, 0x01},
{0x64cb, 0x37},
{0x64cc, 0x01},
{0x64cd, 0x38},
{0x64ce, 0x01},
{0x64cf, 0x39},
{0x64d0, 0x01},
{0x64d1, 0x3a},
{0x64d2, 0x01},
{0x64d3, 0x3b},
{0x64d4, 0x01},
{0x64d5, 0x3c},
{0x64d6, 0x01},
{0x64d7, 0x3d},
{0x64d8, 0x01},
{0x64d9, 0x3e},
{0x64da, 0x01},
{0x64db, 0x3f},
{0x64dc, 0x02},
{0x64dd, 0xa0},
{0x64de, 0x00},
{0x64df, 0xff},
{0x64e0, 0x04},
{0x64e1, 0x31},
{0x64e2, 0x04},
{0x64e3, 0x30},
{0x64e4, 0x01},
{0x64e5, 0x00},
{0x64e6, 0x01},
{0x64e7, 0x11},
{0x64e8, 0x01},
{0x64e9, 0x12},
{0x64ea, 0x01},
{0x64eb, 0x13},
{0x64ec, 0x01},
{0x64ed, 0x14},
{0x64ee, 0x01},
{0x64ef, 0x15},
{0x64f0, 0x01},
{0x64f1, 0x16},
{0x64f2, 0x01},
{0x64f3, 0x17},
{0x64f4, 0x01},
{0x64f5, 0x18},
{0x64f6, 0x01},
{0x64f7, 0x19},
{0x64f8, 0x01},
{0x64f9, 0x1a},
{0x64fa, 0x01},
{0x64fb, 0x1b},
{0x64fc, 0x01},
{0x64fd, 0x1c},
{0x64fe, 0x01},
{0x64ff, 0x1d},
{0x6500, 0x01},
{0x6501, 0x1e},
{0x6502, 0x01},
{0x6503, 0x1f},
{0x6504, 0x02},
{0x6505, 0xa0},
{0x6506, 0x00},
{0x6507, 0xff},
{0x6508, 0x03},
{0x6509, 0x0b},
{0x650a, 0x05},
{0x650b, 0x86},
{0x650c, 0x00},
{0x650d, 0x00},
{0x650e, 0x05},
{0x650f, 0x06},
{0x6510, 0x00},
{0x6511, 0x04},
{0x6512, 0x05},
{0x6513, 0x04},
{0x6514, 0x00},
{0x6515, 0x04},
{0x6516, 0x05},
{0x6517, 0x00},
{0x6518, 0x05},
{0x6519, 0x08},
{0x651a, 0x03},
{0x651b, 0x9a},
{0x651c, 0x05},
{0x651d, 0x86},
{0x651e, 0x00},
{0x651f, 0x00},
{0x6520, 0x05},
{0x6521, 0x06},
{0x6522, 0x00},
{0x6523, 0x01},
{0x6524, 0x05},
{0x6525, 0x04},
{0x6526, 0x00},
{0x6527, 0x04},
{0x6528, 0x05},
{0x6529, 0x00},
{0x652a, 0x05},
{0x652b, 0x08},
{0x652c, 0x03},
{0x652d, 0x99},
{0x652e, 0x05},
{0x652f, 0x06},
{0x6530, 0x00},
{0x6531, 0x00},
{0x6532, 0x05},
{0x6533, 0x04},
{0x6534, 0x00},
{0x6535, 0x04},
{0x6536, 0x05},
{0x6537, 0x00},
{0x6538, 0x05},
{0x6539, 0x08},
{0x653a, 0x03},
{0x653b, 0x98},
{0x653c, 0x05},
{0x653d, 0x06},
{0x653e, 0x00},
{0x653f, 0x00},
{0x6540, 0x05},
{0x6541, 0x04},
{0x6542, 0x00},
{0x6543, 0x04},
{0x6544, 0x05},
{0x6545, 0x00},
{0x6546, 0x05},
{0x6547, 0x08},
{0x6548, 0x03},
{0x6549, 0x97},
{0x654a, 0x05},
{0x654b, 0x06},
{0x654c, 0x05},
{0x654d, 0x04},
{0x654e, 0x00},
{0x654f, 0x04},
{0x6550, 0x05},
{0x6551, 0x00},
{0x6552, 0x05},
{0x6553, 0x08},
{0x6554, 0x03},
{0x6555, 0x96},
{0x6556, 0x05},
{0x6557, 0x06},
{0x6558, 0x05},
{0x6559, 0x04},
{0x655a, 0x00},
{0x655b, 0x04},
{0x655c, 0x05},
{0x655d, 0x00},
{0x655e, 0x05},
{0x655f, 0x08},
{0x6560, 0x03},
{0x6561, 0x95},
{0x6562, 0x05},
{0x6563, 0x06},
{0x6564, 0x05},
{0x6565, 0x04},
{0x6566, 0x00},
{0x6567, 0x04},
{0x6568, 0x05},
{0x6569, 0x00},
{0x656a, 0x05},
{0x656b, 0x08},
{0x656c, 0x03},
{0x656d, 0x94},
{0x656e, 0x05},
{0x656f, 0x06},
{0x6570, 0x00},
{0x6571, 0x00},
{0x6572, 0x05},
{0x6573, 0x04},
{0x6574, 0x00},
{0x6575, 0x04},
{0x6576, 0x05},
{0x6577, 0x00},
{0x6578, 0x05},
{0x6579, 0x08},
{0x657a, 0x03},
{0x657b, 0x93},
{0x657c, 0x05},
{0x657d, 0x06},
{0x657e, 0x00},
{0x657f, 0x00},
{0x6580, 0x05},
{0x6581, 0x04},
{0x6582, 0x00},
{0x6583, 0x04},
{0x6584, 0x05},
{0x6585, 0x00},
{0x6586, 0x05},
{0x6587, 0x08},
{0x6588, 0x03},
{0x6589, 0x92},
{0x658a, 0x05},
{0x658b, 0x06},
{0x658c, 0x05},
{0x658d, 0x04},
{0x658e, 0x00},
{0x658f, 0x04},
{0x6590, 0x05},
{0x6591, 0x00},
{0x6592, 0x05},
{0x6593, 0x08},
{0x6594, 0x03},
{0x6595, 0x91},
{0x6596, 0x05},
{0x6597, 0x06},
{0x6598, 0x05},
{0x6599, 0x04},
{0x659a, 0x00},
{0x659b, 0x04},
{0x659c, 0x05},
{0x659d, 0x00},
{0x659e, 0x05},
{0x659f, 0x08},
{0x65a0, 0x03},
{0x65a1, 0x90},
{0x65a2, 0x05},
{0x65a3, 0x06},
{0x65a4, 0x05},
{0x65a5, 0x04},
{0x65a6, 0x00},
{0x65a7, 0x04},
{0x65a8, 0x05},
{0x65a9, 0x00},
{0x65aa, 0x05},
{0x65ab, 0x08},
{0x65ac, 0x02},
{0x65ad, 0x90},
{0x65ae, 0x05},
{0x65af, 0x06},
{0x65b0, 0x00},
{0x65b1, 0xff},
{0x65b2, 0x04},
{0x65b3, 0x20},
{0x65b4, 0x05},
{0x65b5, 0x06},
{0x65b6, 0x08},
{0x65b7, 0x84},
{0x65b8, 0x04},
{0x65b9, 0x04},
{0x65ba, 0x00},
{0x65bb, 0xff},
{0x65bc, 0x08},
{0x65bd, 0x72},
{0x65be, 0x04},
{0x65bf, 0x0c},
{0x65c0, 0x04},
{0x65c1, 0x04},
{0x65c2, 0x00},
{0x65c3, 0xff},
{0x65c4, 0x04},
{0x65c5, 0x45},
{0x65c6, 0x04},
{0x65c7, 0x54},
{0x65c8, 0x08},
{0x65c9, 0x72},
{0x65ca, 0x00},
{0x65cb, 0xff},
{0x65cc, 0x04},
{0x65cd, 0x20},
{0x65ce, 0x05},
{0x65cf, 0x06},
{0x65d0, 0x08},
{0x65d1, 0x96},
{0x65d2, 0x08},
{0x65d3, 0x5e},
{0x65d4, 0x00},
{0x65d5, 0xff},
{0x65d6, 0x04},
{0x65d7, 0x20},
{0x65d8, 0x05},
{0x65d9, 0x06},
{0x65da, 0x08},
{0x65db, 0x96},
{0x65dc, 0x08},
{0x65dd, 0x5c},
{0x65de, 0x00},
{0x65df, 0xff},
{0x65e0, 0x04},
{0x65e1, 0x20},
{0x65e2, 0x05},
{0x65e3, 0x06},
{0x65e4, 0x08},
{0x65e5, 0x84},
{0x65e6, 0x08},
{0x65e7, 0x70},
{0x65e8, 0x00},
{0x65e9, 0xff},
{0x65ea, 0x00},
{0x65eb, 0xff},
{0x65ec, 0x00},
{0x65ed, 0xff},
{0x30eb, 0x04},
{0x30ed, 0x5a},
{0x30ee, 0x01},
{0x30ef, 0x80},
{0x30f1, 0x5a},
{0x303a, 0x04},
{0x303b, 0x7f},
{0x303c, 0xfe},
{0x303d, 0x19},
{0x303e, 0xd7},
{0x303f, 0x09},
{0x3040, 0x78},
{0x3042, 0x05},
{0x328a, 0x00},
{0x31bf, 0x9f},
{0x31c0, 0xff},
#if  0
{0x3012, 0x01},
{0x3012, 0x00},
{0x3119, 0x44},
//;{0x3132, 0x24},
//;{0x3128, 0xc0},
//;{0x328a, 0x02},
{0x3012, 0x01},
#else
/* HFLIP=1, VFLIP=0 */
{0x3128, 0xc0 | 0x1},
{0x3291, 0x01 | 0x2},
{0x3090, 0x4},
/* invert VSYNC polarity */
{0x3123, 0x02},
/* change settings to 1280x1080 COMB12 30 fps, 96MHz */
{0x3012, 0x0},
{0x3000, 0x3},
{0x3001, 0x50},
{0x3002, 0x0a},
{0x3004, 0x3},
{0x3005, 0x48},
{0x3006, 0x7},
{0x308f, 0x10},
{0x3127, 0x63},
{0x3074, OV10640_X_START >> 8},
{0x3075, OV10640_X_START & 0xff},
{0x3076, (OV10640_Y_START - OV10640_EMB_LINES / 2) >> 8},
{0x3077, (OV10640_Y_START - OV10640_EMB_LINES / 2) & 0xff},
{0x3078, OV10640_X_END >> 8},
{0x3079, OV10640_X_END & 0xff},
{0x307a, (OV10640_Y_END + OV10640_EMB_LINES / 2) >> 8},
{0x307b, (OV10640_Y_END + OV10640_EMB_LINES / 2) & 0xff},
{0x307c, OV10640_DEFAULT_WIDTH >> 8},
{0x307d, OV10640_DEFAULT_WIDTH & 0xff},
{0x307e, (OV10640_DEFAULT_HEIGHT + OV10640_EMB_LINES) >> 8},
{0x307f, (OV10640_DEFAULT_HEIGHT + OV10640_EMB_LINES) & 0xff},
{0x3080, (OV10640_SENSOR_WIDTH + 200) >> 8}, // HTS
{0x3081, (OV10640_SENSOR_WIDTH + 200) & 0xff},
{0x3082, (OV10640_SENSOR_HEIGHT + 208) >> 8}, //VTS
{0x3083, (OV10640_SENSOR_HEIGHT + 208) & 0xff},
{0x3084, 0x0},
{0x3085, 0x0},
{0x3086, 0x0},
{0x3087, 0x0},
{0x346d, 0x14},
{0x3444, 0x28},
{0x3091, 0x0C}, // embedded data, embedded stats
{0x3119, 0x44}, // COMB12
{0x3012, 0x1},
#endif
};
