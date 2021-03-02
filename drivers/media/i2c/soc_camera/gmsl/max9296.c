/*
 * MAXIM max9296 GMSL2 driver
 *
 * Copyright (C) 2019-2020 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_graph.h>
#include <linux/reboot.h>
#include <linux/regmap.h>
#include <linux/videodev2.h>

#include <media/v4l2-common.h>
#include <media/v4l2-clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include "common.h"
#include "max9296.h"
#include "max9296_debug.h"

static char mbus_default[10] = "dvp"; /* mipi, dvp */

static int conf_link;
module_param(conf_link, int, 0644);
MODULE_PARM_DESC(conf_link, " Force configuration link. Used only if robust firmware flashing required (f.e. recovery)");

static int poc_trig;
module_param(poc_trig, int, 0644);
MODULE_PARM_DESC(poc_trig, " Use PoC triggering during RC setup. Useful on systems with dedicated PoC and unstable ser-des lock");

static int him;
module_param(him, int, 0644);
MODULE_PARM_DESC(him, " Use High-Immunity mode (default: leagacy mode)");

static int fsync_period;
module_param(fsync_period, int, 0644);
MODULE_PARM_DESC(fsync_period, " Frame sync period (default: 3.2MHz)");

static int hsync;
module_param(hsync, int, 0644);
MODULE_PARM_DESC(hsync, " HSYNC invertion (default: 0 - not inverted)");

static int vsync = 1;
module_param(vsync, int, 0644);
MODULE_PARM_DESC(vsync, " VSYNC invertion (default: 1 - inverted)");

static int gpio_resetb;
module_param(gpio_resetb, int, 0644);
MODULE_PARM_DESC(gpio_resetb, " Serializer GPIO reset (default: 0 - not used)");

static int active_low_resetb;
module_param(active_low_resetb, int, 0644);
MODULE_PARM_DESC(active_low_resetb, " Serializer GPIO reset level (default: 0 - active high)");

static int timeout_n = 100;
module_param(timeout_n, int, 0644);
MODULE_PARM_DESC(timeout_n, " Timeout of link detection (default: 100 retries)");

static int poc_delay = 50;
module_param(poc_delay, int, 0644);
MODULE_PARM_DESC(poc_delay, " Delay in ms after POC enable (default: 50 ms)");

static int bws;
module_param(bws, int, 0644);
MODULE_PARM_DESC(bws, " BWS mode (default: 0 - 24-bit gmsl packets)");

static int dbl = 1;
module_param(dbl, int, 0644);
MODULE_PARM_DESC(dbl, " DBL mode (default: 1 - DBL mode enabled)");

static int dt = MIPI_DT_YUV8;
module_param(dt, int, 0644);
MODULE_PARM_DESC(dt, " DataType (default: 0x1e - YUV8)");

static unsigned long crossbar = 0xba9876543210;
module_param(crossbar, ulong, 0644);
MODULE_PARM_DESC(crossbar, " Serializer crossbar setup (default: ba9876543210 - reversed)");

static int gmsl = MODE_GMSL2;
module_param(gmsl, int, 0644);
MODULE_PARM_DESC(gmsl, " GMSL mode (default: 2 - GMSL2)");

static char *mbus = mbus_default;
module_param(mbus, charp, 0644);
MODULE_PARM_DESC(mbus, " Interfaces mipi,dvp (default: dvp)");

static int gpio0 = -1, gpio1 = -1, gpio7 = -1, gpio8 = -1;
module_param(gpio0, int, 0644);
MODULE_PARM_DESC(gpio0, "  GPIO0 function select (default: GPIO0 tri-state)");
module_param(gpio1, int, 0644);
MODULE_PARM_DESC(gpio1, "  GPIO1 function select (default: GPIO1 tri-state)");
module_param(gpio7, int, 0644);
MODULE_PARM_DESC(gpio7, "  GPIO7 function select (default: GPIO7 tri-state)");
module_param(gpio8, int, 0644);
MODULE_PARM_DESC(gpio8, "  GPIO8 function select (default: GPIO8 tri-state)");

static const struct regmap_config max9296_regmap[] = {
	{
		/* max9296 */
		.reg_bits = 16,
		.val_bits = 8,
		.max_register = 0x1f03,
	}, {
		/* max9271/max96705 */
		.reg_bits = 8,
		.val_bits = 8,
		.max_register = 0xff,
	}, {
		/* max9695 */
		.reg_bits = 16,
		.val_bits = 8,
		.max_register = 0x1b03,
	}
};

static void max9296_write_remote_verify(struct max9296_priv *priv, int link_n, u8 reg, int val)
{
	struct max9296_link *link = priv->link[link_n];
	int timeout;

	for (timeout = 0; timeout < 10; timeout++) {
		int sts = 0;
		u8 val2 = 0;

		ser_write(reg, val);
		des_read(MAX9296_COMMON1, &sts);
		/* check ACKed */
		if (sts & BIT(link_n)) {
			ser_read(reg, &val2);
			if (val2 == val)
				break;
		}

		usleep_range(1000, 1500);
	}

	if (timeout >= 10)
		dev_err(&priv->client->dev, "timeout remote write acked\n");
}

static void max9296_reset_oneshot(struct max9296_priv *priv)
{
	int timeout;
	int reg = 0;

	des_update_bits(MAX9296_CTRL0, BIT(5), BIT(5)); /* set reset one-shot */

	/* wait for one-shot bit self-cleared */
	for (timeout = 0; timeout < 100; timeout++) {
		des_read(MAX9296_CTRL0, &reg);
		if (!(reg & BIT(5)))
			break;

		mdelay(1);
	}

	if (reg & BIT(5))
		dev_err(&priv->client->dev, "Failed reset oneshot\n");
}

/* -----------------------------------------------------------------------------
 * MIPI, mapping, routing
 */

static void max9296_pipe_override(struct max9296_priv *priv, unsigned int pipe,
				   unsigned int dt, unsigned int vc)
{
	int bpp, bank;

	bpp = mipi_dt_to_bpp(dt);
	bank = pipe / 4;
	pipe %= 4;

	if (priv->dbl == 1) {
		/* DBL=1 is MUX mode, DBL=0 is Normal mode */
		des_update_bits(MAX_BACKTOP27(bank), BIT(pipe + 4), BIT(pipe + 4));	/* enable MUX mode */
		bpp = bpp / 2;								/* divide because of MUX=1 */
	}

	switch (pipe) {
	case 0:
		/* Pipe X: 0 or 4 */
		des_update_bits(MAX_BACKTOP12(bank), 0x1f << 3, bpp << 3);
		des_update_bits(MAX_BACKTOP13(bank), 0x0f, vc);
		des_update_bits(MAX_BACKTOP15(bank), 0x3f, dt);
		des_update_bits(bank ? MAX_BACKTOP28(0) : MAX_BACKTOP22(0), BIT(6), BIT(6)); /* enalbe s/w override */
		break;
	case 1:
		/* Pipe Y: 1 or 5 */
		des_update_bits(MAX_BACKTOP18(bank), 0x1f, bpp);
		des_update_bits(MAX_BACKTOP13(bank), 0x0f << 4, vc << 4);
		des_update_bits(MAX_BACKTOP16(bank), 0x0f, dt & 0x0f);
		des_update_bits(MAX_BACKTOP15(bank), 0x03 << 6, (dt & 0x30) << 2);
		des_update_bits(bank ? MAX_BACKTOP28(0) : MAX_BACKTOP22(0), BIT(7), BIT(7)); /* enable s/w override */
		break;
	case 2:
		/* Pipe Z: 2 or 6 */
		des_update_bits(MAX_BACKTOP19(bank), 0x03, bpp & 0x03);
		des_update_bits(MAX_BACKTOP18(bank), 0xe0, (bpp & 0x1c) << 3);
		des_update_bits(MAX_BACKTOP14(bank), 0x0f, vc);
		des_update_bits(MAX_BACKTOP17(bank), 0x03, dt & 0x03);
		des_update_bits(MAX_BACKTOP16(bank), 0x0f << 4, (dt & 0x3c) << 2);
		des_update_bits(bank ? MAX_BACKTOP30(0) : MAX_BACKTOP25(0), BIT(6), BIT(6)); /* enable s/w override */
		break;
	case 3:
		/* Pipe U: 3 or 7 */
		des_update_bits(MAX_BACKTOP19(bank), 0xfc, bpp << 2);
		des_update_bits(MAX_BACKTOP14(bank), 0x0f << 4, vc << 4);
		des_update_bits(MAX_BACKTOP17(bank), 0x3f << 2, dt << 2);
		des_update_bits(bank ? MAX_BACKTOP30(0) : MAX_BACKTOP25(0), BIT(7), BIT(7)); /* enable s/w override */
		break;
	}
}

static void max9296_set_pipe_to_mipi_mapping(struct max9296_priv *priv,
					      unsigned int pipe, unsigned int map_n,
					      unsigned int in_dt, unsigned int in_vc,
					      unsigned int out_dt, unsigned int out_vc, unsigned int out_mipi)
{
	int offset = 2 * (map_n % 4);

	des_write(MAX_MIPI_MAP_SRC(pipe, map_n), (in_vc << 6) | in_dt);
	des_write(MAX_MIPI_MAP_DST(pipe, map_n), (out_vc << 6) | out_dt);
	des_update_bits(MAX_MIPI_MAP_DST_PHY(pipe, map_n / 4), 0x03 << offset, out_mipi << offset);
	des_update_bits(MAX_MIPI_TX11(pipe), BIT(map_n), BIT(map_n));	/* enable SRC_n to DST_n mapping */
}

static void max9296_mipi_setup(struct max9296_priv *priv)
{
	des_write(MAX9296_REG2, 0);		/* disable all pipes */

	des_write(MAX_MIPI_PHY0, 0x04);		/* MIPI Phy 2x4 mode */
	des_write(MAX_MIPI_PHY3, 0xe4);		/* Lane map: straight */
	des_write(MAX_MIPI_PHY4, 0xe4);		/* Lane map: straight */
	//des_write(MAX_MIPI_PHY5, 0x00);	/* HS_prepare time, non-inverted polarity */
	//des_write(MAX_MIPI_PHY6, 0x00);

	des_write(MAX_MIPI_TX10(1), 0xc0);	/* MIPI1: 4 lanes */
	des_write(MAX_MIPI_TX10(2), 0xc0);	/* MIPI2: 4 lanes */

	des_update_bits(MAX_BACKTOP22(0), 0x3f, ((priv->csi_rate[1] / 100) & 0x1f) | BIT(5)); /* MIPI rate */
	des_update_bits(MAX_BACKTOP25(0), 0x3f, ((priv->csi_rate[1] / 100) & 0x1f) | BIT(5));
	des_update_bits(MAX_BACKTOP28(0), 0x3f, ((priv->csi_rate[2] / 100) & 0x1f) | BIT(5));
	des_update_bits(MAX_BACKTOP31(0), 0x3f, ((priv->csi_rate[2] / 100) & 0x1f) | BIT(5));

	des_update_bits(MAX_MIPI_PHY2, 0xf0, 0xf0); /* enable all MIPI PHYs */
}

/* -----------------------------------------------------------------------------
 * GMSL1
 */

static int max9296_gmsl1_sensor_reset(struct max9296_priv *priv, int link_n, int reset_on)
{
	struct max9296_link *link = priv->link[link_n];

	if (priv->gpio_resetb < 1 || priv->gpio_resetb > 5)
		return -EINVAL;

	/* sensor reset/unreset */
	ser_write(0x0f, (0xfe & ~BIT(priv->gpio_resetb)) | /* set GPIOn value to reset/unreset */
		  ((priv->active_low_resetb ? BIT(priv->gpio_resetb) : 0) ^ reset_on));
	ser_write(0x0e, 0x42 | BIT(priv->gpio_resetb)); /* set GPIOn direction output */

	return 0;
}

static void max9296_gmsl1_cc_enable(struct max9296_priv *priv, int link, int on)
{
	des_update_bits(MAX_GMSL1_4(link), 0x03, on ? 0x03 : 0x00);
	usleep_range(2000, 2500);
}

static int max9296_gmsl1_get_link_lock(struct max9296_priv *priv, int link_n)
{
	int val = 0;

	des_read(MAX_GMSL1_CB(link_n), &val);

	return !!(val & BIT(0));
}

static void max9296_gmsl1_link_crossbar_setup(struct max9296_priv *priv, int link, int dt)
{
	/* Always decode reversed bus, since we always reverse on serializer (old imagers need this) */
	switch (dt) {
	case MIPI_DT_YUV8:
		des_write(MAX_CROSS(link, 0), 7);
		des_write(MAX_CROSS(link, 1), 6);
		des_write(MAX_CROSS(link, 2), 5);
		des_write(MAX_CROSS(link, 3), 4);
		des_write(MAX_CROSS(link, 4), 3);
		des_write(MAX_CROSS(link, 5), 2);
		des_write(MAX_CROSS(link, 6), 1);
		des_write(MAX_CROSS(link, 7), 0);

		if (priv->dbl == 0) {
			/* deserializer DBL=1 is MUX, DBL=0 is Normal */
			des_write(MAX_CROSS(link, 8), 15);
			des_write(MAX_CROSS(link, 9), 14);
			des_write(MAX_CROSS(link, 10), 13);
			des_write(MAX_CROSS(link, 11), 12);
			des_write(MAX_CROSS(link, 12), 11);
			des_write(MAX_CROSS(link, 13), 10);
			des_write(MAX_CROSS(link, 14), 9);
			des_write(MAX_CROSS(link, 15), 8);
		}
		break;
	case MIPI_DT_RAW12:
		des_write(MAX_CROSS(link, 0), 11);
		des_write(MAX_CROSS(link, 1), 10);
		des_write(MAX_CROSS(link, 2), 9);
		des_write(MAX_CROSS(link, 3), 8);
		des_write(MAX_CROSS(link, 4), 7);
		des_write(MAX_CROSS(link, 5), 6);
		des_write(MAX_CROSS(link, 6), 5);
		des_write(MAX_CROSS(link, 7), 4);
		des_write(MAX_CROSS(link, 8), 3);
		des_write(MAX_CROSS(link, 9), 2);
		des_write(MAX_CROSS(link, 10), 1);
		des_write(MAX_CROSS(link, 11), 0);

		if (priv->dbl == 0) {
			/* deserializer DBL=1 is MUX, DBL=0 is Normal */
			des_write(MAX_CROSS(link, 12), 23);
			des_write(MAX_CROSS(link, 13), 22);
			des_write(MAX_CROSS(link, 14), 21);
			des_write(MAX_CROSS(link, 15), 20);
			des_write(MAX_CROSS(link, 16), 19);
			des_write(MAX_CROSS(link, 17), 18);
			des_write(MAX_CROSS(link, 18), 17);
			des_write(MAX_CROSS(link, 19), 16);
			des_write(MAX_CROSS(link, 20), 15);
			des_write(MAX_CROSS(link, 21), 14);
			des_write(MAX_CROSS(link, 22), 13);
			des_write(MAX_CROSS(link, 23), 12);
		}
		break;
	default:
		dev_err(&priv->client->dev, "crossbar for dt %d is not supported\n", dt);
		break;
	}

	des_write(MAX_CROSS(link, 24), (priv->hsync ? 0x40 : 0) + 24);	/* invert HS polarity */
	des_write(MAX_CROSS(link, 25), (priv->vsync ? 0 : 0x40) + 25);	/* invert VS polarity */
	des_write(MAX_CROSS(link, 26), (priv->hsync ? 0x40 : 0) + 26);	/* invert DE polarity */
}

static void max9296_gmsl1_initial_setup(struct max9296_priv *priv)
{
	int i;

	des_write(MAX9296_REG6, 0x10);				/* set GMSL1 mode */
	des_write(MAX9296_REG1, 0x01);				/* 187.5M/3G */

	for (i = 0; i < priv->n_links; i++) {
		des_write(MAX_GMSL1_2(i), 0x03);			/* Autodetect serial data rate range */
		des_write(MAX_GMSL1_4(i), 0);				/* disable REV/FWD CC */
		des_update_bits(MAX_GMSL1_6(i), BIT(7), priv->him ? BIT(7) : 0); /* HIM/Legacy mode */
		des_write(MAX_GMSL1_7(i), (priv->dbl ? BIT(7) : 0) |	/* DBL mode */
					  (priv->bws ? BIT(5) : 0) |	/* BWS 32/24-bit */
					  (priv->hibw ? BIT(3) : 0) |	/* High-bandwidth mode */
					  (priv->hven ? BIT(2) : 0));	/* HS/VS encoding enable */
		des_write(MAX_GMSL1_D(i), 0);				/* disable artificial ACKs, RC conf disable */
		des_write(MAX_GMSL1_F(i), 0);				/* disable DE processing */
		des_write(MAX_GMSL1_96(i), (0x13 << 3) | 0x3);		/* color map: RAW12 double - i.e. bypass packet as is */
	}
}

static int max9296_gmsl1_reverse_channel_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];
	int ser_addrs[] = { 0x40 };					/* possible MAX9271/MAX96705 addresses on i2c bus */
	int lock_sts;
	int timeout = priv->timeout;
	char timeout_str[40];
	u8 val = 0;
	int ret = 0;

	des_write(MAX_GMSL1_D(link_n), 0x81);			/* enable artificial ACKs, RC conf mode */
	des_write(MAX_RLMSC5(link_n), 0xa0);			/* override RC pulse length */
	des_write(MAX_RLMSC4(link_n), 0x80);			/* override RC rise/fall time */
	usleep_range(2000, 2500);
	des_write(MAX_GMSL1_4(link_n), 0x3);			/* enable REV/FWD CC */
	des_write(MAX9296_GMSL1_EN, BIT(link_n));		/* enable GMSL link# */
	des_update_bits(MAX9296_CTRL0, 0x13, BIT(link_n));	/* enable GMSL link# */
	max9296_reset_oneshot(priv);
	usleep_range(2000, 2500);

	for (; timeout > 0; timeout--) {
		if (priv->him) {
			/* HIM mode setup */
			__reg8_write(ser_addrs[0], 0x4d, 0xc0);
			usleep_range(2000, 2500);
			__reg8_write(ser_addrs[0], 0x04, 0x43);	/* wake-up, enable RC, conf_link */
			usleep_range(2000, 2500);
			if (priv->bws) {
				__reg8_write(ser_addrs[0], 0x07, (priv->hven ? 0x04 : 0) |		/* HS/VS encoding enable */
								 (priv->pclk_rising_edge ? 0 : 0x10) |	/* PCLK edge */
								 (0x80) |				/* DBL=1 in serializer */
								 (priv->bws ? 0x20 : 0));		/* BWS 32/24-bit */
				usleep_range(2000, 2500);
			}
		} else {
			/* Legacy mode setup */
			des_write(MAX_RLMS95(link_n), 0x88);		/* override RC Tx amplitude */
			usleep_range(2000, 2500);

			__reg8_write(ser_addrs[0], 0x04, 0x43);		/* wake-up, enable RC, conf_link */
			usleep_range(2000, 2500);
			__reg8_write(ser_addrs[0], 0x08, 0x01);		/* RC receiver high threshold enable */
			__reg8_write(ser_addrs[0], 0x97, 0x5f);		/* enable RC programming (MAX96705-MAX96711 only) */
			usleep_range(2000, 2500);

			if (priv->bws) {
				__reg8_write(ser_addrs[0], 0x07, (priv->hven ? 0x04 : 0) |		/* HS/VS encoding enable */
								 (priv->pclk_rising_edge ? 0 : 0x10) |	/* PCLK edge */
								 (0x80) |				/* DBL=1 in serializer */
								 (priv->bws ? 0x20 : 0));		/* BWS 32/24-bit */
				usleep_range(2000, 2500);
			}

			des_write(MAX_RLMS95(link_n), 0xd3);	/* increase RC Tx amplitude */
			usleep_range(2000, 2500);
		}

		__reg8_read(ser_addrs[0], 0x1e, &val);
		if (val == MAX9271_ID || val == MAX96705_ID || val == MAX96707_ID) {
			link->ser_id = val;
			__reg8_write(ser_addrs[0], 0x00, link->ser_addr << 1);	 /* relocate serizlizer on I2C bus */
			usleep_range(2000, 2500);
			break;
		}

		/* Check if already initialized (after reboot/reset ?) */
		ser_read(0x1e, &val);
		if (val == MAX9271_ID || val == MAX96705_ID || val == MAX96707_ID) {
			link->ser_id = val;
			ser_write(0x04, 0x43);			/* enable RC, conf_link */
			usleep_range(2000, 2500);
			ret = -EADDRINUSE;
			break;
		}

		if (poc_trig) {
			if (!IS_ERR(link->poc_reg) && (timeout % poc_trig == 0)) {
				regulator_disable(link->poc_reg); /* POC power off */
				mdelay(200);
				ret = regulator_enable(link->poc_reg); /* POC power on */
				if (ret)
					dev_err(&link->client->dev, "failed to enable poc regulator\n");
				mdelay(priv->poc_delay);
			}
		}
	}

	max9296_gmsl1_sensor_reset(priv, link_n, 0);		/* sensor un-reset */

	des_write(MAX_GMSL1_D(link_n), 0);			/* disable artificial ACKs, RC conf disable */
	usleep_range(2000, 2500);
	des_read(MAX_GMSL1_CB(link_n), &lock_sts);
	lock_sts = !!(lock_sts & 0x01);

	if (!timeout) {
		ret = -ETIMEDOUT;
		goto out;
	}

	priv->links_mask |= BIT(link_n);

out:
	sprintf(timeout_str, " retries=%d lock_sts=%d", priv->timeout - timeout, lock_sts);
	dev_info(&priv->client->dev, "GMSL1 link%d %s %sat 0x%x %s %s\n", link_n, chip_name(link->ser_id),
			       ret == -EADDRINUSE ? "already " : "", link->ser_addr,
			       ret == -ETIMEDOUT ? "not found: timeout GMSL link establish" : "",
			       priv->timeout - timeout ? timeout_str : "");
	return ret;
}

static int max9296_gmsl1_link_serializer_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];

	/* GMSL setup */
	ser_write(0x0d, 0x22 | MAXIM_I2C_I2C_SPEED);		/* disable artificial ACK, I2C speed set */
	ser_write(0x07, (priv->hven ? 0x04 : 0) |		/* HS/VS encoding enable */
				       (priv->pclk_rising_edge ? 0 : 0x10) | /* PCLK edge */
				       (0x80) |				/* DBL=1 in serializer */
				       (priv->bws ? 0x20 : 0));		/* BWS 32/24-bit */
	usleep_range(2000, 2500);
	ser_write(0x02, 0xff);					/* spread spectrum +-4%, pclk range automatic, Gbps automatic */
	usleep_range(2000, 2500);

	if (link->ser_id != MAX9271_ID) {
		switch (priv->dt) {
		case MIPI_DT_YUV8:
			if (priv->dbl == 1) {
				/* setup crossbar for YUV8/RAW8: reverse DVP bus */
				ser_write(0x20, priv->cb[7]);
				ser_write(0x21, priv->cb[6]);
				ser_write(0x22, priv->cb[5]);
				ser_write(0x23, priv->cb[4]);
				ser_write(0x24, priv->cb[3]);
				ser_write(0x25, priv->cb[2]);
				ser_write(0x26, priv->cb[1]);
				ser_write(0x27, priv->cb[0]);

				/* this is second byte in the packet (DBL=1 in serializer always) */
				ser_write(0x30, priv->cb[7] + 16);
				ser_write(0x31, priv->cb[6] + 16);
				ser_write(0x32, priv->cb[5] + 16);
				ser_write(0x33, priv->cb[4] + 16);
				ser_write(0x34, priv->cb[3] + 16);
				ser_write(0x35, priv->cb[2] + 16);
				ser_write(0x36, priv->cb[1] + 16);
				ser_write(0x37, priv->cb[0] + 16);
			} else {
				/* setup crossbar for YUV8/RAW8: reversed DVP bus */
				ser_write(0x20, priv->cb[4]);
				ser_write(0x21, priv->cb[3]);
				ser_write(0x22, priv->cb[2]);
				ser_write(0x23, priv->cb[1]);
				ser_write(0x24, priv->cb[0]);
				ser_write(0x25, 0x40);
				ser_write(0x26, 0x40);
				if (link->ser_id == MAX96705_ID) {
					ser_write(0x27, 14); /* HS: D14->D18  */
					ser_write(0x28, 15); /* VS: D15->D19 */
					ser_write(0x29, 14); /* DE: D14->D20 */
				}
				if (link->ser_id == MAX96707_ID) {
					ser_write(0x27, 12); /* HS: D12->D18, this is a virtual NC pin, hence it is D14 at HS */
					ser_write(0x28, 13); /* VS: D13->D19 */
					ser_write(0x29, 12); /* DE: D12->D20 */
				}
				ser_write(0x2A, 0x40);

				/* this is second byte in the packet (DBL=1 in serializer) */
				ser_write(0x30, 0x10 + priv->cb[7]);
				ser_write(0x31, 0x10 + priv->cb[6]);
				ser_write(0x32, 0x10 + priv->cb[5]);
				ser_write(0x33, 0x10 + priv->cb[4]);
				ser_write(0x34, 0x10 + priv->cb[3]);
				ser_write(0x35, 0x10 + priv->cb[2]);
				ser_write(0x36, 0x10 + priv->cb[1]);
				ser_write(0x37, 0x10 + priv->cb[0]);
				ser_write(0x38, priv->cb[7]);
				ser_write(0x39, priv->cb[6]);
				ser_write(0x3A, priv->cb[5]);

				ser_write(0x67, 0xC4); /* DBL_ALIGN_TO = 100b */
			}
			break;
		case MIPI_DT_RAW12:
			/* setup crossbar for RAW12: reverse DVP bus */
			ser_write(0x20, priv->cb[11]);
			ser_write(0x21, priv->cb[10]);
			ser_write(0x22, priv->cb[9]);
			ser_write(0x23, priv->cb[8]);
			ser_write(0x24, priv->cb[7]);
			ser_write(0x25, priv->cb[6]);
			ser_write(0x26, priv->cb[5]);
			ser_write(0x27, priv->cb[4]);
			ser_write(0x28, priv->cb[3]);
			ser_write(0x29, priv->cb[2]);
			ser_write(0x2a, priv->cb[1]);
			ser_write(0x2b, priv->cb[0]);

			/* this is second byte in the packet (DBL=1 in serializer) */
			ser_write(0x30, priv->cb[11] + 16);
			ser_write(0x31, priv->cb[10] + 16);
			ser_write(0x32, priv->cb[9] + 16);
			ser_write(0x33, priv->cb[8] + 16);
			ser_write(0x34, priv->cb[7] + 16);
			ser_write(0x35, priv->cb[6] + 16);
			ser_write(0x36, priv->cb[5] + 16);
			ser_write(0x37, priv->cb[4] + 16);
			ser_write(0x38, priv->cb[3] + 16);
			ser_write(0x39, priv->cb[2] + 16);
			ser_write(0x3a, priv->cb[1] + 16);
			ser_write(0x3b, priv->cb[0] + 16);

			if (!(priv->bws || priv->hibw) && priv->dbl)
				dev_err(&priv->client->dev, " BWS must be 27/32-bit for RAW12 in DBL mode\n");
			break;
		}
	}

	/* I2C translator setup */
//	ser_write(0x09, OV490_I2C_ADDR_NEW << 1);	/* sensor I2C translated - must be set by sensor driver */
//	ser_write(0x0A, OV490_I2C_ADDR << 1);		/* sensor I2C native - must be set by sensor driver */
	ser_write(0x0B, BROADCAST << 1);		/* serializer broadcast I2C translated */
	ser_write(0x0C, link->ser_addr << 1);		/* serializer broadcast I2C native */
	/* put serializer in configuration link state  */
	ser_write(0x04, 0x43);				/* enable RC, conf_link */
	usleep_range(2000, 2500);

	return 0;
}

static void max9296_gmsl1_link_pipe_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];
	int pipe = link_n; /* straight map */
	int dt = priv->dt; /* should come from imager */
	int in_vc = 0;

	max9296_pipe_override(priv, pipe, dt, in_vc);		/* override dt, vc */

	des_write(MAX_MIPI_TX11(pipe), 0x00);			/* disable all mappings */
	des_write(MAX_MIPI_TX12(pipe), 0x00);

	/* use map #0 for payload data */
	max9296_set_pipe_to_mipi_mapping(priv, pipe, 0,	/* pipe, map# */
					  dt, in_vc,		/* src DT, VC */
					  dt, link->out_vc,	/* dst DT, VC */
					  link->out_mipi);	/* dst MIPI PHY */
	/* use map #1 for FS */
	max9296_set_pipe_to_mipi_mapping(priv, pipe, 1,	/* pipe, map# */
					  0x00, in_vc,		/* src DT, VC */
					  0x00, link->out_vc,	/* dst DT, VC */
					  link->out_mipi);	/* dst MIPI PHY */
	/* use map #2 for FE */
	max9296_set_pipe_to_mipi_mapping(priv, pipe, 2,	/* pipe, map# */
					  0x01, in_vc,		/* src DT, VC */
					  0x01, link->out_vc,	/* dst DT, VC */
					  link->out_mipi);	/* dst MIPI PHY */
	usleep_range(5000, 5500);

	link->pipes_mask |= BIT(pipe);
}

static void max9296_gmsl1_postinit(struct max9296_priv *priv)
{
	int i;
	u8 val = 0;

	for (i = 0; i < priv->n_links; i++) {
		struct max9296_link *link = priv->link[i];

		if (!(priv->links_mask & BIT(i)))
			continue;

		des_write(MAX_GMSL1_4(i), 0x3);			/* enable REV/FWD CC */
		des_write(MAX9296_GMSL1_EN, BIT(i));		/* enable GMSL link# */
		des_update_bits(MAX9296_CTRL0, 0x13, BIT(i));	/* enable GMSL link# */
		max9296_reset_oneshot(priv);
		usleep_range(2000, 2500);

		ser_read(0x15, &val);
		if (!(val & BIT(1)))
			dev_warn(&priv->client->dev, "link%d valid PCLK is not detected\n", i);

		/* switch to GMSL serial_link for streaming video */
		max9296_write_remote_verify(priv, i, 0x04, conf_link ? 0x43 : 0x83);
		usleep_range(2000, 2500);

		des_write(MAX_GMSL1_4(i), 0x00);		/* disable REV/FWD CC */

		switch (priv->link[i]->ser_id) {
		case MAX9271_ID:
			des_update_bits(MAX_GMSL1_6(i), 0x07, 0x01); /* use D14/15 for HS/VS */
			break;
		case MAX96705_ID:
		case MAX96707_ID:
			des_update_bits(MAX_GMSL1_6(i), 0x07, 0x00); /* use D18/D19 for HS/VS */
			break;
		}
	}

	for (i = 0; i < priv->n_links; i++)
		des_write(MAX_GMSL1_4(i), priv->links_mask & BIT(i) ? 0x03 : 0); /* enable REV/FWD CC */

	des_write(MAX9296_GMSL1_EN, priv->links_mask);		/* enable detected links */
	des_update_bits(MAX9296_CTRL0, 0x13, priv->links_mask == 3 ? 0x13 : priv->links_mask); /* enable detected links */
	max9296_reset_oneshot(priv);				/* one-shot reset links */
}

static void max9296_gmsl1_fsync_setup(struct max9296_priv *priv)
{
	des_write(MAX9296_FSYNC_5, priv->fsync_period & 0xff);	/* Fsync Period L */
	des_write(MAX9296_FSYNC_6, (priv->fsync_period >> 8) & 0xff);/* Fsync Period M */
	des_write(MAX9296_FSYNC_7, priv->fsync_period >> 16);	/* Fsync Period H */
	//des_write(MAX9296_FSYNC_8, 0x00);			/* Disable Err Thresh */
	//des_write(MAX9296_FSYNC_9, 0x00);			/* Disable Err Thresh */
	des_write(MAX9296_FSYNC_10, 0x00);			/* Disable Overlap */
	des_write(MAX9296_FSYNC_11, 0x00);

	des_write(MAX9296_FSYNC_0, 0x00);			/* Manual method, Internal GMSL1 generator mode */

	des_write(MAX_GMSL1_8(0), 0x11);			/* Fsync Tx Enable on Link A */
	des_write(MAX_GMSL1_8(1), 0x11);			/* Fsync Tx Enable on Link B */
	des_write(MAX_GMSL1_8(2), 0x11);			/* Fsync Tx Enable on Link C */
	des_write(MAX_GMSL1_8(3), 0x11);			/* Fsync Tx Enable on Link D */

	des_write(MAX9296_FSYNC_15, 0x1f);			/* GMSL1 Type Fsync, Enable all pipes */
}

/* -----------------------------------------------------------------------------
 * GMSL2
 */

static void max9296_gmsl2_cc_enable(struct max9296_priv *priv, int link, int on)
{
	/* nothing */
}

static int max9296_gmsl2_get_link_lock(struct max9296_priv *priv, int link_n)
{
	int val = 0;

	des_read(MAX9296_CTRL3, &val);

	return !!(val & BIT(3)) && (val & BIT(link_n + 4));
}

static void max9296_gmsl2_initial_setup(struct max9296_priv *priv)
{
	des_write(MAX9296_REG6, 0xC0 | 0x10);	/* set GMSL2 mode */
	des_write(MAX9296_REG1, 0x02);		/* 187.5M/6G */
}

static int max9296_gmsl2_reverse_channel_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];
	int ser_addrs[] = {0x40, 0x42, 0x60, 0x62}; /* possible MAX9295 addresses on i2c bus */
	int timeout = priv->timeout;
	int ret = 0;
	int i = 0;

	des_update_bits(MAX9296_CTRL0, 0x13, BIT(link_n));	/* enable GMSL link# */
	max9296_reset_oneshot(priv);

	/* wait the link to be established, indicated when status bit LOCKED goes high */
	for (; timeout > 0; timeout--) {
		if (max9296_gmsl2_get_link_lock(priv, link_n))
			break;
		mdelay(1);
	}

	if (!timeout) {
		ret = -ETIMEDOUT;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(ser_addrs); i++) {
		int val = 0;

		__reg16_read(ser_addrs[i], 0x000d, &val);	/* read serializer ID */
		if (val == MAX9295A_ID || val == MAX9295B_ID) {
			link->ser_id = val;
			__reg16_write(ser_addrs[i], 0x0000, link->ser_addr << 1); /* relocate serizlizer on I2C bus */
			usleep_range(2000, 2500);
			break;
		}
	}

	if (i == ARRAY_SIZE(ser_addrs)) {
		dev_err(&priv->client->dev, "serializer not found\n");
		goto out;
	}

	priv->links_mask |= BIT(link_n);

out:
	dev_info(&priv->client->dev, "link%d %s %sat 0x%x (0x%x) %s\n", link_n, chip_name(link->ser_id),
			       ret == -EADDRINUSE ? "already " : "", link->ser_addr, ser_addrs[i],
			       ret == -ETIMEDOUT ? "not found: timeout GMSL2 link establish" : "");
	return ret;
}

static int max9296_gmsl2_link_serializer_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];
	int i;

	if (strcmp(priv->mbus, "dvp") == 0) {
		ser_write(MAX9295_VIDEO_TX0(0), BIT(6) |	/* line CRC enable */
						(priv->hven ? BIT(5) : 0)); /* HS/VS encoding */
		ser_write(MAX9295_VIDEO_TX1(0), 0x0a);	/* BPP = 10 */
		ser_write(MAX9295_REG7, 0x07);		/* DVP stream, enable HS/VS, rising edge */

		switch (priv->dt) {
		case MIPI_DT_YUV8:
		case MIPI_DT_RAW12:
			/* setup crossbar: strait DVP mapping */
			ser_write(MAX9295_CROSS(0), priv->cb[0]);
			ser_write(MAX9295_CROSS(1), priv->cb[1]);
			ser_write(MAX9295_CROSS(2), priv->cb[2]);
			ser_write(MAX9295_CROSS(3), priv->cb[3]);
			ser_write(MAX9295_CROSS(4), priv->cb[4]);
			ser_write(MAX9295_CROSS(5), priv->cb[5]);
			ser_write(MAX9295_CROSS(6), priv->cb[6]);
			ser_write(MAX9295_CROSS(7), priv->cb[7]);
			ser_write(MAX9295_CROSS(8), priv->cb[8]);
			ser_write(MAX9295_CROSS(9), priv->cb[9]);
			ser_write(MAX9295_CROSS(10), priv->cb[10]);
			ser_write(MAX9295_CROSS(11), priv->cb[11]);
			break;
		}
	} else {
		/* defaults:
		 *  REG2	- video enable Pipex X,Z
		 *  MIPI_RX0	- 1x4 mode (1-port x 4-lanes)
		 *  MIPI_RX1	- 4-lanes
		 *  MIPI_RX2, MIPI_RX3 - merge PHY1,PHY2 to 1x4-mode
		 *  FRONTTOP_9	- start Pipes X,Z from CSI_A,CSI_B
		 */

		ser_write(MAX9295_FRONTTOP_0, 0x71);			/* enable Pipe X from from CSI_A,CSI_B */
		ser_write(MAX9295_FRONTTOP_12, BIT(6) | priv->dt);	/* primary DT for Pipe X */
		ser_write(MAX9295_FRONTTOP_13, BIT(6) | MIPI_DT_EMB);	/* secondary DT for Pipe X */
	}

	for (i = 0; i < 11; i++) {
		if (priv->gpio[i] == 0) {
			/* GPIO set 0 */
			ser_write(MAX9295_GPIO_A(i), 0x80);	/* 1MOm, GPIO output low */
			ser_write(MAX9295_GPIO_B(i), 0xa0);	/* push-pull, pull-down */
		}
		if (priv->gpio[i] == 1) {
			/* GPIO set 1 */
			ser_write(MAX9295_GPIO_A(i), 0x90);	/* 1MOm, GPIO output high */
			ser_write(MAX9295_GPIO_B(i), 0x60);	/* push-pull, pull-up */
		}
		if (priv->gpio[i] == 2) {
			/* GPIO FSIN */
			ser_write(MAX9295_GPIO_A(i), 0x84);	/* 1MOm, GMSL2 RX from deserializer */
			ser_write(MAX9295_GPIO_C(i), 0x08);	/* pull-none, GPIO ID=8 assosiated with FSYNC transmission */
		}
		if (priv->gpio[i] == 3) {
			/* GPIO Interrupt */
			ser_write(MAX9295_GPIO_A(i), 0x63);	/* 40kOm, GMSL2 TX to deserializer */
			ser_write(MAX9295_GPIO_B(i), 0x25);	/* push-pull, pull-none, GPIO stream ID=5 */
		}
	}

	/* I2C translator setup */
//	ser_write(MAX9295_I2C2, OV490_I2C_ADDR_NEW << 1); /* sensor I2C translated - must be set by sensor driver */
//	ser_write(MAX9295_I2C3, OV490_I2C_ADDR << 1);	/* sensor I2C native - must be set by sensor driver */
	ser_write(MAX9295_I2C4, BROADCAST << 1);	/* serializer broadcast I2C translated */
	ser_write(MAX9295_I2C5, link->ser_addr << 1);	/* serializer broadcast I2C native */
	usleep_range(2000, 2500);

	return 0;
}

static struct {
	int in_dt;
	int out_dt;
} gmsl2_pipe_maps[] = {
	{0x00,		0x00},		/* FS */
	{0x01,		0x01},		/* FE */
	{MIPI_DT_YUV8,	MIPI_DT_YUV8},	/* payload data */
	{MIPI_DT_RAW8,	MIPI_DT_RAW8},
	{MIPI_DT_RAW12,	MIPI_DT_RAW12},
};

static void max9296_gmsl2_pipe_set_source(struct max9296_priv *priv, int pipe, int phy, int in_pipe)
{
	// TODO
}

static void max9296_gmsl2_link_pipe_setup(struct max9296_priv *priv, int link_n)
{
	struct max9296_link *link = priv->link[link_n];
	int pipe = link_n; /* straight mapping */
	int dt = priv->dt; /* must come from imager */
	int in_vc = 0;
	int i;

	max9296_gmsl2_pipe_set_source(priv, pipe, link_n, 0);			/* route Pipe X only */

	if (strcmp(priv->mbus, "dvp") == 0) {
		des_write(MAX9296_RX0(pipe), 0);				/* stream_id = 0 */
		//des_update_bits(MAX_VIDEO_RX0(pipe), BIT(0), BIT(0));		/* disable Packet detector */
		max9296_pipe_override(priv, pipe, dt, in_vc);			/* override dt, vc */
	}

	des_write(MAX_MIPI_TX11(pipe), 0x00);					/* disable all mappings */
	des_write(MAX_MIPI_TX12(pipe), 0x00);

	for (i = 0; i < ARRAY_SIZE(gmsl2_pipe_maps); i++) {
		max9296_set_pipe_to_mipi_mapping(priv, pipe, i,		/* pipe, map# */
						  gmsl2_pipe_maps[i].in_dt, in_vc, /* src DT, VC */
						  gmsl2_pipe_maps[i].out_dt, link->out_vc, /* dst DT, VC */
						  link->out_mipi);		/* dst MIPI PHY */
	}

	link->pipes_mask |= BIT(pipe);
}

static void max9296_gmsl2_postinit(struct max9296_priv *priv)
{
	des_update_bits(MAX9296_CTRL0, 0x13, priv->links_mask == 3 ? 0x13 : priv->links_mask); /* enable detected links */
	max9296_reset_oneshot(priv);				/* one-shot reset links */
}

static void max9296_gmsl2_link_crossbar_setup(struct max9296_priv *priv, int link, int dt)
{
	des_write(MAX_CROSS(link, 24), (priv->hsync ? 0x40 : 0) + 24);	/* invert HS polarity */
	des_write(MAX_CROSS(link, 25), (priv->vsync ? 0 : 0x40) + 25);	/* invert VS polarity */
	des_write(MAX_CROSS(link, 26), (priv->hsync ? 0x40 : 0) + 26);	/* invert DE polarity */
}

static void max9296_gmsl2_fsync_setup(struct max9296_priv *priv)
{
	des_write(MAX9296_FSYNC_5, priv->fsync_period & 0xff);	/* Fsync Period L */
	des_write(MAX9296_FSYNC_6, (priv->fsync_period >> 8) & 0xff);/* Fsync Period M */
	des_write(MAX9296_FSYNC_7, priv->fsync_period >> 16);	/* Fsync Period H */
	des_write(MAX9296_FSYNC_10, 0x00);			/* Disable Overlap */
	des_write(MAX9296_FSYNC_11, 0x00);

	des_write(MAX9296_FSYNC_0, 0x00);			/* Manual method, Internal GMSL2 generator mode */
	des_write(MAX9296_FSYNC_15, 0x80);			/* GMSL2 Type Fsync, Disable all pipes for manual mode */
	des_write(MAX9296_FSYNC_17, 8 << 3);			/* GPIO ID=8 assosiated with FSYNC transmission */
}

/* -----------------------------------------------------------------------------
 * I2C Multiplexer
 */

static int max9296_i2c_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	/* Do nothing! */
	return 0;
}

static int max9296_i2c_mux_init(struct max9296_priv *priv)
{
	struct i2c_client *client = priv->client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	priv->mux = i2c_mux_alloc(client->adapter, &client->dev,
				  priv->n_links, 0, I2C_MUX_LOCKED,
				  max9296_i2c_mux_select, NULL);
	if (!priv->mux)
		return -ENOMEM;

	priv->mux->priv = priv;

	return 0;
}

#define max9296_cc_enable(priv,i,en)		(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_cc_enable(priv, i, en) : \
										 max9296_gmsl1_cc_enable(priv, i, en))
#define max9296_initial_setup(priv)		(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_initial_setup(priv) : \
										 max9296_gmsl1_initial_setup(priv))
#define max9296_reverse_channel_setup(priv,i)	(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_reverse_channel_setup(priv, i) : \
										 max9296_gmsl1_reverse_channel_setup(priv, i))
#define max9296_link_serializer_setup(priv,i)	(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_link_serializer_setup(priv, i) : \
										 max9296_gmsl1_link_serializer_setup(priv, i))
#define max9296_link_pipe_setup(priv,i)	(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_link_pipe_setup(priv, i) : \
										 max9296_gmsl1_link_pipe_setup(priv, i))
#define max9296_link_crossbar_setup(priv,i,dt)	(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_link_crossbar_setup(priv, i, dt) : \
										 max9296_gmsl1_link_crossbar_setup(priv, i, dt))
#define max9296_postinit(priv)			(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_postinit(priv) : \
										 max9296_gmsl1_postinit(priv))
#define max9296_fsync_setup(priv)		(priv->gmsl_mode == MODE_GMSL2 ? max9296_gmsl2_fsync_setup(priv) : \
										 max9296_gmsl1_fsync_setup(priv))

static int max9296_preinit(struct max9296_priv *priv)
{
	int i;

	des_update_bits(MAX9296_CTRL0, BIT(7), BIT(7));	/* reset chip */
	mdelay(5);

	/* enable internal regulator for 1.2V VDD supply */
	des_update_bits(MAX9296_CTRL0, BIT(2), BIT(2));	/* REG_ENABLE = 1 */
	des_update_bits(MAX9296_CTRL2, BIT(4), BIT(4));	/* REG_MNL = 1 */

	/* this is needed for engineering samples */
	for (i = 0; i < priv->n_links; i++) {
		des_write(MAX_RLMS4(i), 0x29);
		des_write(MAX_RLMSA4(i), 0xc8);
		des_write(MAX_RLMSA(i), 0x00);
		des_write(MAX_RLMSB(i), 0x00);
	}

	/* I2C-I2C timings */
	des_write(MAX9296_I2C_PT_0, 0x01);		/* Fast mode Plus, 1mS timeout */
	des_write(MAX9296_I2C_PT_1, 0x51);		/* i2c speed: 397Kbps, 32mS timeout */
	des_write(MAX9296_I2C_0, 0x01);			/* Fast mode Plus, 1mS timeout */
	des_write(MAX9296_I2C_1, 0x51);			/* i2c speed: 397Kbps, 1mS timeout */

	des_write(MAX9296_CTRL1, priv->is_coax ? 0x5 : 0); /* cable mode */
	des_write(MAX9296_GMSL1_EN, 0);			/* disable all links */
	des_update_bits(MAX9296_CTRL0, 0x13, 0);	/* disable all links */

	return 0;
}

static int max9296_initialize(struct max9296_priv *priv)
{
	int ret, i;

	max9296_preinit(priv);
	max9296_initial_setup(priv);
	max9296_mipi_setup(priv);

	for (i = 0; i < priv->n_links; i++) {
		if (!IS_ERR(priv->link[i]->poc_reg)) {
			ret = regulator_enable(priv->link[i]->poc_reg); /* POC power on */
			if (ret) {
				dev_err(&priv->link[i]->client->dev, "failed to enable poc regulator\n");
				continue;
			}
			mdelay(priv->poc_delay);
		}

		ret = max9296_reverse_channel_setup(priv, i);
		if (ret == -ETIMEDOUT)
			continue;
		if (!ret)
			max9296_link_serializer_setup(priv, i);

		max9296_link_pipe_setup(priv, i);
		max9296_link_crossbar_setup(priv, i, priv->dt);

		i2c_mux_add_adapter(priv->mux, 0, i, 0);
		max9296_cc_enable(priv, i, 0);
	}

	max9296_postinit(priv);
	max9296_fsync_setup(priv);

	return 0;
}

static int max9296_reboot_notifier(struct notifier_block *nb, unsigned long code, void *data)
{
	struct max9296_priv *priv = container_of(nb, struct max9296_priv, reboot_nb);
	int i;

	for (i = 0; i < priv->n_links; i++) {
		if (!IS_ERR(priv->link[i]->poc_reg))
			regulator_disable(priv->link[i]->poc_reg); /* POC power off */
	}

	return NOTIFY_DONE;
}

static int max9296_s_power(struct v4l2_subdev *sd, int on)
{
	struct max9296_priv *priv = v4l2_get_subdevdata(sd);

	if (on) {
		if (atomic_inc_return(&priv->use_count) == 1)
			des_update_bits(MAX_BACKTOP12(0), 0x02, 0x02); /* CSI output enable */
	} else {
		if (atomic_dec_return(&priv->use_count) == 0)
			des_update_bits(MAX_BACKTOP12(0), 0x02, 0); /* CSI output disable */
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int max9296_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct max9296_priv *priv = v4l2_get_subdevdata(sd);
	int ret;
	int val = 0;

	ret = des_read(reg->reg, &val);
	if (ret < 0)
		return ret;

	reg->val = val;
	reg->size = sizeof(u16);

	return 0;
}

static int max9296_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct max9296_priv *priv = v4l2_get_subdevdata(sd);

	return des_write(reg->reg, (u8)reg->val);
}
#endif

static struct v4l2_subdev_core_ops max9296_subdev_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register		= max9296_g_register,
	.s_register		= max9296_s_register,
#endif
	.s_power		= max9296_s_power,
};

static struct v4l2_subdev_ops max9296_subdev_ops = {
	.core	= &max9296_subdev_core_ops,
};

static const struct of_device_id max9296_dt_ids[] = {
	{ .compatible = "maxim,max9296" },
	{},
};
MODULE_DEVICE_TABLE(of, max9296_dt_ids);

static int max9296_parse_dt(struct i2c_client *client)
{
	struct max9296_priv *priv = i2c_get_clientdata(client);
	struct device_node *np = client->dev.of_node;
	struct device_node *endpoint = NULL,  *rendpoint = NULL;
	struct property *prop;
	char name[16];
	int i, csi_rate;

	if (of_property_read_u32(np, "maxim,links", &priv->n_links))
		priv->n_links = MAX9296_MAX_LINKS;
	if (of_property_read_u32(np, "maxim,gmsl", &priv->gmsl_mode))
		priv->gmsl_mode = MODE_GMSL2;
	if (of_property_read_bool(np, "maxim,stp"))
		priv->is_coax = 0;
	else
		priv->is_coax = 1;
	if (of_property_read_u32(np, "maxim,resetb-gpio", &priv->gpio_resetb)) {
		priv->gpio_resetb = -1;
	} else {
		if (of_property_read_bool(np, "maxim,resetb-active-high"))
			priv->active_low_resetb = 0;
		else
			priv->active_low_resetb = 1;
	}
	if (of_property_read_u32(np, "maxim,fsync-period", &priv->fsync_period))
		priv->fsync_period = 3210000;/* 96MHz/30fps */
	priv->pclk_rising_edge = true;
	if (of_property_read_bool(np, "maxim,pclk-falling-edge"))
		priv->pclk_rising_edge = false;
	if (of_property_read_u32(np, "maxim,timeout", &priv->timeout))
		priv->timeout = 100;
	if (of_property_read_u32(np, "maxim,him", &priv->him))
		priv->him = 0;
	if (of_property_read_u32(np, "maxim,bws", &priv->bws))
		priv->bws = 0;
	if (of_property_read_u32(np, "maxim,dbl", &priv->dbl))
		priv->dbl = 1;
	if (of_property_read_u32(np, "maxim,hven", &priv->hven))
		priv->hven = 1;
	if (of_property_read_u32(np, "maxim,hibw", &priv->hibw))
		priv->hibw = 0;
	if (of_property_read_u32(np, "maxim,hsync", &priv->hsync))
		priv->hsync = 0;
	if (of_property_read_u32(np, "maxim,vsync", &priv->vsync))
		priv->vsync = 1;
	if (of_property_read_u32(np, "maxim,poc-delay", &priv->poc_delay))
		priv->poc_delay = 50;
	if (of_property_read_u32(np, "maxim,dt", &priv->dt))
		priv->dt = MIPI_DT_YUV8;
	if (of_property_read_u64(np, "maxim,crossbar", &priv->crossbar))
		priv->crossbar = crossbar;
	if (of_property_read_string(np, "maxim,mbus", &priv->mbus))
		priv->mbus = mbus_default;
	for (i = 0; i < 11; i++) {
		sprintf(name, "maxim,gpio%d", i);
		if (of_property_read_u32(np, name, &priv->gpio[i]))
			priv->gpio[i] = -1;
	}

	/* module params override dts */
	if (gmsl != MODE_GMSL2)
		priv->gmsl_mode = gmsl;
	if (him)
		priv->him = him;
	if (fsync_period) {
		priv->fsync_period = fsync_period;
//		priv->fsync_mode = fsync_mode_default;
	}
	if (hsync)
		priv->hsync = hsync;
	if (!vsync)
		priv->vsync = vsync;
	if (gpio_resetb)
		priv->gpio_resetb = gpio_resetb;
	if (active_low_resetb)
		priv->active_low_resetb = active_low_resetb;
	if (timeout_n)
		priv->timeout = timeout_n;
	if (poc_delay)
		priv->poc_delay = poc_delay;
	if (bws)
		priv->bws = bws;
	if (!dbl)
		priv->dbl = dbl;
	if (dt != MIPI_DT_YUV8)
		priv->dt = dt;
//	if (hsgen)
//		priv->hsgen = hsgen;
	if (gpio0 >= 0)
		priv->gpio[0] = gpio0;
	if (gpio1 >= 0)
		priv->gpio[1] = gpio1;
	if (gpio7 >= 0)
		priv->gpio[7] = gpio7;
	if (gpio8 >= 0)
		priv->gpio[8] = gpio8;
	if (strcmp(mbus, "dvp"))
		priv->mbus = mbus;

	/* parse serializer crossbar setup */
	for (i = 0; i < 16; i++) {
		priv->cb[i] = priv->crossbar % 16;
		priv->crossbar /= 16;
	}

	for (i = 0; ; i++) {
		endpoint = of_graph_get_next_endpoint(np, endpoint);
		if (!endpoint)
			break;

		if (i < priv->n_links) {
			if (of_property_read_u32(endpoint, "ser-addr", &priv->link[i]->ser_addr)) {
				of_node_put(endpoint);
				dev_err(&client->dev, "ser-addr not set\n");
				return -EINVAL;
			}
			priv->link[i]->sd_fwnode = of_fwnode_handle(endpoint);
		}

		rendpoint = of_parse_phandle(endpoint, "remote-endpoint", 0);
		if (!rendpoint)
			continue;

		prop = of_find_property(endpoint, "csi-rate", NULL);
		if (prop) {
			of_property_read_u32(endpoint, "csi-rate", &csi_rate);
			of_update_property(rendpoint, prop);
		}

		prop = of_find_property(endpoint, "dvp-order", NULL);
		if (prop)
			of_update_property(rendpoint, prop);
	}

	of_node_put(endpoint);

	for (i = 0; i < priv->n_links; i++) {
		priv->link[i]->out_mipi = 1;	/* CSI default forwarding is to MIPI1 */
		priv->link[i]->out_vc = i;	/* Default VC map: 0 1 2 3 */
	}

	prop = of_find_property(np, "maxim,links-mipi-map", NULL);
	if (prop) {
		const __be32 *map = NULL;
		u32 val;

		for (i = 0; i < priv->n_links; i++) {
			map = of_prop_next_u32(prop, map, &val);
			if (!map)
				break;
			if (val >= MAX9296_MAX_MIPI)
				return -EINVAL;
			priv->link[i]->out_mipi = val;
		}
	}

	for (i = 0; i < priv->n_links; i++)
		priv->csi_rate[priv->link[i]->out_mipi] = csi_rate;

	prop = of_find_property(np, "maxim,links-vc-map", NULL);
	if (prop) {
		const __be32 *map = NULL;
		u32 val;

		for (i = 0; i < priv->n_links; i++) {
			map = of_prop_next_u32(prop, map, &val);
			if (!map)
				break;
			if (val >= 4)
				return -EINVAL;
			priv->link[i]->out_vc = val;
		}
	}

	dev_dbg(&client->dev, "Link# | MIPI rate | Map | VC\n");
	for (i = 0; i < priv->n_links; i++)
		dev_dbg(&client->dev, "%5d | %9d | %3d | %2d\n", i, priv->csi_rate[i], priv->link[i]->out_mipi, priv->link[i]->out_vc);

	return 0;
}

static int max9296_probe(struct i2c_client *client,
			  const struct i2c_device_id *did)
{
	struct max9296_priv *priv;
	struct gpio_desc *pwdn_gpio;
	int ret, i;
	int val = 0;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->regmap = devm_regmap_init_i2c(client, &max9296_regmap[0]);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	i2c_set_clientdata(client, priv);
	priv->client = client;
	atomic_set(&priv->use_count, 0);

	priv->ref_clk = v4l2_clk_get(&client->dev, "ref_clk");
	if (!IS_ERR(priv->ref_clk)) {
		dev_info(&client->dev, "ref_clk = %luKHz", v4l2_clk_get_rate(priv->ref_clk) / 1000);
		v4l2_clk_enable(priv->ref_clk);
	}

	pwdn_gpio = devm_gpiod_get_optional(&client->dev, "shutdown", GPIOD_OUT_HIGH);
	if (!IS_ERR(pwdn_gpio)) {
		udelay(5);
		gpiod_set_value_cansleep(pwdn_gpio, 0);
		usleep_range(3000, 5000);
	}

	des_read(MAX9296_REG13, &val);
	if (val != MAX9296A_ID)
		return -ENODEV;

	for (i = 0; i < MAX9296_MAX_LINKS; i++) {
		priv->link[i] = devm_kzalloc(&client->dev, sizeof(*priv->link[i]), GFP_KERNEL);
		if (!priv->link[i])
			return -ENOMEM;
	}

	ret = max9296_parse_dt(client);
	if (ret)
		goto out;

	for (i = 0; i < priv->n_links; i++) {
		char poc_name[10];

		sprintf(poc_name, "poc%d", i);
		priv->link[i]->poc_reg = devm_regulator_get(&client->dev, poc_name);
		if (PTR_ERR(priv->link[i]->poc_reg) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	for (i = 0; i < priv->n_links; i++) {
		priv->link[i]->client = i2c_new_dummy(client->adapter, priv->link[i]->ser_addr);
		if (!priv->link[i]->client)
			return -ENOMEM;

		priv->link[i]->regmap = regmap_init_i2c(priv->link[i]->client, &max9296_regmap[priv->gmsl_mode]);
		if (IS_ERR(priv->link[i]->regmap))
			return PTR_ERR(priv->link[i]->regmap);
	}

	ret = max9296_i2c_mux_init(priv);
	if (ret) {
		dev_err(&client->dev, "Unable to initialize I2C multiplexer\n");
		goto out;
	}

	ret = max9296_initialize(priv);
	if (ret < 0)
		goto out;

	for (i = 0; i < priv->n_links; i++) {
		v4l2_subdev_init(&priv->link[i]->sd, &max9296_subdev_ops);
		priv->link[i]->sd.owner = client->dev.driver->owner;
		priv->link[i]->sd.dev = &client->dev;
		priv->link[i]->sd.grp_id = i;
		v4l2_set_subdevdata(&priv->link[i]->sd, priv);
		priv->link[i]->sd.fwnode = priv->link[i]->sd_fwnode;

		snprintf(priv->link[i]->sd.name, V4L2_SUBDEV_NAME_SIZE, "%s.%d %d-%04x",
			 client->dev.driver->name, i, i2c_adapter_id(client->adapter),
			 client->addr);

		ret = v4l2_async_register_subdev(&priv->link[i]->sd);
		if (ret < 0)
			goto out;
	}

	priv->reboot_nb.notifier_call = max9296_reboot_notifier;
	ret = register_reboot_notifier(&priv->reboot_nb);
	if (ret) {
		dev_err(&client->dev, "failed to register reboot notifier\n");
		goto out;
	}

	//max9296_debug_add(priv);
out:
	return ret;
}

static int max9296_remove(struct i2c_client *client)
{
	struct max9296_priv *priv = i2c_get_clientdata(client);
	int i;

	//max9296_debug_remove(priv);
	i2c_mux_del_adapters(priv->mux);
	unregister_reboot_notifier(&priv->reboot_nb);

	for (i = 0; i < priv->n_links; i++) {
		v4l2_async_unregister_subdev(&priv->link[i]->sd);
		v4l2_device_unregister_subdev(&priv->link[i]->sd);
		if (!IS_ERR(priv->link[i]->poc_reg))
			regulator_disable(priv->link[i]->poc_reg); /* POC power off */
	}

	return 0;
}

static const struct i2c_device_id max9296_id[] = {
	{ "max9296", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max9296_id);

static struct i2c_driver max9296_i2c_driver = {
	.driver	= {
		.name		= "max9296",
		.of_match_table	= of_match_ptr(max9296_dt_ids),
	},
	.probe		= max9296_probe,
	.remove		= max9296_remove,
	.id_table	= max9296_id,
};

module_i2c_driver(max9296_i2c_driver);

MODULE_DESCRIPTION("GMSL2 driver for MAX9296");
MODULE_AUTHOR("Andrey Gusakov, Vladimir Barinov");
MODULE_LICENSE("GPL");
