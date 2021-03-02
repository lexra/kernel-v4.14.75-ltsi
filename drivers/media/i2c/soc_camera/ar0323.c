/*
 * ON Semiconductor AR0323 sensor camera driver
 *
 * Copyright (C) 2018 Cogent Embedded, Inc.
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

#include "ar0323.h"

static const int ar0323_i2c_addr[] = {0x10, 0x20};

#define AR0323_PID		0x3000
#define AR0323_REV		0x31FE
#define AR0323_VERSION_REG	0x0D56

#define AR0323_MEDIA_BUS_FMT	MEDIA_BUS_FMT_SGRBG12_1X12

struct ar0323_priv {
	struct v4l2_subdev		sd;
	struct v4l2_ctrl_handler	hdl;
	struct media_pad		pad;
	struct v4l2_rect		rect;
	int				init_complete;
	u8				id[6];
	/* serializers */
	int				ti9x4_addr;
	int				ti9x3_addr;
	int				port;
	int				gpio_resetb;
	int				gpio_fsin;
};

static inline struct ar0323_priv *to_ar0323(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ar0323_priv, sd);
}

static int ar0323_set_regs(struct i2c_client *client,
			  const struct ar0323_reg *regs, int nr_regs)
{
	int i;

	for (i = 0; i < nr_regs; i++) {
		if (regs[i].reg == AR0323_DELAY) {
			mdelay(regs[i].val);
			continue;
		}

		reg16_write16(client, regs[i].reg, regs[i].val);
	}

	return 0;
}

static int ar0323_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ar0323_get_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);

	if (format->pad)
		return -EINVAL;

	mf->width = priv->rect.width;
	mf->height = priv->rect.height;
	mf->code = AR0323_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

static int ar0323_set_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	mf->code = AR0323_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *mf;

	return 0;
}

static int ar0323_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index > 0)
		return -EINVAL;

	code->code = AR0323_MEDIA_BUS_FMT;

	return 0;
}

static int ar0323_get_edid(struct v4l2_subdev *sd, struct v4l2_edid *edid)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);

	memcpy(edid->edid, priv->id, 6);

	edid->edid[6] = 0xff;
	edid->edid[7] = client->addr;
	edid->edid[8] = AR0323_VERSION_REG >> 8;
	edid->edid[9] = AR0323_VERSION_REG & 0xff;

	return 0;
}

static int ar0323_set_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect = &sel->r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE ||
	    sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	rect->left = ALIGN(rect->left, 2);
	rect->top = ALIGN(rect->top, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if ((rect->left + rect->width > AR0323_MAX_WIDTH) ||
	    (rect->top + rect->height > AR0323_MAX_HEIGHT))
		*rect = priv->rect;

	priv->rect.left = rect->left;
	priv->rect.top = rect->top;
	priv->rect.width = rect->width;
	priv->rect.height = rect->height;

	return 0;
}

static int ar0323_get_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = AR0323_MAX_WIDTH;
		sel->r.height = AR0323_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = AR0323_MAX_WIDTH;
		sel->r.height = AR0323_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP:
		sel->r = priv->rect;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ar0323_g_mbus_config(struct v4l2_subdev *sd,
			       struct v4l2_mbus_config *cfg)
{
	cfg->flags = V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0 |
		     V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	cfg->type = V4L2_MBUS_CSI2;

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ar0323_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	__be64 be_val;

	if (!reg->size)
		reg->size = sizeof(u16);
	if (reg->size > sizeof(reg->val))
		reg->size = sizeof(reg->val);

	ret = reg16_read_n(client, (u16)reg->reg, (u8*)&be_val, reg->size);
	be_val = be_val << ((sizeof(be_val) - reg->size) * 8);
	reg->val = be64_to_cpu(be_val);

	return ret;
}

static int ar0323_s_register(struct v4l2_subdev *sd,
			     const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u32 size = reg->size;
	int ret;
	__be64 be_val;

	if (!size)
		size = sizeof(u16);
	if (size > sizeof(reg->val))
		size = sizeof(reg->val);

	be_val = cpu_to_be64(reg->val);
	be_val = be_val >> ((sizeof(be_val) - size) * 8);
	ret = reg16_write_n(client, (u16)reg->reg, (u8*)&be_val, size);

	return ret;
}
#endif

static struct v4l2_subdev_core_ops ar0323_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ar0323_g_register,
	.s_register = ar0323_s_register,
#endif
};

static int ar0323_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);
	int ret = -EINVAL;
	u16 val = 0;

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
		/* Digital gain */
		ret = reg16_write16(client, 0x3308, ctrl->val);
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		/* Analog gain */
		ret = reg16_write16(client, 0x3366, (ctrl->val << 8) | (ctrl->val << 4) | ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		/* T1 exposure */
		ret = reg16_write16(client, 0x3012, ctrl->val);
		break;
	case V4L2_CID_HFLIP:
		ret = reg16_read16(client, 0x3040, &val);
		if (ctrl->val)
			val |= (1 << 14);
		else
			val &= ~(1 << 14);
		ret |= reg16_write16(client, 0x3040, val);
		break;
	case V4L2_CID_VFLIP:
		ret = reg16_read16(client, 0x3040, &val);
		if (ctrl->val)
			val |= (1 << 15);
		else
			val &= ~(1 << 15);
		ret |= reg16_write16(client, 0x3040, val);
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ret = 0;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ar0323_ctrl_ops = {
	.s_ctrl = ar0323_s_ctrl,
};

static struct v4l2_subdev_video_ops ar0323_video_ops = {
	.s_stream	= ar0323_s_stream,
	.g_mbus_config	= ar0323_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops ar0323_subdev_pad_ops = {
	.get_edid	= ar0323_get_edid,
	.enum_mbus_code	= ar0323_enum_mbus_code,
	.get_selection	= ar0323_get_selection,
	.set_selection	= ar0323_set_selection,
	.get_fmt	= ar0323_get_fmt,
	.set_fmt	= ar0323_set_fmt,
};

static struct v4l2_subdev_ops ar0323_subdev_ops = {
	.core	= &ar0323_core_ops,
	.video	= &ar0323_video_ops,
	.pad	= &ar0323_subdev_pad_ops,
};

static void ar0323_otp_id_read(struct i2c_client *client)
{
	struct ar0323_priv *priv = to_ar0323(client);
	int i;
	u16 val = 0;

	/* read camera id from ar014x OTP memory */
	reg16_write16(client, 0x3054, 0x400);
	reg16_write16(client, 0x304a, 0x110);
	usleep_range(25000, 25500); /* wait 25 ms */

	for (i = 0; i < 6; i += 2) {
		/* first 4 bytes are equal on all ar014x */
		reg16_read16(client, 0x3800 + i + 4, &val);
		priv->id[i]     = val >> 8;
		priv->id[i + 1] = val & 0xff;
	}
}

static ssize_t ar0323_otp_id_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0323_priv *priv = to_ar0323(client);

	ar0323_otp_id_read(client);

	return snprintf(buf, 32, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
}

static DEVICE_ATTR(otp_id_ar0323, S_IRUGO, ar0323_otp_id_show, NULL);

static int ar0323_initialize(struct i2c_client *client)
{
	struct ar0323_priv *priv = to_ar0323(client);
	u16 val = 0;
	u16 pid = 0, rev = 0;
	int ret = 0;
	int tmp_addr;
	int i;

	for (i = 0; i < ARRAY_SIZE(ar0323_i2c_addr); i++) {
		tmp_addr = client->addr;
		if (priv->ti9x4_addr) {
			client->addr = priv->ti9x4_addr;/* Deserializer I2C address */
			reg8_write(client, 0x5d, ar0323_i2c_addr[i] << 1); /* Sensor native I2C address */
			usleep_range(2000, 2500);	/* wait 2ms */
		}
		client->addr = tmp_addr;

		/* check model ID */
		reg16_read16(client, AR0323_PID, &pid);

		if (pid == AR0323_VERSION_REG)
			break;
	}

	if (pid != AR0323_VERSION_REG) {
		dev_dbg(&client->dev, "Product ID error %x\n", pid);
		ret = -ENODEV;
		goto err;
	}
#if 0
	/* setup XCLK */
	tmp_addr = client->addr;
	if (priv->ti9x4_addr) {
		/* CLK_OUT=22.5792*160*M/N/CLKDIV -> CLK_OUT=27MHz: CLKDIV=2, M=15, N=251: 22.5792*160/8*15/251=26.987MHz=CLK_OUT */
		client->addr = priv->ti9x3_addr;			/* Serializer I2C address */
		reg8_write(client, 0x06, 0x6f);				/* Set CLKDIV and M */
		reg8_write(client, 0x07, 0xfb);				/* Set N */
		reg8_write(client, 0x0e, 0x1f);				/* Set FSIN GPIO to output */
	}
	client->addr = tmp_addr;
#endif
	/* check revision  */
	reg16_read16(client, AR0323_REV, &rev);
	/* Read OTP IDs */
	ar0323_otp_id_read(client);
	/* Program wizard registers */
	ar0323_set_regs(client, ar0323_regs_wizard, ARRAY_SIZE(ar0323_regs_wizard));

	/* Enable stream */
	reg16_read16(client, 0x301a, &val);	// read inital reset_register value
	val |= (1 << 2);			// Set streamOn bit
	reg16_write16(client, 0x301a, val);	// Start Streaming

	dev_info(&client->dev, "ar0323 PID %x (rev %x), res %dx%d, OTP_ID %02x:%02x:%02x:%02x:%02x:%02x\n",
		 pid, rev, AR0323_MAX_WIDTH, AR0323_MAX_HEIGHT, priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
err:
	return ret;
}

static int ar0323_parse_dt(struct device_node *np, struct ar0323_priv *priv)
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
	}
	client->addr = tmp_addr;

	return 0;
}

static int ar0323_probe(struct i2c_client *client,
		       const struct i2c_device_id *did)
{
	struct ar0323_priv *priv;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	v4l2_i2c_subdev_init(&priv->sd, client, &ar0323_subdev_ops);
	priv->sd.flags = V4L2_SUBDEV_FL_HAS_DEVNODE;

	v4l2_ctrl_handler_init(&priv->hdl, 4);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_BRIGHTNESS, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_CONTRAST, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_SATURATION, 0, 7, 1, 2);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_HUE, 0, 23, 1, 12);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_GAMMA, -128, 128, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_SHARPNESS, 0, 10, 1, 3);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_AUTOGAIN, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_GAIN, 1, 0x7ff, 1, 0x200);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, 1, 0xe, 1, 0xa);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_EXPOSURE, 1, 0x600, 1, 0x144);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ar0323_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);
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

	ret = ar0323_parse_dt(client->dev.of_node, priv);
	if (ret)
		goto cleanup;

	ret = ar0323_initialize(client);
	if (ret < 0)
		goto cleanup;

	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = AR0323_MAX_WIDTH;
	priv->rect.height = AR0323_MAX_HEIGHT;

	ret = v4l2_async_register_subdev(&priv->sd);
	if (ret)
		goto cleanup;

	if (device_create_file(&client->dev, &dev_attr_otp_id_ar0323) != 0) {
		dev_err(&client->dev, "sysfs otp_id entry creation failed\n");
		goto cleanup;
	}

	priv->init_complete = 1;

	return 0;

cleanup:
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);
#ifdef CONFIG_SOC_CAMERA_AR0323
	v4l_err(client, "failed to probe @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
#endif
	return ret;
}

static int ar0323_remove(struct i2c_client *client)
{
	struct ar0323_priv *priv = i2c_get_clientdata(client);

	device_remove_file(&client->dev, &dev_attr_otp_id_ar0323);
	v4l2_async_unregister_subdev(&priv->sd);
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);

	return 0;
}

#ifdef CONFIG_SOC_CAMERA_AR0323
static const struct i2c_device_id ar0323_id[] = {
	{ "ar0323", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0323_id);

static const struct of_device_id ar0323_of_ids[] = {
	{ .compatible = "aptina,ar0323", },
	{ }
};
MODULE_DEVICE_TABLE(of, ar0323_of_ids);

static struct i2c_driver ar0323_i2c_driver = {
	.driver	= {
		.name		= "ar0323",
		.of_match_table	= ar0323_of_ids,
	},
	.probe		= ar0323_probe,
	.remove		= ar0323_remove,
	.id_table	= ar0323_id,
};

module_i2c_driver(ar0323_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for AR0323");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");
#endif
