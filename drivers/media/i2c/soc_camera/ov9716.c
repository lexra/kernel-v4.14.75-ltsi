/*
 * OmniVision ov9716 sensor camera driver
 *
 * Copyright (C) 2020 Cogent Embedded, Inc.
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

#include "max9286.h"
#include "ov9716.h"

static const int ov9716_i2c_addr[] = {0x36, 0x10};

#define OV9716_PIDA_REG			0x300a
#define OV9716_PIDB_REG			0x300b
#define OV9716_REV_REG			0x300d
#define OV9716_PID			0x9716

#define OV9716_MEDIA_BUS_FMT		MEDIA_BUS_FMT_SBGGR12_1X12

struct ov9716_priv {
	struct v4l2_subdev		sd;
	struct v4l2_ctrl_handler	hdl;
	struct media_pad		pad;
	struct v4l2_rect		rect;
	struct mutex			lock;
	int				subsampling;
	int				fps_numerator;
	int				fps_denominator;
	int				init_complete;
	u8				id[6];
	bool				emb_enable;
	/* serializers */
	int				max9286_addr;
	int				max9271_addr;
	int				port;
	int				gpio_resetb;
	int				gpio_fsin;
	int				vts;
};

static inline struct ov9716_priv *to_ov9716(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov9716_priv, sd);
}

static inline struct v4l2_subdev *ov9716_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ov9716_priv, hdl)->sd;
}

static void ov9716_s_port(struct i2c_client *client, int fwd_en)
{
	struct ov9716_priv *priv = to_ov9716(client);
	int tmp_addr;
	u8 val = 0;

	if (priv->max9286_addr) {
		tmp_addr = client->addr;
		client->addr = priv->max9286_addr;				/* Deserializer I2C address */
		reg8_read(client, 0x1e, &val);					/* read max928X ID */
		if (val == MAX9286_ID) {
			reg8_write(client, 0x0a, fwd_en ? 0x11 << priv->port : 0); /* Enable/disable reverse/forward control for this port */
			usleep_range(5000, 5500);				/* wait 5ms */
		}
		client->addr = tmp_addr;
	};
}

static int ov9716_set_regs(struct i2c_client *client,
			   const struct ov9716_reg *regs, int nr_regs)
{
	int i;

	for (i = 0; i < nr_regs; i++) {
		if (regs[i].reg == OV9716_DELAY) {
			mdelay(regs[i].val);
			continue;
		}

		if (reg16_write(client, regs[i].reg, regs[i].val)) {
			usleep_range(100, 150); /* wait 100ns */
			reg16_write(client, regs[i].reg, regs[i].val);
		}
	}

	return 0;
}

static int ov9716_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ov9716_set_window(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	dev_dbg(&client->dev, "L=%d T=%d %dx%d\n", priv->rect.left, priv->rect.top, priv->rect.width, priv->rect.height);

	/* start recording group3 */
	reg16_write(client, 0x3467, 0x00);
	reg16_write(client, 0x3464, 0x0c);

	/* horiz crop start (reverse) */
	reg16_write(client, BIT(15) | 0x30a0, (OV9716_X_END - priv->rect.left - priv->rect.width + 1) >> 8);
	reg16_write(client, BIT(15) | 0x30a1, (OV9716_X_END - priv->rect.left - priv->rect.width + 1) & 0xff);
	/* horiz crop end (reverse) */
	reg16_write(client, BIT(15) | 0x30a4, (OV9716_X_END - priv->rect.left) >> 8);
	reg16_write(client, BIT(15) | 0x30a5, (OV9716_X_END - priv->rect.left) & 0xff);
	/* vert crop start */
	reg16_write(client, BIT(15) | 0x30a2, (priv->rect.top + OV9716_Y_START - OV9716_EMB_PADDED) >> 8);
	reg16_write(client, BIT(15) | 0x30a3, (priv->rect.top + OV9716_Y_START - OV9716_EMB_PADDED) & 0xff);
	/* vert crop end */
	reg16_write(client, BIT(15) | 0x30a6, (priv->rect.top + priv->rect.height + OV9716_Y_START + 1 + OV9716_EMB_PADDED) >> 8);
	reg16_write(client, BIT(15) | 0x30a7, (priv->rect.top + priv->rect.height + OV9716_Y_START + 1 + OV9716_EMB_PADDED) & 0xff);
	/* horiz output */
	reg16_write(client, BIT(15) | 0x30ac, (priv->rect.width + OV9716_EXTRA_OFFSET) >> 8);
	reg16_write(client, BIT(15) | 0x30ad, (priv->rect.width + OV9716_EXTRA_OFFSET) & 0xff);
	/* vert output */
	reg16_write(client, BIT(15) | 0x30ae, (priv->rect.height + OV9716_EMB_PADDED) >> 8);
	reg16_write(client, BIT(15) | 0x30af, (priv->rect.height + OV9716_EMB_PADDED) & 0xff);

	/* stop recording and launch group3 */
	reg16_write(client, 0x3464, 0x1c);
	reg16_write(client, 0x3467, 0x01);

	return 0;
};

static int ov9716_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	if (format->pad)
		return -EINVAL;

	mf->width = priv->rect.width;
	mf->height = priv->rect.height;
	mf->code = OV9716_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

static int ov9716_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	mf->code = OV9716_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *mf;

	return 0;
}

static int ov9716_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index > 0)
		return -EINVAL;

	code->code = OV9716_MEDIA_BUS_FMT;

	return 0;
}

static int ov9716_get_edid(struct v4l2_subdev *sd, struct v4l2_edid *edid)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	memcpy(edid->edid, priv->id, 6);

	edid->edid[6] = 0xff;
	edid->edid[7] = client->addr;
	edid->edid[8] = OV9716_PID >> 8;
	edid->edid[9] = OV9716_PID & 0xff;

	return 0;
}

static int ov9716_set_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect = &sel->r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE ||
	    sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	rect->left = ALIGN(rect->left, 2);
	rect->top = ALIGN(rect->top, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if ((rect->left + rect->width > OV9716_MAX_WIDTH) ||
	    (rect->top + rect->height > OV9716_MAX_HEIGHT))
		*rect = priv->rect;

	priv->rect.left = rect->left;
	priv->rect.top = rect->top;
	priv->rect.width = rect->width;
	priv->rect.height = rect->height;

	ov9716_set_window(sd);

	return 0;
}

static int ov9716_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OV9716_MAX_WIDTH;
		sel->r.height = OV9716_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OV9716_DEFAULT_WIDTH;
		sel->r.height = OV9716_DEFAULT_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP:
		sel->r = priv->rect;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r.left = 0;
		sel->r.top = OV9716_EMB_PADDED;
		sel->r.width = priv->rect.width;
		sel->r.height = priv->rect.height;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ov9716_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->flags = V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0 |
		     V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	cfg->type = V4L2_MBUS_CSI2;

	return 0;
}

static int ov9716_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = priv->fps_numerator;
	cp->timeperframe.denominator = priv->fps_denominator;

	return 0;
}

static int ov9716_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;
	int ret = 0;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;

	if (priv->fps_denominator != cp->timeperframe.denominator ||
	    priv->fps_numerator != cp->timeperframe.numerator) {
		priv->vts = (OV9716_SENSOR_HEIGHT + 208) * 30 * cp->timeperframe.numerator / cp->timeperframe.denominator;

//		reg16_write(client, 0x3012, 0);
		reg16_write(client, 0x30b2, priv->vts >> 8);
		reg16_write(client, 0x30b3, priv->vts & 0xff);
//		ret = reg16_write(client, 0x3012, 1);

		priv->fps_denominator = cp->timeperframe.numerator;
		priv->fps_denominator = cp->timeperframe.denominator;
	}

	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9716_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
	int ret;

	if (!reg->size)
		reg->size = sizeof(u8);
	if (reg->size > sizeof(reg->val))
		reg->size = sizeof(reg->val);

	mutex_lock(&priv->lock);

	ret = reg16_read_n(client, (u16)reg->reg, (u8*)&reg->val, reg->size);

	mutex_unlock(&priv->lock);

	return ret;
}

static int ov9716_s_register(struct v4l2_subdev *sd,
			     const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
	u32 size = reg->size;
	int ret;

	if (!size)
		size = sizeof(u8);
	if (size > sizeof(reg->val))
		size = sizeof(reg->val);

	mutex_lock(&priv->lock);

	ret = reg16_write_n(client, (u16)reg->reg, (u8*)&reg->val, size);

	mutex_unlock(&priv->lock);

	return ret;
}
#endif

static struct v4l2_subdev_core_ops ov9716_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov9716_g_register,
	.s_register = ov9716_s_register,
#endif
};

static int ov9716_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ov9716_to_sd(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
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
		break;
	case V4L2_CID_GAIN:
		reg16_write(client, 0x3162, ctrl->val >> 8);	// L
		reg16_write(client, 0x3163, ctrl->val & 0xff);	// L
		reg16_write(client, 0x3164, ctrl->val >> 8);	// S
		reg16_write(client, 0x3165, ctrl->val & 0xff);	// S
		reg16_write(client, 0x3166, ctrl->val >> 8);	// VS
		ret = reg16_write(client, 0x3167, ctrl->val & 0xff); // VS
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		val |= (ctrl->val << 0);			// L
		val |= (ctrl->val << 2);			// S
		val |= (ctrl->val << 4);			// VS
		ret = reg16_write(client, 0x30BB, 0x80 | val);	// cg_vs conversion gain is x2.57
		break;
	case V4L2_CID_EXPOSURE:
		reg16_write(client, 0x30B6, ctrl->val >> 8);	// DCG (L + S)
		reg16_write(client, 0x30B7, ctrl->val & 0xff);	// DCG (L + S)
		reg16_write(client, 0x30B8, ctrl->val >> 8);	// VS
		ret = reg16_write(client, 0x30B9, ctrl->val & 8); // VS
		break;
	case V4L2_CID_HFLIP:
		reg16_read(client, 0x3252, &val);
		val &= ~(0x1 << 0);
		val |= (ctrl->val << 0);
		reg16_write(client, 0x3252, val);

		reg16_read(client, 0x30C0, &val);
		val &= ~(0x1 << 2);
		val |= (ctrl->val << 2);
		ret = reg16_write(client, 0x30C0, val);
		break;
	case V4L2_CID_VFLIP:
		reg16_read(client, 0x30C0, &val);
		val &= ~(0x1 << 3);
		val |= (ctrl->val << 3);
		ret = reg16_write(client, 0x30C0, val);
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ret = 0;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ov9716_ctrl_ops = {
	.s_ctrl = ov9716_s_ctrl,
};

static struct v4l2_subdev_video_ops ov9716_video_ops = {
	.s_stream	= ov9716_s_stream,
	.g_mbus_config	= ov9716_g_mbus_config,
	.g_parm		= ov9716_g_parm,
	.s_parm		= ov9716_s_parm,
};

static const struct v4l2_subdev_pad_ops ov9716_subdev_pad_ops = {
	.get_edid	= ov9716_get_edid,
	.enum_mbus_code	= ov9716_enum_mbus_code,
	.get_selection	= ov9716_get_selection,
	.set_selection	= ov9716_set_selection,
	.get_fmt	= ov9716_get_fmt,
	.set_fmt	= ov9716_set_fmt,
};

static struct v4l2_subdev_ops ov9716_subdev_ops = {
	.core	= &ov9716_core_ops,
	.video	= &ov9716_video_ops,
	.pad	= &ov9716_subdev_pad_ops,
};

static void ov9716_otp_id_read(struct i2c_client *client)
{
	struct ov9716_priv *priv = to_ov9716(client);
	int i;
	int otp_bank0_allzero = 1;

	for (i = 0; i < 6; i++) {
		reg16_read(client, 0x7a50 + i, &priv->id[i]);
		if (priv->id[i])
			otp_bank0_allzero = 0;
	}

	if (otp_bank0_allzero) {
		for (i = 0; i < 6; i++)
			reg16_read(client, 0x7a60 + i, &priv->id[i]);
	}
}

static ssize_t ov9716_otp_id_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	return snprintf(buf, 32, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
}

static ssize_t ov9716_emb_enable_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t count)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);
	u32 val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;
	priv->emb_enable = !!val;

	/* vert crop start */
	reg16_write(client, 0x30a2, (priv->rect.top + OV9716_Y_START - OV9716_EMB_PADDED) >> 8);
	reg16_write(client, 0x30a3, (priv->rect.top + OV9716_Y_START - OV9716_EMB_PADDED) & 0xff);
	/* vert crop end */
	reg16_write(client, 0x30a6, (priv->rect.top + priv->rect.height + 1 + OV9716_Y_START + OV9716_EMB_PADDED) >> 8);
	reg16_write(client, 0x30a7, (priv->rect.top + priv->rect.height + 1 + OV9716_Y_START + OV9716_EMB_PADDED) & 0xff);
	/* vert output */
	reg16_write(client, 0x30ae, (priv->rect.height + OV9716_EMB_PADDED) >> 8);
	reg16_write(client, 0x30af, (priv->rect.height + OV9716_EMB_PADDED) & 0xff);

	reg16_write(client, 0x30c1, priv->emb_enable ? 0x04 : 0x00);

	return count;
}

static ssize_t ov9716_emb_enable_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov9716_priv *priv = to_ov9716(client);

	return snprintf(buf, 4, "%d\n", priv->emb_enable);
}

static DEVICE_ATTR(otp_id_ov9716, S_IRUGO, ov9716_otp_id_show, NULL);
static DEVICE_ATTR(emb_enable_ov9716, S_IRUGO|S_IWUSR, ov9716_emb_enable_show, ov9716_emb_enable_store);

static int ov9716_initialize(struct i2c_client *client)
{
	struct ov9716_priv *priv = to_ov9716(client);
	u16 pid;
	u8 val = 0, rev;
	int ret = 0;
	int tmp_addr, i;

	ov9716_s_port(client, 1);

	for (i = 0; i < ARRAY_SIZE(ov9716_i2c_addr); i++) {
		tmp_addr = client->addr;
		if (priv->max9286_addr) {
			client->addr = priv->max9271_addr;
			reg8_write(client, 0x0a, ov9716_i2c_addr[i] << 1); /* Sensor native I2C address */
			usleep_range(2000, 2500);
		};
		client->addr = tmp_addr;

		/* check product ID */
		reg16_read(client, OV9716_PIDA_REG, &val);
		pid = val;
		reg16_read(client, OV9716_PIDB_REG, &val);
		pid = (pid << 8) | val;

		if (pid == OV9716_PID)
			break;
	}

	if (pid != OV9716_PID) {
		dev_err(&client->dev, "Product ID error %x\n", pid);
		ret = -ENODEV;
		goto out;
	}

	/* check revision  */
	reg16_read(client, OV9716_REV_REG, &val);
	rev = 0x10 | ((val & 0xf0) >> 4);
	/* Read OTP IDs */
	ov9716_otp_id_read(client);
	/* Program setup registers */
	switch (rev) {
	case 0x1e:
		ov9716_set_regs(client, ov9716_regs_r1e, ARRAY_SIZE(ov9716_regs_r1e));
		break;
	default:
		dev_err(&client->dev, "Unsupported chip revision\n");
		return -EINVAL;
	}

	dev_info(&client->dev, "ov9716 PID %x (r%x), res %dx%d, OTP_ID %02x:%02x:%02x:%02x:%02x:%02x\n",
		 pid, rev, OV9716_MAX_WIDTH, OV9716_MAX_HEIGHT, priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
out:
	ov9716_s_port(client, 0);

	return ret;
}

static int ov9716_parse_dt(struct device_node *np, struct ov9716_priv *priv)
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

		if (!of_property_read_u32(rendpoint, "max9271-addr", &priv->max9271_addr) &&
		    !of_property_read_u32(rendpoint->parent->parent, "reg", &priv->max9286_addr) &&
		    !kstrtouint(strrchr(rendpoint->full_name, '@') + 1, 0, &priv->port))
			break;
	}

	of_node_put(endpoint);

	if (!priv->max9286_addr) {
		dev_err(&client->dev, "deserializer does not present for OV9716\n");
		return -EINVAL;
	}

	ov9716_s_port(client, 1);

	/* setup I2C translator address */
	tmp_addr = client->addr;
	if (priv->max9286_addr) {
		client->addr = priv->max9271_addr;			/* Serializer I2C address */
		reg8_write(client, 0x09, tmp_addr << 1);		/* Sensor translated I2C address */
		usleep_range(2000, 2500);				/* wait 2ms */
	};
	client->addr = tmp_addr;

	return 0;
}

static int ov9716_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov9716_priv *priv;
	struct v4l2_ctrl *ctrl;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	v4l2_i2c_subdev_init(&priv->sd, client, &ov9716_subdev_ops);
	priv->sd.flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = OV9716_DEFAULT_WIDTH;
	priv->rect.height = OV9716_DEFAULT_HEIGHT;
	priv->fps_numerator = 1;
	priv->fps_denominator = 30;
	priv->emb_enable = 1;
	mutex_init(&priv->lock);

	v4l2_ctrl_handler_init(&priv->hdl, 4);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_BRIGHTNESS, 0, 0xff, 1, 0x30);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_CONTRAST, 0, 4, 1, 2);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_SATURATION, 0, 0xff, 1, 0xff);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_HUE, 0, 255, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_GAMMA, 0, 0xffff, 1, 0x233);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_GAIN, 0, 0x3fff, 1, 0x100);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, 0, 3, 1, 1);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0x40);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 1);
	v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);
	ctrl = v4l2_ctrl_new_std(&priv->hdl, &ov9716_ctrl_ops,
			  V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, 1, 32, 1, 9);
	if (ctrl)
		ctrl->flags &= ~V4L2_CTRL_FLAG_READ_ONLY;
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

	ret = ov9716_parse_dt(client->dev.of_node, priv);
	if (ret)
		goto cleanup;

	ret = ov9716_initialize(client);
	if (ret < 0)
		goto cleanup;

	ret = v4l2_async_register_subdev(&priv->sd);
	if (ret)
		goto cleanup;

	if (device_create_file(&client->dev, &dev_attr_otp_id_ov9716) != 0||
	    device_create_file(&client->dev, &dev_attr_emb_enable_ov9716) != 0) {
		dev_err(&client->dev, "sysfs otp_id entry creation failed\n");
		goto cleanup;
	}

	priv->init_complete = 1;

	return 0;

cleanup:
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);
	return ret;
}

static int ov9716_remove(struct i2c_client *client)
{
	struct ov9716_priv *priv = i2c_get_clientdata(client);

	device_remove_file(&client->dev, &dev_attr_otp_id_ov9716);
	device_remove_file(&client->dev, &dev_attr_emb_enable_ov9716);
	v4l2_async_unregister_subdev(&priv->sd);
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);

	return 0;
}

#ifdef CONFIG_SOC_CAMERA_OV9716
static const struct i2c_device_id ov9716_id[] = {
	{ "ov9716", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9716_id);

static const struct of_device_id ov9716_of_ids[] = {
	{ .compatible = "ovti,ov9716", },
	{ }
};
MODULE_DEVICE_TABLE(of, ov9716_of_ids);

static struct i2c_driver ov9716_i2c_driver = {
	.driver	= {
		.name		= "ov9716",
		.of_match_table	= ov9716_of_ids,
	},
	.probe		= ov9716_probe,
	.remove		= ov9716_remove,
	.id_table	= ov9716_id,
};

module_i2c_driver(ov9716_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for OV9716");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");
#endif
