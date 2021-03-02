/*
 * OmniVision ov10640 sensor camera wizard 1280x1080@30/BGGR/BT601/12bit
 *
 * Copyright (C) 2015-2019 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

//#define OV10640_DISPLAY_PATTERN
#define OV10640_FSIN_ENABLE

#define OV10640_DEFAULT_WIDTH	1280
#define OV10640_DEFAULT_HEIGHT	1080

#define OV10640_EMB_LINES	4 /* 2+2 emb lines at top and 2+2 stat lines at bottom */
#define OV10640_EMB_PADDED	(priv->emb_enable ? OV10640_EMB_LINES : 0) /* embedded data (SOF) and stats (EOF) */

#define OV10640_DELAY		0xffff

#define OV10640_MAX_WIDTH	1280
#define OV10640_MAX_HEIGHT	1080
#define OV10640_SENSOR_WIDTH	1292
#define OV10640_SENSOR_HEIGHT	1092

#define OV10640_X_START		((OV10640_SENSOR_WIDTH - OV10640_DEFAULT_WIDTH) / 2)
#define OV10640_Y_START		((OV10640_SENSOR_HEIGHT - OV10640_DEFAULT_HEIGHT) / 2)
#define OV10640_X_END		(OV10640_X_START + OV10640_DEFAULT_WIDTH - 1)
#define OV10640_Y_END		(OV10640_Y_START + OV10640_DEFAULT_HEIGHT - 1)

struct ov10640_reg {
	u16	reg;
	u8	val;
};

#include "ov10640_r1d.h"
#include "ov10640_r1e.h"
#include "ov10640_r1f.h"
