/*
 * OmniVision ov10640 sensor camera driver
 *
 * Copyright (C) 2015-2020 Cogent Embedded, Inc.
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

#include "../gmsl/common.h"
#include "ov10640.h"

static const int ov10640_i2c_addr[] = {0x30, 0x34, 0x36};

#define OV10640_PID_REGA		0x300a
#define OV10640_PID_REGB		0x300b
#define OV10640_REV_REG			0x300d
#define OV10640_PID			0xa640

#define OV10640_MEDIA_BUS_FMT		MEDIA_BUS_FMT_SBGGR12_1X12

struct ov10640_priv {
	struct v4l2_subdev		sd;
	struct v4l2_ctrl_handler	hdl;
	struct media_pad		pad;
	struct v4l2_rect		rect;
	struct mutex			lock;
	int				fps_numerator;
	int				fps_denominator;
	int				init_complete;
	u8				id[6];
	int				dvp_order;
	bool				emb_enable;
	int				vts;
	/* serializers */
	int				ser_addr;
};

static int dvp_order = 0;
module_param(dvp_order, int, 0644);
MODULE_PARM_DESC(dvp_order, " DVP bus bits order");

static inline struct ov10640_priv *to_ov10640(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov10640_priv, sd);
}

static inline struct v4l2_subdev *ov10640_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ov10640_priv, hdl)->sd;
}

static int ov10640_set_regs(struct i2c_client *client,
			    const struct ov10640_reg *regs, int nr_regs)
{
	int i;

	for (i = 0; i < nr_regs; i++) {
		if (regs[i].reg == OV10640_DELAY) {
			mdelay(regs[i].val);
			continue;
		}

		if (reg16_write(client, regs[i].reg, regs[i].val)) {
			usleep_range(100, 150); /* wait 100ns */
			if (reg16_write(client, regs[i].reg, regs[i].val))
				printk("ov10640 reg 0x%04x write failed\n", regs[i].reg);
		}
	}

	return 0;
}

static void ov10640_otp_id_read(struct i2c_client *client)
{
	struct ov10640_priv *priv = to_ov10640(client);
	int i;
	int otp_bank0_allzero = 1;

	reg16_write(client, 0x349C, 1);
	usleep_range(25000, 25500); /* wait 25 ms */

	for (i = 0; i < 6; i++) {
		/* first 6 bytes are equal on all ov10640 */
		reg16_read(client, 0x349e + i + 6, &priv->id[i]);
		if (priv->id[i])
			otp_bank0_allzero = 0;
	}

	if (otp_bank0_allzero) {
		reg16_write(client, 0x3495, 0x41); /* bank#1 */
		reg16_write(client, 0x349C, 1);
		usleep_range(25000, 25500); /* wait 25 ms */

		for (i = 0; i < 6; i++)
			reg16_read(client, 0x34ae + i, &priv->id[i]);
	}
}

static int ov10640_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ov10640_set_window(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	dev_dbg(&client->dev, "L=%d T=%d %dx%d\n", priv->rect.left, priv->rect.top, priv->rect.width, priv->rect.height);

	/* horiz crop start (reverse) */
	reg16_write(client, 0x3074, (OV10640_X_END - priv->rect.left - priv->rect.width + 1) >> 8);
	reg16_write(client, 0x3075, (OV10640_X_END - priv->rect.left - priv->rect.width + 1) & 0xff);
	/* horiz crop end (reverse) */
	reg16_write(client, 0x3078, (OV10640_X_END - priv->rect.left) >> 8);
	reg16_write(client, 0x3079, (OV10640_X_END - priv->rect.left) & 0xff);
	/* vert crop start */
	reg16_write(client, 0x3076, (priv->rect.top + OV10640_Y_START - OV10640_EMB_PADDED / 2) >> 8);
	reg16_write(client, 0x3077, (priv->rect.top + OV10640_Y_START - OV10640_EMB_PADDED / 2) & 0xff);
	/* vert crop end */
	reg16_write(client, 0x307a, (priv->rect.top + priv->rect.height + OV10640_Y_START - 1 + OV10640_EMB_PADDED / 2) >> 8);
	reg16_write(client, 0x307b, (priv->rect.top + priv->rect.height + OV10640_Y_START - 1 + OV10640_EMB_PADDED / 2) & 0xff);
	/* horiz output */
	reg16_write(client, 0x307c, priv->rect.width >> 8);
	reg16_write(client, 0x307d, priv->rect.width & 0xff);
	/* vert output */
	reg16_write(client, 0x307e, (priv->rect.height + OV10640_EMB_PADDED) >> 8);
	reg16_write(client, 0x307f, (priv->rect.height + OV10640_EMB_PADDED) & 0xff);

	return 0;
};

static int ov10640_get_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	if (format->pad)
		return -EINVAL;

	mf->width = priv->rect.width;
	mf->height = priv->rect.height;
	mf->code = OV10640_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

static int ov10640_set_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	mf->code = OV10640_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *mf;

	return 0;
}

static int ov10640_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index > 0)
		return -EINVAL;

	code->code = OV10640_MEDIA_BUS_FMT;

	return 0;
}

static int ov10640_get_edid(struct v4l2_subdev *sd, struct v4l2_edid *edid)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	memcpy(edid->edid, priv->id, 6);

	edid->edid[6] = 0xff;
	edid->edid[7] = client->addr;
	edid->edid[8] = OV10640_PID >> 8;
	edid->edid[9] = OV10640_PID & 0xff;

	return 0;
}

static int ov10640_set_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect = &sel->r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE ||
	    sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	rect->left = ALIGN(rect->left, 2);
	rect->top = ALIGN(rect->top, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if ((rect->left + rect->width > OV10640_MAX_WIDTH) ||
	    (rect->top + rect->height > OV10640_MAX_HEIGHT))
		*rect = priv->rect;

	priv->rect.left = rect->left;
	priv->rect.top = rect->top;
	priv->rect.width = rect->width;
	priv->rect.height = rect->height;

	ov10640_set_window(sd);

	return 0;
}

static int ov10640_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OV10640_MAX_WIDTH;
		sel->r.height = OV10640_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = OV10640_DEFAULT_WIDTH;
		sel->r.height = OV10640_DEFAULT_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP:
		sel->r = priv->rect;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r.left = 0;
		sel->r.top = OV10640_EMB_PADDED / 2;
		sel->r.width = priv->rect.width;
		sel->r.height = priv->rect.height;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ov10640_g_mbus_config(struct v4l2_subdev *sd,
				 struct v4l2_mbus_config *cfg)
{
	cfg->flags = V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0 |
		     V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	cfg->type = V4L2_MBUS_CSI2;

	return 0;
}

static int ov10640_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = priv->fps_numerator;
	cp->timeperframe.denominator = priv->fps_denominator;

	return 0;
}

static int ov10640_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;
	int ret = 0;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;

	if (priv->fps_denominator != cp->timeperframe.denominator ||
	    priv->fps_numerator != cp->timeperframe.numerator) {
		priv->vts = (OV10640_SENSOR_HEIGHT + 208) * 30 * cp->timeperframe.numerator / cp->timeperframe.denominator;

		reg16_write(client, 0x3012, 0);
		reg16_write(client, 0x3082, priv->vts >> 8);
		reg16_write(client, 0x3083, priv->vts & 0xff);
		ret = reg16_write(client, 0x3012, 1);

		priv->fps_denominator = cp->timeperframe.numerator;
		priv->fps_denominator = cp->timeperframe.denominator;
	}

	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov10640_g_register(struct v4l2_subdev *sd,
			      struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
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

static int ov10640_s_register(struct v4l2_subdev *sd,
			      const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
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

static struct v4l2_subdev_core_ops ov10640_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov10640_g_register,
	.s_register = ov10640_s_register,
#endif
};

static int ov10640_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ov10640_to_sd(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
	int ret = -EINVAL;
	u8 val = 0;
	static char again[8] = {0, 1, 4, 2, 5, 3, 6, 7};

	if (!priv->init_complete)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
	case V4L2_CID_GAMMA:
		break;
	case V4L2_CID_AUTOGAIN:
		/* automatic gain/exposure */
		reg16_read(client, 0x30FA, &val);
		val &= ~(0x1 << 6);
		val |= (ctrl->val << 6);
		ret = reg16_write(client, 0x30FA, val);
		break;
	case V4L2_CID_GAIN:
		reg16_write(client, 0x30EC, ctrl->val >> 8);	// L
		reg16_write(client, 0x30ED, ctrl->val & 0xff);	// L
		reg16_write(client, 0x30EE, ctrl->val >> 8);	// S
		reg16_write(client, 0x30EF, ctrl->val & 0xff);	// S
		reg16_write(client, 0x30F0, ctrl->val >> 8);	// VS
		ret = reg16_write(client, 0x30F1, ctrl->val & 0xff); // VS
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		val = again[ctrl->val] & 0x4 ? 0xC0 : 0;	// High conversion gain is x2.57, low conversion gain is x1
		val |= ((again[ctrl->val] & 0x3) << 0);		// L
		val |= ((again[ctrl->val] & 0x3) << 2);		// S
		val |= ((again[ctrl->val] & 0x3) << 4);		// VS
		ret = reg16_write(client, 0x30EB, val);
		break;
	case V4L2_CID_EXPOSURE:
		reg16_write(client, 0x30E6, ctrl->val >> 8);	// L
		reg16_write(client, 0x30E7, ctrl->val & 0xff);	// L
		reg16_write(client, 0x30E8, ctrl->val/16 >> 8);	// S
		reg16_write(client, 0x30E9, ctrl->val/16 & 0xff);// S
		ret = reg16_write(client, 0x30EA, ctrl->val/256 >> 8); // VS
		break;
	case V4L2_CID_HFLIP:
		reg16_read(client, 0x3128, &val);
		val &= ~(0x1 << 0);
		val |= (ctrl->val << 0);
		reg16_write(client, 0x3128, val);

		reg16_read(client, 0x3291, &val);
		val &= ~(0x1 << 1);
		val |= (ctrl->val << 1);
		reg16_write(client, 0x3291, val);

		reg16_read(client, 0x3090, &val);
		val &= ~(0x1 << 2);
		val |= (ctrl->val << 2);
		ret = reg16_write(client, 0x3090, val);
		break;
	case V4L2_CID_VFLIP:
		reg16_read(client, 0x3128, &val);
		val &= ~(0x1 << 1);
		val |= (ctrl->val << 1);
		reg16_write(client, 0x3128, val);

		reg16_read(client, 0x3291, &val);
		val &= ~(0x1 << 2);
		val |= (ctrl->val << 2);
		reg16_write(client, 0x3291, val);

		reg16_read(client, 0x3090, &val);
		val &= ~(0x1 << 3);
		val |= (ctrl->val << 3);
		ret = reg16_write(client, 0x3090, val);
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ret = 0;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ov10640_ctrl_ops = {
	.s_ctrl = ov10640_s_ctrl,
};

static struct v4l2_subdev_video_ops ov10640_video_ops = {
	.s_stream	= ov10640_s_stream,
	.g_mbus_config	= ov10640_g_mbus_config,
	.g_parm		= ov10640_g_parm,
	.s_parm		= ov10640_s_parm,
};

static const struct v4l2_subdev_pad_ops ov10640_subdev_pad_ops = {
	.get_edid	= ov10640_get_edid,
	.enum_mbus_code	= ov10640_enum_mbus_code,
	.get_selection	= ov10640_get_selection,
	.set_selection	= ov10640_set_selection,
	.get_fmt	= ov10640_get_fmt,
	.set_fmt	= ov10640_set_fmt,
};

static struct v4l2_subdev_ops ov10640_subdev_ops = {
	.core	= &ov10640_core_ops,
	.video	= &ov10640_video_ops,
	.pad	= &ov10640_subdev_pad_ops,
};

static ssize_t ov10640_otp_id_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	return snprintf(buf, 32, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
}

static ssize_t ov10640_emb_enable_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t count)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);
	u32 val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;
	priv->emb_enable = !!val;

	/* vert crop start */
	reg16_write(client, 0x3076, (priv->rect.top + OV10640_Y_START - OV10640_EMB_PADDED / 2) >> 8);
	reg16_write(client, 0x3077, (priv->rect.top + OV10640_Y_START - OV10640_EMB_PADDED / 2) & 0xff);
	/* vert crop end */
	reg16_write(client, 0x307a, (priv->rect.top + priv->rect.height - 1 + OV10640_Y_START + OV10640_EMB_PADDED / 2) >> 8);
	reg16_write(client, 0x307b, (priv->rect.top + priv->rect.height - 1 + OV10640_Y_START + OV10640_EMB_PADDED / 2) & 0xff);
	/* vert output */
	reg16_write(client, 0x307e, (priv->rect.height + OV10640_EMB_PADDED) >> 8);
	reg16_write(client, 0x307f, (priv->rect.height + OV10640_EMB_PADDED) & 0xff);

	reg16_write(client, 0x3091, priv->emb_enable ? 0x0C : 0x00);

	return count;
}

static ssize_t ov10640_emb_enable_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov10640_priv *priv = to_ov10640(client);

	return snprintf(buf, 4, "%d\n", priv->emb_enable);
}

static DEVICE_ATTR(otp_id_ov10640, S_IRUGO, ov10640_otp_id_show, NULL);
static DEVICE_ATTR(emb_enable_ov10640, S_IRUGO|S_IWUSR, ov10640_emb_enable_show, ov10640_emb_enable_store);

static int ov10640_initialize(struct i2c_client *client)
{
	struct ov10640_priv *priv = to_ov10640(client);
	u8 val = 0, rev;
	u16 pid = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ov10640_i2c_addr); i++) {
		setup_i2c_translator(client, priv->ser_addr, ov10640_i2c_addr[i]);
		udelay(100);

		/* check product ID */
		reg16_read(client, OV10640_PID_REGA, &val);
		pid = val;
		reg16_read(client, OV10640_PID_REGB, &val);
		pid = (pid << 8) | val;

		if (pid == OV10640_PID)
			break;
	}

	if (pid != OV10640_PID) {
		dev_dbg(&client->dev, "Product ID error %x\n", pid);
		return -ENODEV;
	}

	/* check revision  */
	reg16_read(client, OV10640_REV_REG, &val);
	rev = 0x10 | ((val & 0xf) + 0xa);
	/* Read OTP IDs */
	ov10640_otp_id_read(client);
	/* Program wizard registers */
	switch (rev) {
	case 0x1d:
		ov10640_set_regs(client, ov10640_regs_wizard_r1d, ARRAY_SIZE(ov10640_regs_wizard_r1d));
		break;
	case 0x1e:
		ov10640_set_regs(client, ov10640_regs_wizard_r1e, ARRAY_SIZE(ov10640_regs_wizard_r1e));
		break;
	case 0x1f:
		ov10640_set_regs(client, ov10640_regs_wizard_r1f, ARRAY_SIZE(ov10640_regs_wizard_r1f));
		break;
	default:
		dev_err(&client->dev, "Unsupported chip revision\n");
		return -EINVAL;
	}
	/* Set DVP bit swap */
	reg16_write(client, 0x3124, priv->dvp_order << 4);

	dev_info(&client->dev, "ov10640 PID %x (r%x), res %dx%d, OTP_ID %02x:%02x:%02x:%02x:%02x:%02x\n",
		 pid, rev, OV10640_MAX_WIDTH, OV10640_MAX_HEIGHT, priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
	return 0;
}

static int ov10640_parse_dt(struct device_node *np, struct ov10640_priv *priv)
{
	struct i2c_client *client = v4l2_get_subdevdata(&priv->sd);
	u32 addrs[2], naddrs;

	naddrs = of_property_count_elems_of_size(np, "reg", sizeof(u32));
	if (naddrs != 2) {
		dev_err(&client->dev, "Invalid DT reg property\n");
		return -EINVAL;
	}

	priv->ser_addr = addrs[1];

	/* module params override dts */
	if (dvp_order)
		priv->dvp_order = dvp_order;

	return 0;
}

static int ov10640_probe(struct i2c_client *client,
			 const struct i2c_device_id *did)
{
	struct ov10640_priv *priv;
	struct v4l2_ctrl *ctrl;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	v4l2_i2c_subdev_init(&priv->sd, client, &ov10640_subdev_ops);
	priv->sd.flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = OV10640_DEFAULT_WIDTH;
	priv->rect.height = OV10640_DEFAULT_HEIGHT;
	priv->fps_numerator = 1;
	priv->fps_denominator = 30;
	priv->emb_enable = 1;
	mutex_init(&priv->lock);

	v4l2_ctrl_handler_init(&priv->hdl, 4);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_BRIGHTNESS, 0, 0xff, 1, 0x30);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_CONTRAST, 0, 4, 1, 2);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_SATURATION, 0, 0xff, 1, 0xff);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_HUE, 0, 255, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_GAMMA, 0, 0xffff, 1, 0x233);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_AUTOGAIN, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_GAIN, 0, 0x3fff, 1, 0x100);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, 0, 7, 1, 1);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0x400);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 1);
	v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);
	ctrl = v4l2_ctrl_new_std(&priv->hdl, &ov10640_ctrl_ops,
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

	ret = ov10640_parse_dt(client->dev.of_node, priv);
	if (ret)
		goto cleanup;

	ret = ov10640_initialize(client);
	if (ret < 0)
		goto cleanup;

	ret = v4l2_async_register_subdev(&priv->sd);
	if (ret)
		goto cleanup;

	if (device_create_file(&client->dev, &dev_attr_otp_id_ov10640) != 0||
	    device_create_file(&client->dev, &dev_attr_emb_enable_ov10640) != 0) {
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

static int ov10640_remove(struct i2c_client *client)
{
	struct ov10640_priv *priv = i2c_get_clientdata(client);

	device_remove_file(&client->dev, &dev_attr_otp_id_ov10640);
	device_remove_file(&client->dev, &dev_attr_emb_enable_ov10640);
	v4l2_async_unregister_subdev(&priv->sd);
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);

	return 0;
}

static const struct i2c_device_id ov10640_id[] = {
	{ "ov10640", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov10640_id);

static const struct of_device_id ov10640_of_ids[] = {
	{ .compatible = "ovti,ov10640", },
	{ }
};
MODULE_DEVICE_TABLE(of, ov10640_of_ids);

static struct i2c_driver ov10640_i2c_driver = {
	.driver	= {
		.name		= "ov10640",
		.of_match_table	= ov10640_of_ids,
	},
	.probe		= ov10640_probe,
	.remove		= ov10640_remove,
	.id_table	= ov10640_id,
};

module_i2c_driver(ov10640_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for OV10640");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");