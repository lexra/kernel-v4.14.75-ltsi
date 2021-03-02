/*
 * OmniVision ov9716 sensor camera wizard 1392x976@30/BGGR/BT601/12bit
 *
 * Copyright (C) 2020 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

//#define OV9716_DISPLAY_PATTERN
#define OV9716_FSIN_ENABLE

#define OV9716_DEFAULT_WIDTH	1280
#define OV9716_DEFAULT_HEIGHT	960

#define OV9716_EMB_LINES	2 /* 2 emb lines at top */
#define OV9716_EMB_PADDED	(priv->emb_enable ? OV9716_EMB_LINES : 0) /* embedded data (SOF) */

#define OV9716_DELAY		0xffff

#define OV9716_MAX_WIDTH	1408
#define OV9716_MAX_HEIGHT	976
#define OV9716_SENSOR_WIDTH	1408
#define OV9716_SENSOR_HEIGHT	992

#define OV9716_EXTRA_OFFSET	2 /* Looks max9286 silicon truncates height by 2 lines, hence enlarge */

#define OV9716_X_DEF_START	((OV9716_SENSOR_WIDTH - OV9716_DEFAULT_WIDTH) / 2)
#define OV9716_Y_DEF_START	((OV9716_SENSOR_HEIGHT - OV9716_DEFAULT_HEIGHT) / 2)
#define OV9716_X_DEF_END	(OV9716_X_START + OV9716_DEFAULT_WIDTH - 1)
#define OV9716_Y_DEF_END	(OV9716_Y_START + OV9716_DEFAULT_HEIGHT - 1 + OV9716_EXTRA_OFFSET)

#define OV9716_X_START		((OV9716_SENSOR_WIDTH - OV9716_MAX_WIDTH) / 2)
#define OV9716_Y_START		((OV9716_SENSOR_HEIGHT - OV9716_MAX_HEIGHT) / 2)
#define OV9716_X_END		(OV9716_X_START + OV9716_MAX_WIDTH - 1)
#define OV9716_Y_END		(OV9716_Y_START + OV9716_MAX_HEIGHT - 1 + OV9716_EXTRA_OFFSET)

struct ov9716_reg {
	u16	reg;
	u8	val;
};

#include "ov9716_r1e.h"
