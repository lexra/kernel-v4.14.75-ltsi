/*
 * ON Semiconductor AR0220 sensor camera wizard 1848x948@30/BGGR/MIPI
 *
 * Copyright (C) 2019 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

//#define AR0220_DISPLAY_PATTERN_FIXED
//#define AR0220_DISPLAY_PATTERN_COLOR_BAR

#define AR0220_DEFAULT_WIDTH	1792
#define AR0220_DEFAULT_HEIGHT	944

#define AR0220_MAX_WIDTH	1828
#define AR0220_MAX_HEIGHT	948
#define AR0220_SENSOR_WIDTH	1828
#define AR0220_SENSOR_HEIGHT	948

#define AR0220_X_START		((AR0220_SENSOR_WIDTH - AR0220_DEFAULT_WIDTH) / 2)
#define AR0220_Y_START		((AR0220_SENSOR_HEIGHT - AR0220_DEFAULT_HEIGHT) / 2)
#define AR0220_X_END		(AR0220_X_START + AR0220_DEFAULT_WIDTH - 1)
#define AR0220_Y_END		(AR0220_Y_START + AR0220_DEFAULT_HEIGHT - 1)

#include "ar0220_rev2.h"
#include "ar0220_rev3.h"
#include "ar0220_rev4.h"
