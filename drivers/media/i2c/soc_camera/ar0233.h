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

//#define AR0233_DISPLAY_PATTERN_FIXED
//#define AR0233_DISPLAY_PATTERN_COLOR_BAR

#define AR0233_DEFAULT_WIDTH	2048
#define AR0233_DEFAULT_HEIGHT	1280

#define AR0233_MAX_WIDTH	2048
#define AR0233_MAX_HEIGHT	1280
#define AR0233_SENSOR_WIDTH	2072
#define AR0233_SENSOR_HEIGHT	1296

#define AR0233_X_START		((AR0233_SENSOR_WIDTH - AR0233_DEFAULT_WIDTH) / 2)
#define AR0233_Y_START		((AR0233_SENSOR_HEIGHT - AR0233_DEFAULT_HEIGHT) / 2)
#define AR0233_X_END		(AR0233_X_START + AR0233_DEFAULT_WIDTH - 1)
#define AR0233_Y_END		(AR0233_Y_START + AR0233_DEFAULT_HEIGHT - 1)

#include "ar0233_rev1.h"
#include "ar0233_rev2.h"
