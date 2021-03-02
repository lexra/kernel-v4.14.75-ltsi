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

/* 3exp HDR, Full Resolution, MIPI 4-lane 12-bit 36FPS, EXTCLK=23MHz (comes from deser) */
static const struct ar0xxx_reg *ar0220_regs_hdr_mipi_12bit_36fps_rev4[] = {
	ar0220_rev2_Reset,
	ar0220_rev3_Recommended_Settings,
	ar0220_rev2_pll_23_4lane_12b,
	ar0220_rev2_Readout_Mode_Configuration,
	ar0220_rev2_Full_Res_FOV,
	ar0220_rev2_hdr_Timing_and_Exposure,
	ar0220_rev2_hdr_12bit_output,
	ar0220_rev2_mipi_12bit_4lane,
	ar0220_rev2_Recommended_HDR_Settings,
	NULL,
};
