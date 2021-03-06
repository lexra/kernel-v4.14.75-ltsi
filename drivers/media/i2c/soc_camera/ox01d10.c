/*
 * OmniVision OX01D10 sensor camera driver
 *
 * Copyright (C) 2019 Cogent Embedded, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/videodev2.h>

#include <media/soc_camera.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>

#include "ox01d10.h"

#define OX01D10_I2C_ADDR	0x36

#define OX01D10_PID_REGA	0x300A
#define OX01D10_PID_REGB	0x300B
#define OX01D10_PID		0x5801

#define OX01D10_MEDIA_BUS_FMT	MEDIA_BUS_FMT_SBGGR12_1X12

struct ox01d10_priv {
	struct v4l2_subdev		sd;
	struct v4l2_ctrl_handler	hdl;
	struct media_pad		pad;
	struct v4l2_rect		rect;
	int				init_complete;
	u8				id[6];
	int				exposure;
	int				gain;
	int				again;
	int				autogain;
	/* serializers */
	int				ti9x4_addr;
	int				ti9x3_addr;
	int				port;
	int				gpio_resetb;
	int				gpio_fsin;
};

static inline struct ox01d10_priv *to_ox01d10(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ox01d10_priv, sd);
}

static int ox01d10_set_regs(struct i2c_client *client,
			  const struct ox01d10_reg *regs, int nr_regs)
{
	int i;

	for (i = 0; i < nr_regs; i++) {
		if (regs[i].reg == OX01D10_DELAY) {
			mdelay(regs[i].val);
			continue;
		}

		reg16_write(client, regs[i].reg, regs[i].val);
	}

	return 0;
}

static int ox01d10_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ox01d10_get_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);

	if (format->pad)
		return -EINVAL;

	mf->width = priv->rect.width;
	mf->height = priv->rect.height;
	mf->code = OX01D10_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

static int ox01d10_set_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	mf->code = OX01D10_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *mf;

	return 0;
}

static int ox01d10_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index > 0)
		return -EINVAL;

	code->code = OX01D10_MEDIA_BUS_FMT;

	return 0;
}

static int ox01d10_get_edid(struct v4l2_subdev *sd, struct v4l2_edid *edid)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);

	memcpy(edid->edid, priv->id, 6);

	edid->edid[6] = 0xff;
	edid->edid[7] = client->addr;
	edid->edid[8] = OX01D10_PID >> 8;
	edid->edid[9] = OX01D10_PID & 0xff;

	return 0;
}

static int ox01d10_set_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect = &sel->r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE ||
	    sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	rect->left = ALIGN(rect->left, 2);
	rect->top = ALIGN(rect->top, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if ((rect->left + rect->width > OX01D10_MAX_WIDTH) ||
	    (rect->top + rect->height > OX01D10_MAX_HEIGHT))
		*rect = priv->rect;

	priv->rect.left = rect->left;
	priv->rect.top = rect->top;
	priv->rect.width = rect->width;
	priv->rect.height = rect->height;

	return 0;
}

static int ox01d10_get_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OX01D10_MAX_WIDTH;
		sel->r.height = OX01D10_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OX01D10_MAX_WIDTH;
		sel->r.height = OX01D10_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP:
		sel->r = priv->rect;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ox01d10_g_mbus_config(struct v4l2_subdev *sd,
			       struct v4l2_mbus_config *cfg)
{
	cfg->flags = V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0 |
		     V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	cfg->type = V4L2_MBUS_CSI2;

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ox01d10_g_register(struct v4l2_subdev *sd,
			    struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!reg->size)
		reg->size = sizeof(u8);
	if (reg->size > sizeof(reg->val))
		reg->size = sizeof(reg->val);

	return reg16_read_n(client, (u16)reg->reg, (u8*)&reg->val, reg->size);
}

static int ox01d10_s_register(struct v4l2_subdev *sd,
			    const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u32 size = reg->size;

	if (!size)
		size = sizeof(u8);
	if (size > sizeof(reg->val))
		size = sizeof(reg->val);

	return reg16_write_n(client, (u16)reg->reg, (u8*)&reg->val, size);
}
#endif

static struct v4l2_subdev_core_ops ox01d10_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ox01d10_g_register,
	.s_register = ox01d10_s_register,
#endif
};

static int ox01d10_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);
	int ret = -EINVAL;
	u8 val = 0;

	if (!priv->init_complete)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
	case V4L2_CID_GAMMA:
	case V4L2_CID_SHARPNESS:
	case V4L2_CID_AUTOGAIN:
		break;
	case V4L2_CID_GAIN:
		/* start recording group3 */
		ret = reg16_write(client, 0x3208, 0x03);
		/* HCG digital gain */
		ret |= reg16_write(client, 0x350a, ctrl->val >> 8);
		ret |= reg16_write(client, 0x350b, ctrl->val & 0xff);
		/* LCG digital gain */
		ret |= reg16_write(client, 0x354a, ctrl->val/8 >> 8);
		ret |= reg16_write(client, 0x354b, ctrl->val/8 & 0xff);
		/* VS digital gain */
		ret |= reg16_write(client, 0x358a, ctrl->val/64 >> 8);
		ret |= reg16_write(client, 0x358b, ctrl->val/64 & 0xff);
		/* stop recording and launch group3 */
		ret |= reg16_write(client, 0x3208, 0x13);
		ret |= reg16_write(client, 0x3208, 0xe3);
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		/* start recording group3 */
		ret = reg16_write(client, 0x3208, 0x03);
		/* HCG real gain */
		ret |= reg16_write(client, 0x3508, ctrl->val >> 8);
		ret |= reg16_write(client, 0x3509, ctrl->val & 0xff);
		/* LCG real gain */
		ret |= reg16_write(client, 0x3548, ctrl->val/8 >> 8);
		ret |= reg16_write(client, 0x3549, ctrl->val/8 & 0xff);
		/* VS real gain */
		ret |= reg16_write(client, 0x3588, ctrl->val/64 >> 8);
		ret |= reg16_write(client, 0x3589, ctrl->val/64 & 0xff);
		/* stop recording and launch group3 */
		ret |= reg16_write(client, 0x3208, 0x13);
		ret |= reg16_write(client, 0x3208, 0xe3);
		break;
	case V4L2_CID_EXPOSURE:
		/* start recording group3 */
		ret = reg16_write(client, 0x3208, 0x03);
		/* HCG (long) exposure time */
		ret |= reg16_write(client, 0x3501, ctrl->val >> 8);
		ret |= reg16_write(client, 0x3502, ctrl->val & 0xff);
		/* LCG (short) exposure time */
		ret |= reg16_write(client, 0x3541, ctrl->val/4 >> 8);
		ret |= reg16_write(client, 0x3542, ctrl->val/4 & 0xff);
		/* VS exposure time */
		ret |= reg16_write(client, 0x3581, ctrl->val/16 >> 8);
		ret |= reg16_write(client, 0x3582, ctrl->val/16 & 0xff);
		/* stop recording and launch group3 */
		ret |= reg16_write(client, 0x3208, 0x13);
		ret |= reg16_write(client, 0x3208, 0xe3);
		break;
	case V4L2_CID_HFLIP:
		/* start recording group3 */
		ret = reg16_write(client, 0x3208, 0x03);
		ret = reg16_read(client, 0x3821, &val);
		if (ctrl->val)
			val |= 0x04;
		else
			val &= ~0x04;
		ret |= reg16_write(client, 0x3821, val);
		/* hflip channges CFA, hence compensate it by moving crop window over bayer matrix */
		ret |= reg16_read(client, 0x3811, &val);
		if (ctrl->val)
			val++;
		else
			val--;
		ret |= reg16_write(client, 0x3811, val);
		/* stop recording and launch group3 */
		ret |= reg16_write(client, 0x3208, 0x13);
		ret |= reg16_write(client, 0x3208, 0xe3);
		break;
	case V4L2_CID_VFLIP:
		ret = reg16_read(client, 0x3820, &val);
		if (ctrl->val)
			val |= 0x44;
		else
			val &= ~0x44;
		ret |= reg16_write(client, 0x3820, val);
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ox01d10_ctrl_ops = {
	.s_ctrl = ox01d10_s_ctrl,
};

static struct v4l2_subdev_video_ops ox01d10_video_ops = {
	.s_stream	= ox01d10_s_stream,
	.g_mbus_config	= ox01d10_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops ox01d10_subdev_pad_ops = {
	.get_edid	= ox01d10_get_edid,
	.enum_mbus_code	= ox01d10_enum_mbus_code,
	.get_selection	= ox01d10_get_selection,
	.set_selection	= ox01d10_set_selection,
	.get_fmt	= ox01d10_get_fmt,
	.set_fmt	= ox01d10_set_fmt,
};

static struct v4l2_subdev_ops ox01d10_subdev_ops = {
	.core	= &ox01d10_core_ops,
	.video	= &ox01d10_video_ops,
	.pad	= &ox01d10_subdev_pad_ops,
};

static void ox01d10_otp_id_read(struct i2c_client *client)
{
}

static ssize_t ox01d10_otp_id_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ox01d10_priv *priv = to_ox01d10(client);

	return snprintf(buf, 32, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
}

static DEVICE_ATTR(otp_id_ox01d10, S_IRUGO, ox01d10_otp_id_show, NULL);

static int ox01d10_initialize(struct i2c_client *client)
{
	struct ox01d10_priv *priv = to_ox01d10(client);
	u8 val = 0;
	u16 pid;
	int ret = 0;
	int tmp_addr = 0;

	/* check and show model ID */
	reg16_read(client, OX01D10_PID_REGA, &val);
	pid = val;
	reg16_read(client, OX01D10_PID_REGB, &val);
	pid = (pid << 8) | val;

	if (pid != OX01D10_PID) {
		dev_dbg(&client->dev, "Product ID error %x\n", pid);
		ret = -ENODEV;
		goto err;
	}

	tmp_addr = client->addr;
	if (priv->ti9x4_addr) {
		client->addr = priv->ti9x3_addr;
		reg8_write(client, 0x02, 0x13);	/* MIPI 2-lanes */

		/* Setup XCLK: CLK_OUT=23MHz*160*M/N/CLKDIV, CLK_OUT=24MHz (desired), CLKDIV=4, M=6, N=230, 23*160/4*6/230 = 24MHz = CLK_OUT */
		reg8_write(client, 0x06, 0x46);	/* Set CLKDIV and M */
		reg8_write(client, 0x07, 0xe6);	/* Set N */
	}
	client->addr = tmp_addr;

	/* Program wizard registers */
	ox01d10_set_regs(client, ox01d10_regs_wizard_r1b_hdr3, ARRAY_SIZE(ox01d10_regs_wizard_r1b_hdr3));
	/* Read OTP IDs */
	ox01d10_otp_id_read(client);

	dev_info(&client->dev, "ox01d10 PID %x, res %dx%d, OTP_ID %02x:%02x:%02x:%02x:%02x:%02x\n",
		 pid, OX01D10_MAX_WIDTH, OX01D10_MAX_HEIGHT, priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
err:
	return ret;
}

static int ox01d10_parse_dt(struct device_node *np, struct ox01d10_priv *priv)
{
	struct i2c_client *client = v4l2_get_subdevdata(&priv->sd);
	int i;
	struct device_node *endpoint = NULL, *rendpoint = NULL;
	int tmp_addr = 0;

	for (i = 0; ; i++) {
		endpoint = of_graph_get_next_endpoint(np, endpoint);
		if (!endpoint)
			break;

		rendpoint = of_parse_phandle(endpoint, "remote-endpoint", 0);
		if (!rendpoint)
			continue;

		if (!of_property_read_u32(rendpoint, "ti9x3-addr", &priv->ti9x3_addr) &&
		    !of_property_match_string(rendpoint->parent->parent, "compatible", "ti,ti9x4") &&
		    !of_property_read_u32(rendpoint->parent->parent, "reg", &priv->ti9x4_addr) &&
		    !kstrtouint(strrchr(rendpoint->full_name, '@') + 1, 0, &priv->port))
			break;
	}

	of_node_put(endpoint);

	if (!priv->ti9x4_addr) {
		dev_err(&client->dev, "deserializer does not present\n");
		return -EINVAL;
	}

	/* setup I2C translator address */
	tmp_addr = client->addr;
	if (priv->ti9x4_addr) {
		client->addr = priv->ti9x4_addr;			/* Deserializer I2C address */
		reg8_write(client, 0x4c, (priv->port << 4) | (1 << priv->port)); /* Select RX port number */
		usleep_range(2000, 2500);				/* wait 2ms */
		reg8_write(client, 0x65, tmp_addr << 1);		/* Sensor translated I2C address */
		reg8_write(client, 0x5d, OX01D10_I2C_ADDR << 1);		/* Sensor native I2C address */
	}
	client->addr = tmp_addr;

	mdelay(10);

	return 0;
}

static int ox01d10_probe(struct i2c_client *client,
		       const struct i2c_device_id *did)
{
	struct ox01d10_priv *priv;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	v4l2_i2c_subdev_init(&priv->sd, client, &ox01d10_subdev_ops);
	priv->sd.flags = V4L2_SUBDEV_FL_HAS_DEVNODE;

	priv->exposure = 0x200;
	priv->gain = 0x200;
	priv->again = 0x200;
	priv->autogain = 1;
	v4l2_ctrl_handler_init(&priv->hdl, 4);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_BRIGHTNESS, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_CONTRAST, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_SATURATION, 0, 7, 1, 2);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_HUE, 0, 23, 1, 12);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_GAMMA, -128, 128, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_SHARPNESS, 0, 10, 1, 3);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_AUTOGAIN, 0, 1, 1, priv->autogain);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_GAIN, 1, 0xfff, 1, priv->gain);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, 1, 0xfff, 1, priv->again);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_EXPOSURE, 1, 0xffff, 1, priv->exposure);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ox01d10_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 1);
	priv->sd.ctrl_handler = &priv->hdl;

	ret = priv->hdl.error;
	if (ret)
		goto cleanup;

	v4l2_ctrl_handler_setup(&priv->hdl);

	priv->pad.flags = MEDIA_PAD_FL_SOURCE;
	priv->sd.entity.flags |= MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&priv->sd.entity, 1, &priv->pad);
	if (ret < 0)
		goto cleanup;

	ret = ox01d10_parse_dt(client->dev.of_node, priv);
	if (ret)
		goto cleanup;

	ret = ox01d10_initialize(client);
	if (ret < 0)
		goto cleanup;

	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = OX01D10_MAX_WIDTH;
	priv->rect.height = OX01D10_MAX_HEIGHT;

	ret = v4l2_async_register_subdev(&priv->sd);
	if (ret)
		goto cleanup;

	if (device_create_file(&client->dev, &dev_attr_otp_id_ox01d10) != 0) {
		dev_err(&client->dev, "sysfs otp_id entry creation failed\n");
		goto cleanup;
	}

	priv->init_complete = 1;

	return 0;

cleanup:
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);
#ifdef CONFIG_SOC_CAMERA_OX01D10
	v4l_err(client, "failed to probe @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
#endif
	return ret;
}

static int ox01d10_remove(struct i2c_client *client)
{
	struct ox01d10_priv *priv = i2c_get_clientdata(client);

	device_remove_file(&client->dev, &dev_attr_otp_id_ox01d10);
	v4l2_async_unregister_subdev(&priv->sd);
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);

	return 0;
}

#ifdef CONFIG_SOC_CAMERA_OX01D10
static const struct i2c_device_id ox01d10_id[] = {
	{ "ox01d10", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ox01d10_id);

static const struct of_device_id ox01d10_of_ids[] = {
	{ .compatible = "ovti,ox01d10", },
	{ }
};
MODULE_DEVICE_TABLE(of, ox01d10_of_ids);

static struct i2c_driver ox01d10_i2c_driver = {
	.driver	= {
		.name		= "ox01d10",
		.of_match_table	= ox01d10_of_ids,
	},
	.probe		= ox01d10_probe,
	.remove		= ox01d10_remove,
	.id_table	= ox01d10_id,
};

module_i2c_driver(ox01d10_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for OX01D10");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");
#endif
