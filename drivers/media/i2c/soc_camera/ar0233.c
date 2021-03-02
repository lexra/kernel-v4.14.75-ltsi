/*
 * ON Semiconductor AR0220/233 sensor camera driver
 *
 * Copyright (C) 2018-2019 Cogent Embedded, Inc.
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

#define AR_DELAY		0xffff
static int AR_MAX_WIDTH, AR_DEFAULT_WIDTH;
static int AR_MAX_HEIGHT, AR_DEFAULT_HEIGHT;
#define AR_EMB_LINES		8
#define AR_EMB_PADDED		(priv->emb_enable ? AR_EMB_LINES : 0) /* embedded data (SOF) and stats (EOF) */
static int AR_X_START;
static int AR_Y_START;
static int AR_X_END;
static int AR_Y_END;

struct ar0xxx_reg {
	u16	reg;
	u16	val;
};

#include "ar0220.h"
#include "ar0233.h"

static const int ar0233_i2c_addr[] = {0x10, 0x20};

#define AR_PID_REG		0x3000
#define AR_REV_REG		0x31FE

enum {
	AR0220_PID = 0x0C54,
	AR0233_PID = 0x0956,
} chipid;

#define AR_MEDIA_BUS_FMT	MEDIA_BUS_FMT_SGRBG12_1X12

struct ar0233_priv {
	struct v4l2_subdev		sd;
	struct v4l2_ctrl_handler	hdl;
	struct media_pad		pad;
	struct v4l2_rect		rect;
	int				fps_denominator;
	int				fps_numerator;
	int				init_complete;
	u8				id[6];
	bool				emb_enable;
	/* serializers */
	int				ti9x4_addr;
	int				ti9x3_addr;
	int				port;
	int				trigger;
	int				vts;
	int				vts_ref;
};

static int extclk = 23;
module_param(extclk, int, 0644);
MODULE_PARM_DESC(extclk, " EXTCLK value in MHz (default: 23) ");

static char *mode = "hdr";
module_param(mode, charp, 0644);
MODULE_PARM_DESC(mode, " Modes linear,hdr,seplus,seplus60 (default: hdr)");

static inline struct ar0233_priv *to_ar0233(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ar0233_priv, sd);
}

static int ar0233_set_regs(struct i2c_client *client, const struct ar0xxx_reg **pregs)
{
	const struct ar0xxx_reg *regs;
	int i, j;

	for (j = 0; ; j++) {
		regs = pregs[j];

		if (!pregs[j])
			break;

		for (i = 0; ; i++) {
			if (!regs[i].reg && !regs[i].val)
				break;

			if (regs[i].reg == AR_DELAY) {
				mdelay(regs[i].val);
				continue;
			}

			reg16_write16(client, regs[i].reg, regs[i].val);
		}
	}

	return 0;
}

static int ar0233_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int ar0233_set_window(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	dev_dbg(&client->dev, "L=%d T=%d %dx%d\n", priv->rect.left, priv->rect.top, priv->rect.width, priv->rect.height);

	/* horiz crop start */
	reg16_write16(client, 0x3004, priv->rect.left + AR_X_START);
	/* horiz crop end */
	reg16_write16(client, 0x3008, priv->rect.left + priv->rect.width - 1 + AR_X_START);
	/* vert crop start */
	reg16_write16(client, 0x3002, priv->rect.top + AR_Y_START);
	/* vert crop end */
	/* limit window for embedded data/stats */
	reg16_write16(client, 0x3006, priv->rect.top + priv->rect.height - 1 + AR_Y_START);

	return 0;
};

static int ar0233_get_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	if (format->pad)
		return -EINVAL;

	mf->width = priv->rect.width;
	mf->height = priv->rect.height;
	mf->code = AR_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

static int ar0233_set_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	mf->code = AR_MEDIA_BUS_FMT;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
	mf->field = V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *mf;

	return 0;
}

static int ar0233_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index > 0)
		return -EINVAL;

	code->code = AR_MEDIA_BUS_FMT;

	return 0;
}

static int ar0233_get_edid(struct v4l2_subdev *sd, struct v4l2_edid *edid)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	memcpy(edid->edid, priv->id, 6);

	edid->edid[6] = 0xff;
	edid->edid[7] = client->addr;
	edid->edid[8] = chipid >> 8;
	edid->edid[9] = chipid & 0xff;

	return 0;
}

static int ar0233_set_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *rect = &sel->r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE ||
	    sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	rect->left = ALIGN(rect->left, 2);
	rect->top = ALIGN(rect->top, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if ((rect->left + rect->width > AR_MAX_WIDTH) ||
	    (rect->top + rect->height > AR_MAX_HEIGHT))
		*rect = priv->rect;

	priv->rect.left = rect->left;
	priv->rect.top = rect->top;
	priv->rect.width = rect->width;
	priv->rect.height = rect->height;

	ar0233_set_window(sd);

	return 0;
}

static int ar0233_get_selection(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = AR_MAX_WIDTH;
		sel->r.height = AR_MAX_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = AR_DEFAULT_WIDTH;
		sel->r.height = AR_DEFAULT_HEIGHT;
		return 0;
	case V4L2_SEL_TGT_CROP:
		sel->r = priv->rect;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r.left = 0;
		sel->r.top = AR_EMB_PADDED;
		sel->r.width = priv->rect.width;
		sel->r.height = priv->rect.height;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ar0233_g_mbus_config(struct v4l2_subdev *sd,
			       struct v4l2_mbus_config *cfg)
{
	cfg->flags = V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0 |
		     V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	cfg->type = V4L2_MBUS_CSI2;

	return 0;
}

static int ar0233_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = priv->fps_numerator;
	cp->timeperframe.denominator = priv->fps_denominator;

	return 0;
}

static int ar0233_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);
	struct v4l2_captureparm *cp = &parms->parm.capture;
	int ret = 0;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;

	if (priv->fps_denominator != cp->timeperframe.denominator ||
	    priv->fps_numerator != cp->timeperframe.numerator) {
		priv->vts = priv->vts_ref * 30 * cp->timeperframe.numerator / cp->timeperframe.denominator;

		ret = reg16_write16(client, 0x300A, priv->vts);		/* FRAME_LENGTH_LINES_ */

		priv->fps_numerator = cp->timeperframe.numerator;
		priv->fps_denominator = cp->timeperframe.denominator;
	}

	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ar0233_g_register(struct v4l2_subdev *sd,
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

static int ar0233_s_register(struct v4l2_subdev *sd,
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

static struct v4l2_subdev_core_ops ar0233_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ar0233_g_register,
	.s_register = ar0233_s_register,
#endif
};

static int ar0233_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);
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
		if (ctrl->val < 0x600 * 30 * priv->fps_numerator / priv->fps_denominator)
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
		ret = reg16_read16(client, 0x350e, &val);
		if (ctrl->val)
			val |= (1 << 0);
		else
			val &= ~(1 << 0);
		ret |= reg16_write16(client, 0x350e, val);
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ret = 0;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ar0233_ctrl_ops = {
	.s_ctrl = ar0233_s_ctrl,
};

static struct v4l2_subdev_video_ops ar0233_video_ops = {
	.s_stream	= ar0233_s_stream,
	.g_mbus_config	= ar0233_g_mbus_config,
	.g_parm		= ar0233_g_parm,
	.s_parm		= ar0233_s_parm,
};

static const struct v4l2_subdev_pad_ops ar0233_subdev_pad_ops = {
	.get_edid	= ar0233_get_edid,
	.enum_mbus_code	= ar0233_enum_mbus_code,
	.get_selection	= ar0233_get_selection,
	.set_selection	= ar0233_set_selection,
	.get_fmt	= ar0233_get_fmt,
	.set_fmt	= ar0233_set_fmt,
};

static struct v4l2_subdev_ops ar0233_subdev_ops = {
	.core	= &ar0233_core_ops,
	.video	= &ar0233_video_ops,
	.pad	= &ar0233_subdev_pad_ops,
};

static void ar0233_otp_id_read(struct i2c_client *client)
{
	struct ar0233_priv *priv = to_ar0233(client);
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

static ssize_t ar0233_otp_id_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	ar0233_otp_id_read(client);

	return snprintf(buf, 32, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
}

static ssize_t ar0233_emb_enable_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t count)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);
	u32 val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;
	priv->emb_enable = !!val;

	switch (chipid) {
	case AR0220_PID:
		reg16_write16(client, 0x3064, priv->emb_enable ? 0x1982 : 0x1802); break;
	case AR0233_PID:
		reg16_write16(client, 0x3064, priv->emb_enable ? 0x0180 : 0); break;
	default:
		return -EINVAL;
	}

	return count;
}

static ssize_t ar0233_emb_enable_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0233_priv *priv = to_ar0233(client);

	return snprintf(buf, 4, "%d\n", priv->emb_enable);
}

static DEVICE_ATTR(otp_id, S_IRUGO, ar0233_otp_id_show, NULL);
static DEVICE_ATTR(emb_enable, S_IRUGO|S_IWUSR, ar0233_emb_enable_show, ar0233_emb_enable_store);

static int ar0233_initialize(struct i2c_client *client)
{
	struct ar0233_priv *priv = to_ar0233(client);
	char chip_name[10] = "unknown";
	u16 val = 0;
	u16 pid = 0, rev = 0;
	int ret = 0;
	int tmp_addr;
	int i;

	for (i = 0; i < ARRAY_SIZE(ar0233_i2c_addr); i++) {
		tmp_addr = client->addr;
		if (priv->ti9x4_addr) {
			client->addr = priv->ti9x4_addr;/* Deserializer I2C address */
			reg8_write(client, 0x5d, ar0233_i2c_addr[i] << 1); /* Sensor native I2C address */
			usleep_range(2000, 2500);	/* wait 2ms */
		}
		client->addr = tmp_addr;

		/* check model ID */
		reg16_read16(client, AR_PID_REG, &pid);

		if (pid == AR0220_PID || pid == AR0233_PID)
			break;
	}

	if (pid != AR0220_PID && pid != AR0233_PID) {
		dev_dbg(&client->dev, "Product ID error %x\n", pid);
		ret = -ENODEV;
		goto err;
	}

	/* check revision  */
	reg16_read16(client, AR_REV_REG, &rev);
	/* Read OTP IDs */
	ar0233_otp_id_read(client);

	switch (pid) {
		case AR0220_PID:
			chipid = AR0220_PID;
			strncpy(chip_name, "AR0220", 10);
			AR_MAX_WIDTH = AR0220_MAX_WIDTH;
			AR_MAX_HEIGHT = AR0220_MAX_HEIGHT;
			AR_DEFAULT_WIDTH = AR0220_DEFAULT_WIDTH;
			AR_DEFAULT_HEIGHT = AR0220_DEFAULT_HEIGHT;
			AR_X_START = AR0220_X_START;
			AR_Y_START = AR0220_Y_START;
			AR_X_END = AR0220_X_END;
			AR_Y_END = AR0220_Y_END;

			if (extclk == 25) {
				ar0220_regs_hdr_mipi_12bit_36fps_rev2[3] = ar0220_rev2_pll_25_4lane_12b;
				ar0220_regs_hdr_mipi_12bit_36fps_rev3[3] = ar0220_rev2_pll_25_4lane_12b;
				ar0220_regs_hdr_mipi_12bit_36fps_rev4[2] = ar0220_rev2_pll_25_4lane_12b;
			}
			if (extclk == 27) {
				ar0220_regs_hdr_mipi_12bit_36fps_rev2[3] = ar0220_rev2_pll_27_4lane_12b;
				ar0220_regs_hdr_mipi_12bit_36fps_rev3[3] = ar0220_rev2_pll_27_4lane_12b;
				ar0220_regs_hdr_mipi_12bit_36fps_rev4[2] = ar0220_rev2_pll_27_4lane_12b;
			}

			switch (rev & 0xf) {
			case 0x1:
			case 0x2:
				ar0233_set_regs(client, ar0220_regs_hdr_mipi_12bit_36fps_rev2);
				break;
			case 0x3:
				ar0233_set_regs(client, ar0220_regs_hdr_mipi_12bit_36fps_rev3);
				break;
			case 0x4:
				ar0233_set_regs(client, ar0220_regs_hdr_mipi_12bit_36fps_rev4);
				break;
			default:
				dev_err(&client->dev, "Unsupported chip revision\n");
			}
			break;
		case AR0233_PID:
			chipid = AR0233_PID;
			strncpy(chip_name, "AR0233", 10);
			AR_MAX_WIDTH = AR0233_MAX_WIDTH;
			AR_MAX_HEIGHT = AR0233_MAX_HEIGHT;
			AR_DEFAULT_WIDTH = AR0233_DEFAULT_WIDTH;
			AR_DEFAULT_HEIGHT = AR0233_DEFAULT_HEIGHT;
			AR_X_START = AR0233_X_START;
			AR_Y_START = AR0233_Y_START;
			AR_X_END = AR0233_X_END;
			AR_Y_END = AR0233_Y_END;

			/* Program wizard registers */
			switch (rev & 0xf) {
			case 0x1:
				ar0233_set_regs(client, ar0233_regs_hdr_mipi_12bit_30fps_rev1);
				break;
			case 0x2:
				if (extclk == 27) {
					ar0233_regs_hdr_mipi_12bit_30fps_rev2[4] = ar0233_rev2_pll_27_102_4lane_12b;
					ar0233_regs_seplus_mipi_12bit_30fps_rev2[2] = ar0233_rev2_pll_27_102_4lane_12b;
				}

				if (strcmp(mode, "hdr") == 0)
					ar0233_set_regs(client, ar0233_regs_hdr_mipi_12bit_30fps_rev2);
				else if (strcmp(mode, "seplus") == 0)
					ar0233_set_regs(client, ar0233_regs_seplus_mipi_12bit_30fps_rev2);
				else if (strcmp(mode, "seplus60") == 0) {
					AR_MAX_WIDTH = 1920;
					AR_MAX_HEIGHT = 1080;
					AR_DEFAULT_WIDTH = 1920;
					AR_DEFAULT_HEIGHT = 1080;

					ar0233_set_regs(client, ar0233_regs_seplus_mipi_12bit_60fps_rev2);
					if (extclk != 27) {
						reg16_read16(client, 0x3030, &val);
						reg16_write16(client, 0x3030, val * 27 / extclk + 1);
					}
				} else
					dev_err(&client->dev, "Unsupported mode %s\n", mode);
				break;
			default:
				dev_err(&client->dev, "Unsupported chip revision\n");
			}
			break;
	}

	reg16_read16(client, 0x300A, &val);	/* FRAME_LENGTH_LINES_ */
	priv->vts_ref = val;

	/* Enable trigger */
	if (priv->trigger >= 0 && priv->trigger < 4) {
		reg16_write16(client, 0x340A, (~(BIT(priv->trigger) << 4)) & 0xf0);	/* GPIO_CONTROL1: GPIOn input enable */
		reg16_write16(client, 0x340C, (0x2 << 2*priv->trigger));		/* GPIO_CONTROL2: GPIOn is trigger */
		reg16_write16(client, 0x30CE, 0x0120);					/* TRIGGER_MODE */
		//reg16_write16(client, 0x30DC, 0x0120);				/* TRIGGER_DELAY */
	}

	/* Enable stream */
	reg16_read16(client, 0x301a, &val);	// read inital reset_register value
	val |= (1 << 8);			/* GPI pins enable */
	val |= (1 << 2);			// Set streamOn bit
	reg16_write16(client, 0x301a, val);	// Start Streaming

	dev_info(&client->dev, "%s PID %x (rev%x), res %dx%d, mode=%s, OTP_ID %02x:%02x:%02x:%02x:%02x:%02x\n", chip_name,
		 pid, rev & 0xf, AR_MAX_WIDTH, AR_MAX_HEIGHT, mode, priv->id[0], priv->id[1], priv->id[2], priv->id[3], priv->id[4], priv->id[5]);
err:
	return ret;
}

static int ar0233_parse_dt(struct device_node *np, struct ar0233_priv *priv)
{
	struct i2c_client *client = v4l2_get_subdevdata(&priv->sd);
	int i;
	struct device_node *endpoint = NULL, *rendpoint = NULL;
	int tmp_addr = 0;

	for (i = 0; ; i++) {
		endpoint = of_graph_get_next_endpoint(np, endpoint);
		if (!endpoint)
			break;

		of_property_read_u32(endpoint, "trigger", &priv->trigger);

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
		dev_dbg(&client->dev, "deserializer does not present\n");
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

	/* module params override dts */
	if (trigger)
		priv->trigger = trigger;

	return 0;
}

static int ar0233_probe(struct i2c_client *client,
		       const struct i2c_device_id *did)
{
	struct ar0233_priv *priv;
	int ret;

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	v4l2_i2c_subdev_init(&priv->sd, client, &ar0233_subdev_ops);
	priv->sd.flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->fps_numerator = 1;
	priv->fps_denominator = 30;
	priv->emb_enable = 1;

	v4l2_ctrl_handler_init(&priv->hdl, 4);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_BRIGHTNESS, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_CONTRAST, 0, 16, 1, 7);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_SATURATION, 0, 7, 1, 2);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_HUE, 0, 23, 1, 12);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_GAMMA, -128, 128, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_SHARPNESS, 0, 10, 1, 3);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_AUTOGAIN, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_GAIN, 1, 0x7ff, 1, 0x200);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, 1, 0xe, 1, 0xa);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_EXPOSURE, 1, 0x600 * 30, 1, 0x144);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 1);
	v4l2_ctrl_new_std(&priv->hdl, &ar0233_ctrl_ops,
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

	ret = ar0233_parse_dt(client->dev.of_node, priv);
	if (ret)
		goto cleanup;

	ret = ar0233_initialize(client);
	if (ret < 0)
		goto cleanup;

	priv->rect.left = 0;
	priv->rect.top = 0;
	priv->rect.width = AR_DEFAULT_WIDTH;
	priv->rect.height = AR_DEFAULT_HEIGHT;

	ret = v4l2_async_register_subdev(&priv->sd);
	if (ret)
		goto cleanup;

	if (device_create_file(&client->dev, &dev_attr_otp_id) != 0 ||
	    device_create_file(&client->dev, &dev_attr_emb_enable) != 0) {
		dev_err(&client->dev, "sysfs entry creation failed\n");
		goto cleanup;
	}

	priv->init_complete = 1;

	return 0;

cleanup:
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);
#ifdef CONFIG_SOC_CAMERA_AR0233
	v4l_err(client, "failed to probe @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
#endif
	return ret;
}

static int ar0233_remove(struct i2c_client *client)
{
	struct ar0233_priv *priv = i2c_get_clientdata(client);

	device_remove_file(&client->dev, &dev_attr_otp_id);
	device_remove_file(&client->dev, &dev_attr_emb_enable);
	v4l2_async_unregister_subdev(&priv->sd);
	media_entity_cleanup(&priv->sd.entity);
	v4l2_ctrl_handler_free(&priv->hdl);
	v4l2_device_unregister_subdev(&priv->sd);

	return 0;
}

#ifdef CONFIG_SOC_CAMERA_AR0233
static const struct i2c_device_id ar0233_id[] = {
	{ "ar0233", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0233_id);

static const struct of_device_id ar0233_of_ids[] = {
	{ .compatible = "aptina,ar0233", },
	{ }
};
MODULE_DEVICE_TABLE(of, ar0233_of_ids);

static struct i2c_driver ar0233_i2c_driver = {
	.driver	= {
		.name		= "ar0233",
		.of_match_table	= ar0233_of_ids,
	},
	.probe		= ar0233_probe,
	.remove		= ar0233_remove,
	.id_table	= ar0233_id,
};

module_i2c_driver(ar0233_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for AR0233");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");
#endif
