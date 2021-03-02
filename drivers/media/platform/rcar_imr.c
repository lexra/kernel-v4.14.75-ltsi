/*
 * rcar_imr.c  --  R-Car IMR-LX4 Driver
 *
 * Copyright (C) 2015  Cogent Embedded, Inc.  <source@cogentembedded.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/rcar-imr.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

#define DRV_NAME                        "rcar_imr"

/*******************************************************************************
 * Module parameters
 ******************************************************************************/

static int debug;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-4)");

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/

/* Number of RSE planes on V3H (non scaled, 1/2, 1/4, 1/8) */
#define RSE_PLANES_NUM 4

/* ...configuration data */
struct imr_cfg {
	/* ...display-list main program data */
	void                   *dl_vaddr;
	dma_addr_t              dl_dma_addr;
	u32                     dl_size;
	u32                     dl_start_offset;

	/* ...pointers to the source/destination planes */
	u32                    *src_pa_ptr[2];
	u32                    *dst_pa_ptr[2];
	/* ...pointers to the RSE destination planes */
	u32                    *dstn_pa_ptr[RSE_PLANES_NUM];
	u32                    *dstr_pa_ptr[RSE_PLANES_NUM];

	/* ...offsets to RSE destination planes */
	u32                     dstnr_offsets[IMR_EXTDST_NUM];

	/* ...RSE logical right shift data */
	u32                    *rscr_ptr;
	u8                      rscr_sc8, rscr_sc4, rscr_sc2;

	/* ...RSE destination stride values */
	u32                     dstnr_strides[IMR_EXTDST_NUM];
	u32                    *striden_ptr[RSE_PLANES_NUM];
	u32                    *strider_ptr[RSE_PLANES_NUM];

	/* ...subpixel destination coordinates space */
	int                     dst_subpixel;

	/* ...reference counter */
	u32                     refcount;

	/* ...identifier (for debug output) */
	u32                     id;
};

struct imr_buffer {
	/* ...standard M2M buffer descriptor */
	struct v4l2_m2m_buffer      buf;

	/* ...pointer to mesh configuration for processing */
	struct imr_cfg             *cfg;
};

struct imr_q_data {
	/* ...latched pixel format */
	struct v4l2_pix_format      fmt;

	/* ...current format flags */
	u32                         flags;
};

struct imr_format_info {
	char       *name;
	u32         fourcc;
	u32         flags;
};

/* ...per-device data */
struct imr_device {
	struct device          *dev;
	struct clk             *clock;
	void __iomem           *mmio;
	int                     irq;
	struct mutex            mutex;
	spinlock_t              lock;

	struct v4l2_device      v4l2_dev;
	struct video_device     video_dev;
	struct v4l2_m2m_dev    *m2m_dev;
	struct device          *alloc_dev;

	bool			rse;

	/* ...do we need that counter really? framework counts fh structures for us - tbd */
	int                     refcount;

	/* ...should we include media-dev? likely, no - tbd */
};

/* ...per file-handle context */
struct imr_ctx {
	struct v4l2_fh          fh;
	struct imr_device      *imr;
	struct v4l2_m2m_ctx    *m2m_ctx;
	struct imr_q_data       queue[2];

	/* ...current job configuration */
	struct imr_cfg         *cfg;

	/* ...frame sequence counter */
	u32                     sequence;

	/* ...cropping parameters (in pixels) */
	u16                     crop[4];

	/* ...solid color code */
	u32                     color;

	/* ...number of active configurations (debugging) */
	u32                     cfg_num;
};

/*******************************************************************************
 * IMR registers
 ******************************************************************************/

#define IMR_CR                          0x08
#define IMR_CR_RS                       (1 << 0)
#define IMR_CR_SWRST                    (1 << 15)

#define IMR_SR                          0x0C
#define IMR_SRCR                        0x10
#define IMR_SR_TRA                      (1 << 0)
#define IMR_SR_IER                      (1 << 1)
#define IMR_SR_INT                      (1 << 2)
#define IMR_SR_REN                      (1 << 5)

#define IMR_ICR                         0x14
#define IMR_IMR                         0x18
#define IMR_ICR_TRAEN                   (1 << 0)
#define IMR_ICR_IEREN                   (1 << 1)
#define IMR_ICR_INTEN                   (1 << 2)

#define IMR_DLSP                        0x1C
#define IMR_DLSR                        0x20
#define IMR_DLSAR                       0x30

#define IMR_DSAR                        0x34
#define IMR_SSAR                        0x38
#define IMR_DSTR                        0x3C
#define IMR_SSTR                        0x40
#define IMR_DSOR                        0x50

#define IMR_CMRCR                       0x54
#define IMR_CMRCSR                      0x58
#define IMR_CMRCCR                      0x5C
#define IMR_CMR_LUCE                    (1 << 1)
#define IMR_CMR_CLCE                    (1 << 2)
#define IMR_CMR_DUV_SHIFT               3
#define IMR_CMR_DUV_MASK                (3 << IMR_CMR_DUV_SHIFT)
#define IMR_CMR_SUV_SHIFT               5
#define IMR_CMR_SUV_MASK                (3 << IMR_CMR_SUV_SHIFT)
#define IMR_CMR_YISM                    (1 << 7)
#define IMR_CMR_DY10                    (1 << 8)
#define IMR_CMR_DY12                    (1 << 9)
#define IMR_CMR_SY10                    (1 << 11)
#define IMR_CMR_SY12                    (1 << 12)
#define IMR_CMR_YCM                     (1 << 14)
#define IMR_CMR_CP16E                   (1 << 15)

#define IMR_CMRCR2                      0xE4
#define IMR_CMRCSR2                     0xE8
#define IMR_CMRCCR2                     0xEC
#define IMR_CMR2_LUTE                   (1 << 0)
#define IMR_CMR2_YUV422E                (1 << 2)
#define IMR_CMR2_YUV422FORM             (1 << 5)
#define IMR_CMR2_UVFORM                 (1 << 6)
#define IMR_CMR2_TCTE                   (1 << 12)
#define IMR_CMR2_DCTE                   (1 << 15)

#define IMR_TRIMR                       0x60
#define IMR_TRIMSR                      0x64
#define IMR_TRIMCR                      0x68
#define IMR_TRIM_TME                    (1 << 0)
#define IMR_TRIM_BFE                    (1 << 1)
#define IMR_TRIM_AUTODG                 (1 << 2)
#define IMR_TRIM_AUTOSG                 (1 << 3)
#define IMR_TRIM_DYDXM                  (1 << 4)
#define IMR_TRIM_DUDVM                  (1 << 5)
#define IMR_TRIM_TCM                    (1 << 6)

#define IMR_TRICR                       0x6C
#define IMR_TRIC_YCFORM                 (1 << 31)
#define IMR_TRICR2                      0xA0

#define IMR_UVDPOR                      0x70
#define IMR_SUSR                        0x74
#define IMR_SVSR                        0x78

#define IMR_XMINR                       0x80
#define IMR_YMINR                       0x84
#define IMR_XMAXR                       0x88
#define IMR_YMAXR                       0x8C

#define IMR_AMXSR                       0x90
#define IMR_AMYSR                       0x94
#define IMR_AMXOR                       0x98
#define IMR_AMYOR                       0x9C

#define IMR_CPDPOR                      0xD0
#define IMR_CPDP_YLDPO_SHIFT            8
#define IMR_CPDP_UBDPO_SHIFT            4
#define IMR_CPDP_VRDPO_SHIFT            0

#define IMR_TPOR                        0xF0

#define IMR_RSCSR			0x204
#define IMR_RSCCR			0x208
#define IMR_RSCR_RSE			31
#define IMR_RSCR_SC8			25
#define IMR_RSCR_SC4			21
#define IMR_RSCR_SC2			17

#define IMR_DSANRR0                     0x210
#define IMR_DSTNRR0                     0x214
#define IMR_DSARR0                      0x218
#define IMR_DSTRR0                      0x21C

/*******************************************************************************
 * Auxiliary helpers
 ******************************************************************************/

static inline struct imr_ctx * fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct imr_ctx, fh);
}

static inline struct imr_buffer * to_imr_buffer(struct vb2_v4l2_buffer *vbuf)
{
	struct v4l2_m2m_buffer *b = container_of(vbuf, struct v4l2_m2m_buffer, vb);

	return container_of(b, struct imr_buffer, buf);
}

/*******************************************************************************
 * Local constants definition
 ******************************************************************************/

#define IMR_F_Y8                        (1 << 0)
#define IMR_F_Y10                       (1 << 1)
#define IMR_F_Y12                       (1 << 2)
#define IMR_F_UV8                       (1 << 3)
#define IMR_F_UV10                      (1 << 4)
#define IMR_F_UV12                      (1 << 5)
#define IMR_F_PLANAR                    (1 << 6)
#define IMR_F_INTERLEAVED               (1 << 7)
#define IMR_F_PLANES_MASK               ((1 << 8) - 1)
#define IMR_F_UV_SWAP                   (1 << 8)
#define IMR_F_YUV_SWAP                  (1 << 9)

/* ...get common planes bits */
static inline u32 __imr_flags_common(u32 iflags, u32 oflags)
{
	return (iflags & oflags) & IMR_F_PLANES_MASK;
}

static const struct imr_format_info imr_lx4_formats[] = {
	{
		.name = "YUV 4:2:2 semiplanar (NV16)",
		.fourcc = V4L2_PIX_FMT_NV16,
		.flags = IMR_F_Y8 | IMR_F_UV8 | IMR_F_PLANAR,
	},
	{
		.name = "YVU 4:2:2 semiplanar (NV61)",
		.fourcc = V4L2_PIX_FMT_NV61,
		.flags = IMR_F_Y8 | IMR_F_UV8 | IMR_F_PLANAR | IMR_F_UV_SWAP,
	},
	{
		.name = "YUV 4:2:2 interleaved (YUYV)",
		.fourcc = V4L2_PIX_FMT_YUYV,
		.flags = IMR_F_Y8 | IMR_F_UV8,
	},
	{
		.name = "YUV 4:2:2 interleaved (UYVY)",
		.fourcc = V4L2_PIX_FMT_UYVY,
		.flags = IMR_F_Y8 | IMR_F_UV8 | IMR_F_YUV_SWAP,
	},
	{
		.name = "YUV 4:2:2 interleaved (YVYU)",
		.fourcc = V4L2_PIX_FMT_YVYU,
		.flags = IMR_F_Y8 | IMR_F_UV8 | IMR_F_UV_SWAP,
	},
	{
		.name = "YUV 4:2:2 interleaved (UYVY)",
		.fourcc = V4L2_PIX_FMT_VYUY,
		.flags = IMR_F_Y8 | IMR_F_UV8 | IMR_F_UV_SWAP | IMR_F_YUV_SWAP,
	},
	{
		.name = "Greyscale 8-bit",
		.fourcc = V4L2_PIX_FMT_GREY,
		.flags = IMR_F_Y8 | IMR_F_PLANAR,
	},
	{
		.name = "Greyscale 10-bit",
		.fourcc = V4L2_PIX_FMT_Y10,
		.flags = IMR_F_Y8 | IMR_F_Y10 | IMR_F_PLANAR,
	},
	{
		.name = "Greyscale 12-bit",
		.fourcc = V4L2_PIX_FMT_Y12,
		.flags = IMR_F_Y8 | IMR_F_Y10 | IMR_F_Y12 | IMR_F_PLANAR,
	},
	{
		.name = "Chrominance UV 8-bit",
		.fourcc = V4L2_PIX_FMT_UV8,
		.flags = IMR_F_UV8 | IMR_F_PLANAR,
	},
};

/* ...mesh configuration constructor */
static struct imr_cfg * imr_cfg_create(struct imr_ctx *ctx, u32 dl_size, u32 dl_start)
{
	struct imr_device  *imr = ctx->imr;
	struct imr_cfg     *cfg;

	/* ...allocate configuration descriptor */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg) {
		v4l2_err(&imr->v4l2_dev, "failed to allocate configuration descriptor\n");
		return ERR_PTR(-ENOMEM);
	}

	/* ...allocate contiguous memory for a display list */
	cfg->dl_vaddr = dma_alloc_writecombine(imr->dev, dl_size, &cfg->dl_dma_addr, GFP_KERNEL);
	if (!cfg->dl_vaddr) {
		v4l2_err(&imr->v4l2_dev, "failed to allocate %u bytes for a DL\n", dl_size);
		kfree(cfg);
		return ERR_PTR(-ENOMEM);
	}

	cfg->dl_size = dl_size;
	cfg->dl_start_offset = dl_start;
	cfg->refcount = 1;
	cfg->id = ctx->sequence;

	/* ...for debugging purposes, advance number of active configurations */
	ctx->cfg_num++;

	return cfg;
}

/* ...add reference to the current configuration */
static inline struct imr_cfg * imr_cfg_ref(struct imr_ctx *ctx)
{
	struct imr_cfg  *cfg = ctx->cfg;

	BUG_ON(!cfg);
	cfg->refcount++;
	return cfg;
}

/* ...mesh configuration destructor */
static void imr_cfg_unref(struct imr_ctx *ctx, struct imr_cfg *cfg)
{
	struct imr_device *imr = ctx->imr;

	/* ...no atomicity is required as operation is locked with device mutex */
	if (!cfg || --cfg->refcount)
		return;

	/* ...release memory allocated for a display list */
	if (cfg->dl_vaddr)
		dma_free_writecombine(imr->dev, cfg->dl_size, cfg->dl_vaddr, cfg->dl_dma_addr);

	/* ...destroy the configuration structure */
	kfree(cfg);

	/* ...decrement number of active configurations (debugging) */
	WARN_ON(!ctx->cfg_num--);
}



/*******************************************************************************
 * Context processing queue
 ******************************************************************************/

static int imr_queue_setup(struct vb2_queue *vq,
			unsigned int *nbuffers, unsigned int *nplanes,
			unsigned int sizes[], struct device *alloc_devs[])
{
	struct imr_ctx     *ctx = vb2_get_drv_priv(vq);
	struct imr_q_data  *q_data = &ctx->queue[V4L2_TYPE_IS_OUTPUT(vq->type) ? 0 : 1];
	int                 s = q_data->fmt.bytesperline;
	int                 h = q_data->fmt.height;

	/* ...we use only single-plane formats */
	*nplanes = 1;

	v4l2_dbg(1, debug, &ctx->imr->v4l2_dev, "format: %c%c%c%c, s=%d, h=%d\n",
		(q_data->fmt.pixelformat >> 0) & 0xff, (q_data->fmt.pixelformat >> 8) & 0xff,
		(q_data->fmt.pixelformat >> 16) & 0xff, (q_data->fmt.pixelformat >> 24) & 0xff,
		s, h);

	/* ...specify plane size */
	switch (q_data->fmt.pixelformat) {
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_Y10:
	case V4L2_PIX_FMT_Y12:
	case V4L2_PIX_FMT_Y16:
	case V4L2_PIX_FMT_UV8:
	case V4L2_PIX_FMT_GREY:
		sizes[0] = s * h;
		break;

	case V4L2_PIX_FMT_NV16:
		sizes[0] = s * h * 2;
		break;

	default:
		return -EINVAL;
	}

	/* ...specify default allocator */
	alloc_devs[0] = ctx->imr->alloc_dev;

	return 0;
}

static int imr_buf_prepare(struct vb2_buffer *vb)
{
	/* struct imr_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue); */

	/* ...unclear yet if we want to prepare a buffer somehow (cache invalidation? - tbd) */
	return 0;
}

static void imr_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue       *q = vb->vb2_queue;
	struct imr_ctx         *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	WARN_ON_ONCE(!mutex_is_locked(&ctx->imr->mutex));

	v4l2_dbg(3, debug, &ctx->imr->v4l2_dev, "%sput buffer <0x%08llx> submitted\n",
			q->is_output ? "in" : "out",
			vb2_dma_contig_plane_dma_addr(vb, 0));

	/* ...for input buffer, put current configuration pointer (add reference) */
	if (q->is_output)
		to_imr_buffer(vbuf)->cfg = imr_cfg_ref(ctx);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vbuf);
}

static void imr_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_queue       *q = vb->vb2_queue;
	struct imr_ctx         *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	WARN_ON(!mutex_is_locked(&ctx->imr->mutex));

	/* ...any special processing of completed buffer? - tbd */
	v4l2_dbg(3, debug, &ctx->imr->v4l2_dev, "%sput buffer <0x%08llx> done (err: %d) (ctx=%p)\n",
			q->is_output ? "in" : "out",
			vb2_dma_contig_plane_dma_addr(vb, 0),
			vb->state, ctx);

	/* ...unref configuration pointer as needed */
	if (q->is_output)
		imr_cfg_unref(ctx, to_imr_buffer(vbuf)->cfg);
}

static int imr_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct imr_ctx     *ctx = vb2_get_drv_priv(vq);
	int                 ret;

	ret = 0;//pm_runtime_get_sync(ctx->imr->dev);
	if (ret < 0) {
		v4l2_err(&ctx->imr->v4l2_dev, "failed to start %s streaming: %d\n",
			(V4L2_TYPE_IS_OUTPUT(vq->type) ? "output" : "capture"), ret);
		return ret;
	} else {
		v4l2_dbg(1, debug, &ctx->imr->v4l2_dev, "%s streaming started\n",
			(V4L2_TYPE_IS_OUTPUT(vq->type) ? "output" : "capture"));
		return 0;
	}
}

static void imr_stop_streaming(struct vb2_queue *vq)
{
	struct imr_ctx             *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer     *vb;
	unsigned long               flags;

	spin_lock_irqsave(&ctx->imr->lock, flags);

	/* ...purge all buffers from a queue */
	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		while ((vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx)) != NULL)
			v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);
	} else {
		while ((vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx)) != NULL)
			v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);
	}

	spin_unlock_irqrestore(&ctx->imr->lock, flags);

	v4l2_dbg(1, debug, &ctx->imr->v4l2_dev, "%s streaming stopped\n",
		(V4L2_TYPE_IS_OUTPUT(vq->type) ? "output" : "capture"));

	//pm_runtime_put(ctx->imr->dev);
}

/* ...buffer queue operations */
static struct vb2_ops imr_qops = {
	.queue_setup        = imr_queue_setup,
	.buf_prepare        = imr_buf_prepare,
	.buf_queue          = imr_buf_queue,
	.buf_finish         = imr_buf_finish,
	.start_streaming    = imr_start_streaming,
	.stop_streaming     = imr_stop_streaming,
	.wait_prepare       = vb2_ops_wait_prepare,
	.wait_finish        = vb2_ops_wait_finish,
};

/* ...M2M device processing queue initialization */
static int imr_queue_init(void *priv, struct vb2_queue *src_vq,
			struct vb2_queue *dst_vq)
{
	struct imr_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct imr_buffer);
	src_vq->ops = &imr_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	src_vq->lock = &ctx->imr->mutex;
	src_vq->dev = ctx->imr->v4l2_dev.dev;
	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->ops = &imr_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	dst_vq->lock = &ctx->imr->mutex;
	ret = vb2_queue_init(dst_vq);
	if (ret)
		return ret;

	return 0;
}

/*******************************************************************************
 * Display list commands
 ******************************************************************************/

/* ...display list opcodes */
#define IMR_OP_TRI(n)                   ((0x8A << 24) | ((n) & 0xFFFF))
#define IMR_OP_LINE(n)                  ((0x8B << 24) | ((n) & 0xFFFF))
#define IMR_OP_NOP(n)                   ((0x80 << 24) | ((n) & 0xFFFF))
#define IMR_OP_TRAP                     ((0x8F << 24))
#define IMR_OP_WTL(add, n)              ((0x81 << 24) | (((add) / 4) << 16) | ((n) & 0xFFFF))
#define IMR_OP_WTS(add, data)           ((0x82 << 24) | (((add) / 4) << 16) | ((data) & 0xFFFF))
#define IMR_OP_WTL2(add, n)             ((0x83 << 24) | (((add) / 4) << 10) | ((n) & 0x3FF))
#define IMR_OP_INT                      ((0x88 << 24))
#define IMR_OP_SYNCM                    ((0x86 << 24))
#define IMR_OP_GOSUB                    ((0x8C << 24))
#define IMR_OP_RET                      ((0x8D << 24))

/*******************************************************************************
 * Operation type decoding helpers
 ******************************************************************************/

static inline u16 __imr_auto_sg_dg_tcm(u32 type)
{
	return (type & IMR_MAP_AUTOSG ? IMR_TRIM_AUTOSG : (type & IMR_MAP_AUTODG ? IMR_TRIM_AUTODG : 0)) |
		(type & IMR_MAP_TCM ? IMR_TRIM_TCM : 0);
}

static inline u16 __imr_bfe_tme(u32 type)
{
	return (type & IMR_MAP_TME ? IMR_TRIM_TME : 0) | (type & IMR_MAP_BFE ? IMR_TRIM_BFE : 0);
}

static inline u16 __imr_uvdp(u32 type)
{
	return __IMR_MAP_UVDPOR(type) | (type & IMR_MAP_DDP ? (1 << 8) : 0);
}

static inline u16 __imr_cpdp(u32 type)
{
	return (__IMR_MAP_YLDPO(type) << 8) | (__IMR_MAP_UBDPO(type) << 4) | __IMR_MAP_VRDPO(type);
}

static inline u16 __imr_luce(u32 type)
{
	return (type & IMR_MAP_LUCE ? IMR_CMR_LUCE : 0);
}

static inline u16 __imr_clce(u32 type)
{
	return (type & IMR_MAP_CLCE ? IMR_CMR_CLCE : 0);
}

/*******************************************************************************
 * Type A (absolute coordinates of source/destination) mapping
 ******************************************************************************/

/* ...return size of the subroutine for type "a" mapping */
static inline u32 imr_tri_type_a_get_length(struct imr_mesh *mesh, int item_size)
{
	return ((mesh->columns * (item_size / 2) + 1) * (mesh->rows - 1) + 1) * sizeof(u32);
}

/* ...set a mesh rows * columns using absolute coordinates */
static inline u32 * imr_tri_set_type_a(u32 *dl, void *map, struct imr_mesh *mesh, int item_size)
{
	int     rows = mesh->rows;
	int     columns = mesh->columns;
	u32     stride = item_size * columns;
	int     i, j;

	/* ...convert lattice into set of stripes */
	for (i = 0; i < rows - 1; i++) {
		*dl++ = IMR_OP_TRI(columns * 2);
		for (j = 0; j < columns; j++) {
			memcpy((void *)dl, map, item_size);
			memcpy((void *)dl + item_size, map + stride, item_size);
			dl += item_size / 2;
			map += item_size;
		}
	}

	*dl++ = IMR_OP_RET;
	return dl;
}

/*******************************************************************************
 * Type B mapping (automatically generated source or destination coordinates)
 ******************************************************************************/

/* ...calculate length of a type "b" mapping */
static inline u32 imr_tri_type_b_get_length(struct imr_mesh *mesh, int item_size)
{
	return ((mesh->columns * (item_size / 2) + 2) * (mesh->rows - 1) + 4) * sizeof(u32);
}

/* ...set an auto-generated mesh n * m for a source/destination */
static inline u32 * imr_tri_set_type_b(u32 *dl, void *map, struct imr_mesh *mesh, int item_size)
{
	int     rows = mesh->rows;
	int     columns = mesh->columns;
	int     x0 = mesh->x0;
	int     y0 = mesh->y0;
	int     dx = mesh->dx;
	int     dy = mesh->dy;
	int     stride = item_size * columns;
	int     i, j;

	/* ...set mesh configuration */
	*dl++ = IMR_OP_WTS(IMR_AMXSR, dx);
	*dl++ = IMR_OP_WTS(IMR_AMYSR, dy);

	/* ...origin by "x" coordinate is the same across all rows */
	*dl++ = IMR_OP_WTS(IMR_AMXOR, x0);

	/* ...convert lattice into set of stripes */
	for (i = 0; i < rows - 1; i++, y0 += dy) {
		/* ...set origin by "y" coordinate for a current row */
		*dl++ = IMR_OP_WTS(IMR_AMYOR, y0);
		*dl++ = IMR_OP_TRI(2 * columns);

		/* ...fill single row */
		for (j = 0; j < columns; j++) {
			memcpy((void *)dl, map, item_size);
			memcpy((void *)dl + item_size, map + stride, item_size);
			dl += item_size / 2;
			map += item_size;
		}
	}

	*dl++ = IMR_OP_RET;
	return dl;
}

/*******************************************************************************
 * Type C mapping (vertex-buffer-object)
 ******************************************************************************/

/* ...calculate length of a type "c" mapping */
static inline u32 imr_tri_type_c_get_length(int num, int item_size)
{
	return ((4 + 3 * item_size) * num + 4);
}

/* ...set a VBO mapping using absolute coordinates */
static inline u32 * imr_tri_set_type_c(u32 *dl, void *map, int num, int item_size)
{
	int     i;

	/* ...prepare list of triangles to draw */
	for (i = 0; i < num; i++) {
		*dl++ = IMR_OP_TRI(3);
		memcpy((void *)dl, map, 3 * item_size);
		dl += 3 * item_size / 4;
		map += 3 * item_size;
	}

	*dl++ = IMR_OP_RET;
	return dl;
}

/*******************************************************************************
 * DL program creation
 ******************************************************************************/

/* ...return length of a DL main program */
static inline u32 imr_dl_program_length(struct imr_ctx *ctx)
{
	u32     iflags = ctx->queue[0].flags;
	u32     oflags = ctx->queue[1].flags;
	u32     cflags = __imr_flags_common(iflags, oflags);

	/* ...check if formats are compatible */
	if (((iflags & IMR_F_PLANAR) != 0 && (oflags & IMR_F_PLANAR) == 0) || (cflags == 0)) {
		v4l2_err(&ctx->imr->v4l2_dev, "formats are incompatible: if=%x, of=%x, cf=%x\n", iflags, oflags, cflags);
		return 0;
	}

	/* ...maximal possible length of the program is 27 32-bits words; round up to 32 */
	return 32 << 2;
}

/* ...setup DL for Y/YUV planar/interleaved processing */
static inline void imr_dl_program_setup(struct imr_ctx *ctx, struct imr_cfg *cfg, u32 type, u32 *dl, u32 subaddr)
{
	u32     iflags = ctx->queue[0].flags;
	u32     oflags = ctx->queue[1].flags;
	u32     cflags = __imr_flags_common(iflags, oflags);
	u16     src_y_fmt = (iflags & IMR_F_Y12 ? IMR_CMR_SY12 : (iflags & IMR_F_Y10 ? IMR_CMR_SY10 : 0));
	u16     src_uv_fmt = (iflags & IMR_F_UV12 ? 2 : (iflags & IMR_F_UV10 ? 1 : 0)) << IMR_CMR_SUV_SHIFT;
	u16     dst_y_fmt = (cflags & IMR_F_Y12 ? IMR_CMR_DY12 : (cflags & IMR_F_Y10 ? IMR_CMR_DY10 : 0));
	u16     dst_uv_fmt = (cflags & IMR_F_UV12 ? 2 : (cflags & IMR_F_UV10 ? 1 : 0)) << IMR_CMR_DUV_SHIFT;
	int     w = ctx->queue[0].fmt.width;
	int     h = ctx->queue[0].fmt.height;
	int     s = ctx->queue[0].fmt.bytesperline;
	int     W = ctx->queue[1].fmt.width;
	int     H = ctx->queue[1].fmt.height;
	int     S = ctx->queue[1].fmt.bytesperline;
	u32     tricr = ctx->color & 0xFFFFFF;
	int     i;

	v4l2_dbg(2, debug, &ctx->imr->v4l2_dev, "setup %u*%u -> %u*%u mapping (type=%x)\n", w, h, W, H, type);

	/* ...set triangle mode register from user-supplied descriptor */
	*dl++ = IMR_OP_WTS(IMR_TRIMCR, 0xFFFF);

	/* ...set automatic source / destination coordinates generation flags */
	*dl++ = IMR_OP_WTS(IMR_TRIMSR, __imr_auto_sg_dg_tcm(type) | __imr_bfe_tme(type));

	/* ...that's probably not needed? - tbd */
	*dl++ = IMR_OP_SYNCM;

	/* ...set source / destination coordinate precision */
	*dl++ = IMR_OP_WTS(IMR_UVDPOR, __imr_uvdp(type));

	/* ...that's probably not needed? - tbd */
	*dl++ = IMR_OP_SYNCM;

	/* ...set luminance/chromacity correction parameters precision */
	*dl++ = IMR_OP_WTS(IMR_CPDPOR, __imr_cpdp(type));

	/* ...reset rendering mode registers */
	*dl++ = IMR_OP_WTS(IMR_CMRCCR, 0xFFFF);
	*dl++ = IMR_OP_WTS(IMR_CMRCCR2, 0xFFFF);

	if (type & IMR_MAP_RSE) {
		/* ...enable RSE */
		*dl++ = IMR_OP_WTL(IMR_RSCCR, 1);
		*dl++ = 0xffffffff;
		*dl++ = IMR_OP_WTL(IMR_RSCSR, 1);
		cfg->rscr_ptr = dl++;

		for (i = 0; i < RSE_PLANES_NUM; i++) {
			/* ...set destination planes base address and strides */
			*dl++ = IMR_OP_WTL(IMR_DSANRR0 + i * 0x10, 4);
			cfg->dstn_pa_ptr[i] = dl++;
			cfg->striden_ptr[i] = dl++;
			cfg->dstr_pa_ptr[i] = dl++;
			cfg->strider_ptr[i] = dl++;
		}

		cfg->rscr_sc8 = cfg->rscr_sc4 = cfg->rscr_sc2 = 0;
		memset(cfg->dstnr_offsets, 0, sizeof(cfg->dstnr_offsets));
		memset(cfg->dstnr_strides, 0, sizeof(cfg->dstnr_strides));
	} else {
		/* ...disable RSE */
		*dl++ = IMR_OP_WTL(IMR_RSCCR, 1);
		*dl++ = 0xffffffff;

		for (i = 0; i < RSE_PLANES_NUM; i++) {
			cfg->dstn_pa_ptr[i] = NULL;
			cfg->striden_ptr[i] = NULL;
			cfg->dstr_pa_ptr[i] = NULL;
			cfg->strider_ptr[i] = NULL;
		}
		cfg->rscr_ptr = NULL;
	}
	/* ...set source/destination addresses of Y/UV plane */
	*dl++ = IMR_OP_WTL(IMR_DSAR, 2);
	cfg->dst_pa_ptr[0] = dl++;
	cfg->src_pa_ptr[0] = dl++;

	/* ...select planar/interleaved mode basing on input format */
	if (iflags & IMR_F_PLANAR) {
		/* ...planar input means planar output; set Y-plane precision */
		if (cflags & IMR_F_Y8) {
			/* ...setup Y-plane processing: YCM=0, SY/DY=xx, SUV/DUV=0 */
			*dl++ = IMR_OP_WTS(IMR_CMRCSR, src_y_fmt | src_uv_fmt | dst_y_fmt | dst_uv_fmt | __imr_luce(type) | __imr_clce(type));

			/* ...set source/destination strides basing on Y-plane precision */
			*dl++ = IMR_OP_WTS(IMR_DSTR, S);
			*dl++ = IMR_OP_WTS(IMR_SSTR, s);
		} else {
			/* ...setup UV-plane processing only */
			*dl++ = IMR_OP_WTS(IMR_CMRCSR, IMR_CMR_YCM | src_uv_fmt | dst_uv_fmt | __imr_clce(type) | __imr_luce(type));

			/* ...set source/destination strides basing on UV-plane precision */
			*dl++ = IMR_OP_WTS(IMR_DSTR, S);
			*dl++ = IMR_OP_WTS(IMR_SSTR, s);
		}
	} else {
		u16		src_fmt = (iflags & IMR_F_UV_SWAP ? IMR_CMR2_UVFORM : 0) | (iflags & IMR_F_YUV_SWAP ? IMR_CMR2_YUV422FORM : 0);

		/* ...interleaved input; output is either interleaved or planar */
		*dl++ = IMR_OP_WTS(IMR_CMRCSR2, IMR_CMR2_YUV422E | src_fmt);

		/* ...destination is always YUYV or UYVY */
		tricr |= (oflags & IMR_F_YUV_SWAP ? IMR_TRIC_YCFORM : 0);

		/* ...set precision of Y/UV planes and required correction */
		*dl++ = IMR_OP_WTS(IMR_CMRCSR, src_y_fmt | src_uv_fmt | dst_y_fmt | dst_uv_fmt | __imr_clce(type) | __imr_luce(type));

		/* ...set source stride basing on precision (2 or 4 bytes/pixel) */
		*dl++ = IMR_OP_WTS(IMR_SSTR, s/* << (iflags & IMR_F_Y10 ? 2 : 1)*/);

		/* ...if output is planar, put the offset value */
		if (oflags & IMR_F_PLANAR) {
			/* ...specify offset of a destination UV plane */
			*dl++ = IMR_OP_WTL(IMR_DSOR, 1);
			*dl++ = S * H;

			/* ...force planar output */
			*dl++ = IMR_OP_WTS(IMR_CMRCSR, IMR_CMR_YISM);

			/* ...destination stride is 1 or 2 bytes/pixel (same for both Y and UV planes) */
			*dl++ = IMR_OP_WTS(IMR_DSTR, S);
		} else {
			/* ...destination stride if 2 or 4 bytes/pixel (Y and UV planes interleaved) */
			*dl++ = IMR_OP_WTS(IMR_DSTR, S);
		}
	}

	/* ...set source width/height of Y/UV plane (for Y plane upper part of SUSR is ignored) */
	*dl++ = IMR_OP_WTL(IMR_SUSR, 2);
	*dl++ = ((w - 2) << 16) | (w - 1);
	*dl++ = h - 1;

	/* ...set triangle single color */
	*dl++ = IMR_OP_WTL(IMR_TRICR, 1);
	*dl++ = tricr;

	/* ...invoke subroutine for triangles drawing */
	*dl++ = IMR_OP_GOSUB;
	*dl++ = subaddr;

	/* ...if we have a planar output with both Y and UV planes available */
	if ((cflags & (IMR_F_PLANAR | IMR_F_Y8 | IMR_F_UV8)) == (IMR_F_PLANAR | IMR_F_Y8 | IMR_F_UV8)) {
		/* ...select UV-plane processing mode; put sync before switching */
		*dl++ = IMR_OP_SYNCM;

		/* ...setup UV-plane source/destination addresses */
		*dl++ = IMR_OP_WTL(IMR_DSAR, 2);
		cfg->dst_pa_ptr[1] = dl++;
		cfg->src_pa_ptr[1] = dl++;

		/* ...select correction mode */
		*dl++ = IMR_OP_WTS(IMR_CMRCSR, IMR_CMR_YCM | __imr_clce(type) | __imr_luce(type));

		/* ...draw triangles */
		*dl++ = IMR_OP_GOSUB;
		*dl++ = subaddr;
	} else {
		/* ...clear pointers to the source/destination UV-planes addresses */
		cfg->src_pa_ptr[1] = cfg->dst_pa_ptr[1] = NULL;
	}

	/* ...signal completion of the operation */
	*dl++ = IMR_OP_SYNCM;
	*dl++ = IMR_OP_TRAP;
}

/*******************************************************************************
 * Mapping specification processing
 ******************************************************************************/

/* ...set mapping data (function called with video device lock held) */
static int imr_ioctl_map(struct imr_ctx *ctx, struct imr_map_desc *desc)
{
	struct imr_device      *imr = ctx->imr;
	struct imr_mesh        *mesh = NULL;
	int                     vbo_num = 0;
	struct imr_cfg         *cfg;
	void                   *buf, *map;
	u32                     type;
	u32                     length, item_size;
	u32                     tri_length;
	void                   *dl_vaddr;
	u32                     dl_size;
	u32                     dl_start_offset;
	dma_addr_t              dl_dma_addr;
	int                     ret;

	/* ...read remainder of data into temporary buffer */
	length = desc->size;
	buf = kmalloc(length, GFP_KERNEL);
	if (!buf) {
		v4l2_err(&imr->v4l2_dev, "failed to allocate %u bytes for mapping reading\n", length);
		return -ENOMEM;
	}

	/* ...copy mesh data */
	if (copy_from_user(buf, (void __user *)desc->data, length)) {
		v4l2_err(&imr->v4l2_dev, "failed to read %u bytes of mapping specification\n", length);
		ret = -EFAULT;
		goto out;
	}

	type = desc->type;

	/* ...check for RSE */
	if ((type & IMR_MAP_RSE) && !imr->rse) {
		v4l2_err(&imr->v4l2_dev, "Rotator & Scaler extension not supported\n");
		return -EINVAL;
	}

	/* ...mesh item size calculation */
	item_size = (type & IMR_MAP_LUCE ? 4 : 0) + (type & IMR_MAP_CLCE ? 4 : 0);

	/* ...calculate the length of a display list */
	if (type & IMR_MAP_MESH) {
		/* ...assure we have proper mesh descriptor */
		if (length < sizeof(struct imr_mesh)) {
			v4l2_err(&imr->v4l2_dev, "invalid mesh specification size: %u\n", length);
			ret = -EINVAL;
			goto out;
		}

		mesh = (struct imr_mesh *)buf;
		length -= sizeof(struct imr_mesh);
		map = buf + sizeof(struct imr_mesh);

		if (type & (IMR_MAP_AUTODG | IMR_MAP_AUTOSG)) {
			/* ...source / destination vertex size is 4 bytes */
			item_size += 4;

			/* ...mapping is given using automatic generation pattern; check size */
			if (mesh->rows * mesh->columns * item_size != length) {
				v4l2_err(&imr->v4l2_dev, "invalid mesh size: %u*%u*%u != %u\n", mesh->rows, mesh->columns, item_size, length);
				ret = -EINVAL;
				goto out;
			}

			/* ...calculate size of triangles drawing subroutine */
			tri_length = imr_tri_type_b_get_length(mesh, item_size);
		} else {
			/* ...source / destination vertes size if 8 bytes */
			item_size += 8;

			/* ...mapping is done with absolute coordinates */
			if (mesh->rows * mesh->columns * item_size != length) {
				v4l2_err(&imr->v4l2_dev, "invalid mesh size: %u*%u*%u != %u\n", mesh->rows, mesh->columns, item_size, length);
				ret = -EINVAL;
				goto out;
			}

			/* ...calculate size of triangles drawing subroutine */
			tri_length = imr_tri_type_a_get_length(mesh, item_size);
		}
	} else {
		/* ...make sure there is no automatic-generation flags */
		if (type & (IMR_MAP_AUTODG | IMR_MAP_AUTOSG)) {
			v4l2_err(&imr->v4l2_dev, "invalid auto-dg/sg flags: 0x%x\n", type);
			ret = -EINVAL;
			goto out;
		}

		map = buf;

		/* ...vertex is given with absolute coordinates */
		item_size += 8;

		/* ...calculate total number of triangles */
		vbo_num = length / (3 * item_size);

		/* ...check the length is sane */
		if (length != vbo_num * 3 * item_size) {
			v4l2_err(&imr->v4l2_dev, "invalid vbo size: %u*%u*3 != %u\n", vbo_num, item_size, length);
			ret = -EINVAL;
			goto out;
		}

		/* ...calculate size of trangles drawing subroutine */
		tri_length = imr_tri_type_c_get_length(vbo_num, item_size);
	}

	/* ...DL main program shall start with 8-byte aligned address */
	dl_start_offset = (tri_length + 7) & ~7;

	/* ...calculate main routine length */
	dl_size = imr_dl_program_length(ctx);
	if (!dl_size) {
		v4l2_err(&imr->v4l2_dev, "format configuration error\n");
		ret = -EINVAL;
		goto out;
	}

	/* ...we use a single display list, with TRI subroutine prepending MAIN */
	dl_size += dl_start_offset;

	/* ...unref current configuration (will not be used by subsequent jobs) */
	imr_cfg_unref(ctx, ctx->cfg);

	/* ...create new configuration */
	cfg = imr_cfg_create(ctx, dl_size, dl_start_offset);
	if (IS_ERR(cfg)) {
		ret = PTR_ERR(cfg);
		ctx->cfg = NULL;
		v4l2_err(&imr->v4l2_dev, "failed to create configuration: %d\n", ret);
		goto out;
	}

	ctx->cfg = cfg;

	/* ...get pointer to the new display list */
	dl_vaddr = cfg->dl_vaddr;
	dl_dma_addr = cfg->dl_dma_addr;

	/* ...prepare a triangles drawing subroutine */
	if (type & IMR_MAP_MESH) {
		if (type & (IMR_MAP_AUTOSG | IMR_MAP_AUTODG)) {
			imr_tri_set_type_b(dl_vaddr, map, mesh, item_size);
		} else {
			imr_tri_set_type_a(dl_vaddr, map, mesh, item_size);
		}
	} else {
		imr_tri_set_type_c(dl_vaddr, map, vbo_num, item_size);
	}

	/* ...prepare main DL-program */
	imr_dl_program_setup(ctx, cfg, type, dl_vaddr + dl_start_offset, (u32)dl_dma_addr);

	/* ...update cropping parameters */
	cfg->dst_subpixel = (type & IMR_MAP_DDP ? 2 : 0);

	/* ...display list updated successfully */
	v4l2_dbg(2, debug, &ctx->imr->v4l2_dev, "display-list created: #%u[%08X]:%u[%u]\n",
		cfg->id, (u32)dl_dma_addr, dl_size, dl_start_offset);

	if (debug >= 4)
		print_hex_dump_bytes("DL-", DUMP_PREFIX_OFFSET, dl_vaddr + dl_start_offset, dl_size - dl_start_offset);

	/* ...success */
	ret = 0;

out:
	/* ...release interim buffer */
	kfree(buf);

	return ret;
}

/* ...set mapping data (function called with video device lock held) */
static int imr_ioctl_map_raw(struct imr_ctx *ctx, struct imr_map_desc *desc)
{
	struct imr_device      *imr = ctx->imr;
	u32                     type = desc->type;
	struct imr_cfg         *cfg;
	void                   *dl_vaddr;
	u32                     dl_size;
	dma_addr_t              dl_dma_addr;

	/* ...check RSE */
	if ((type & IMR_MAP_RSE) && !imr->rse) {
		v4l2_err(&imr->v4l2_dev, "Rotator & Scaler extension not supported\n");
		return -EINVAL;
	}

	/* ...calculate main routine length */
	dl_size = imr_dl_program_length(ctx);
	if (!dl_size) {
		v4l2_err(&imr->v4l2_dev, "format configuration error\n");
		return -EINVAL;
	}

	/* ...unref current configuration (will not be used by subsequent jobs) */
	imr_cfg_unref(ctx, ctx->cfg);

	/* ...create new configuration (starts with zero offset) */
	cfg = imr_cfg_create(ctx, dl_size, 0);
	if (IS_ERR(cfg)) {
		ctx->cfg = NULL;
		v4l2_err(&imr->v4l2_dev, "failed to create configuration: %ld\n", PTR_ERR(cfg));
		return PTR_ERR(cfg);
	}

	ctx->cfg = cfg;

	/* ...get pointer to the new display list */
	dl_vaddr = cfg->dl_vaddr;
	dl_dma_addr = cfg->dl_dma_addr;

	/* ...prepare main DL-program */
	imr_dl_program_setup(ctx, cfg, type, dl_vaddr, (u32)(uintptr_t)desc->data);

	/* ...update cropping parameters */
	cfg->dst_subpixel = (type & IMR_MAP_DDP ? 2 : 0);

	/* ...display list updated successfully */
	v4l2_dbg(2, debug, &ctx->imr->v4l2_dev, "display-list created: #%u[%08X]:%u[%u]\n",
		cfg->id, (u32)dl_dma_addr, dl_size, 0);

	if (debug >= 4)
		print_hex_dump_bytes("DL-", DUMP_PREFIX_OFFSET, dl_vaddr, dl_size);

	/* ...success */
	return 0;
}

/* ...set mapping data (function called with video device lock held) */
static int imr_ioctl_color(struct imr_ctx *ctx, u32 color)
{
	ctx->color = color;

	return 0;
}

static int imr_extdst_set(struct imr_ctx *ctx, u32 *extdst)
{
	struct imr_device      *imr = ctx->imr;
	struct imr_cfg         *cfg = ctx->cfg;

	if (!cfg) {
		v4l2_err(&imr->v4l2_dev, "failed to set V3H extension dst buffers: No active confguration.\n");
		return -EINVAL;
	}

	if (copy_from_user((void *) cfg->dstnr_offsets, (void __user *) extdst, sizeof(cfg->dstnr_offsets))) {
		v4l2_err(&imr->v4l2_dev, "failed to read V3H extension dst buffers\n");
		return -EFAULT;
	}

	return 0;
}

static int imr_extstride_set(struct imr_ctx *ctx, struct imr_rse_param *param)
{
	struct imr_device      *imr = ctx->imr;
	struct imr_cfg         *cfg = ctx->cfg;

	if (!cfg) {
		v4l2_err(&imr->v4l2_dev, "failed to set V3H extension buffers params: No active confguration.\n");
		return -EINVAL;
	}

	cfg->rscr_sc8 = param->sc8;
	cfg->rscr_sc4 = param->sc4;
	cfg->rscr_sc2 = param->sc2;

	if (copy_from_user((void *) cfg->dstnr_strides, (void __user *) param->strides, sizeof(cfg->dstnr_strides))) {
		v4l2_err(&imr->v4l2_dev, "failed to read V3H extension buffers strides\n");
		return -EFAULT;
	}

	return 0;
}

/*******************************************************************************
 * V4L2 I/O controls
 ******************************************************************************/

/* ...check the format stride */
static inline int __imr_verify_fmt_stride(struct v4l2_pix_format *pix)
{
	int     stride;

	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_GREY:
	case V4L2_PIX_FMT_UV8:
		/* ...single byte per pixel */
		stride = pix->width;
		break;

	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_Y10:
	case V4L2_PIX_FMT_Y12:
		/* ...two bytes per pixel */
		stride = pix->width * 2;
		break;

	default:
		/* ...unsupported format */
		return -1;
	}

	if (pix->bytesperline)
		return (pix->bytesperline >= stride ? 0 : -1);

	/* ...no stride is specified; use default one */
	pix->bytesperline = stride;

	return 0;
}

/* ...test for a format supported */
static int __imr_try_fmt(struct imr_ctx *ctx, struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	u32     fourcc = pix->pixelformat;
	int     i;

	/* ...fix-up format stride if needed */
	if (__imr_verify_fmt_stride(pix) < 0)
		return -EINVAL;

	/* ...both output and capture interface have the same set of supported formats */
	for (i = 0; i < ARRAY_SIZE(imr_lx4_formats); i++) {
		if (fourcc == imr_lx4_formats[i].fourcc) {
			/* ...fix-up format specification as needed */
			pix->field = V4L2_FIELD_NONE;

			v4l2_dbg(1, debug, &ctx->imr->v4l2_dev, "%s-format request: '%c%c%c%c', %d*%d\n",
				(V4L2_TYPE_IS_OUTPUT(f->type) ? "output" : "capture"),
				(fourcc >> 0) & 0xff, (fourcc >> 8) & 0xff,
				(fourcc >> 16) & 0xff, (fourcc >> 24) & 0xff,
				pix->width, pix->height);

			/* ...verify source/destination image dimensions */
			if (V4L2_TYPE_IS_OUTPUT(f->type))
				v4l_bound_align_image(&pix->bytesperline, 256, 8192, 8, &pix->height, 1, 2048, 0, 0);
			else
				v4l_bound_align_image(&pix->bytesperline, 64, 8192, 6, &pix->height, 1, 2048, 0, 0);

			/* ...verify width is not exceeding the maximal value */
			(pix->width > 2048 ? pix->width = 2048 : 0);

			return i;
		}
	}

	v4l2_err(&ctx->imr->v4l2_dev, "unsupported format request: '%c%c%c%c'\n",
		(fourcc >> 0) & 0xff, (fourcc >> 8) & 0xff,
		(fourcc >> 16) & 0xff, (fourcc >> 24) & 0xff);

	return -EINVAL;
}

/* ...capabilities query */
static int imr_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strlcpy(cap->driver, DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, DRV_NAME, sizeof(cap->card));
	strlcpy(cap->bus_info, DRV_NAME, sizeof(cap->bus_info));

	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT |
		V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;

	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

/* ...enumerate supported formats */
static int imr_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	/* ...no distinction between output/capture formats */
	if (f->index < ARRAY_SIZE(imr_lx4_formats)) {
		const struct imr_format_info *fmt = &imr_lx4_formats[f->index];
		strlcpy(f->description, fmt->name, sizeof(f->description));
		f->pixelformat = fmt->fourcc;
		return 0;
	}

	return -EINVAL;
}

/* ...retrieve current queue format; operation is locked ? */
static int imr_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct imr_ctx     *ctx = fh_to_ctx(priv);
	struct vb2_queue   *vq;
	struct imr_q_data  *q_data;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	q_data = &ctx->queue[V4L2_TYPE_IS_OUTPUT(f->type) ? 0 : 1];

	/* ...processing is locked? tbd */
	f->fmt.pix = q_data->fmt;

	return 0;
}

/* ...test particular format; operation is not locked */
static int imr_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct imr_ctx     *ctx = fh_to_ctx(priv);
	struct vb2_queue   *vq;

	/* ...make sure we have a queue of particular type */
	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	/* ...test if format is supported (adjust as appropriate) */
	return (__imr_try_fmt(ctx, f) >= 0 ? 0 : -EINVAL);
}

/* ...apply queue format; operation is locked ? */
static int imr_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct imr_ctx     *ctx = fh_to_ctx(priv);
	struct vb2_queue   *vq;
	struct imr_q_data  *q_data;
	int                 i;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	/* ...check if queue is busy */
	if (vb2_is_busy(vq))
		return -EBUSY;

	/* ...test if format is supported (adjust as appropriate) */
	i = __imr_try_fmt(ctx, f);
	if (i < 0)
		return -EINVAL;

	/* ...format is supported; save current format in a queue-specific data */
	q_data = &ctx->queue[V4L2_TYPE_IS_OUTPUT(f->type) ? 0 : 1];

	/* ...processing is locked? tbd */
	q_data->fmt = f->fmt.pix;
	q_data->flags = imr_lx4_formats[i].flags;

	/* ...set default crop factors */
	if (V4L2_TYPE_IS_OUTPUT(f->type) == 0) {
		ctx->crop[0] = 0;
		ctx->crop[1] = f->fmt.pix.width - 1;
		ctx->crop[2] = 0;
		ctx->crop[3] = f->fmt.pix.height - 1;
	}

	return 0;
}

static int imr_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *reqbufs)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int imr_querybuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int imr_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);

	/* ...operation is protected with a queue lock */
	WARN_ON(!mutex_is_locked(&ctx->imr->mutex));

	/* ...verify the configuration is complete */
	if (!ctx->cfg) {
		v4l2_err(&ctx->imr->v4l2_dev, "stream configuration is not complete\n");
		return -EINVAL;
	}

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int imr_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int imr_expbuf(struct file *file, void *priv, struct v4l2_exportbuffer *eb)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);

	return v4l2_m2m_expbuf(file, ctx->m2m_ctx, eb);
}

static int imr_streamon(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct imr_ctx         *ctx = fh_to_ctx(priv);

	/* ...context is prepared for a streaming */
	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int imr_streamoff(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct imr_ctx  *ctx = fh_to_ctx(priv);

	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int imr_g_crop(struct file *file, void *priv, struct v4l2_crop *cr)
{
	struct imr_ctx  *ctx = fh_to_ctx(priv);

	/* ...subpixel resolution of output buffer is not counted here */
	cr->c.left = ctx->crop[0];
	cr->c.top = ctx->crop[2];
	cr->c.width = ctx->crop[1] - ctx->crop[0];
	cr->c.height = ctx->crop[3] - ctx->crop[2];

	return 0;
}

static int imr_s_crop(struct file *file, void *priv, const struct v4l2_crop *cr)
{
	struct imr_ctx *ctx = fh_to_ctx(priv);
	int             x0 = cr->c.left;
	int             y0 = cr->c.top;
	int             x1 = x0 + cr->c.width;
	int             y1 = y0 + cr->c.height;

	if (x0 < 0 || x1 >= 2048 || y0 < 0 || y1 >= 2048) {
		v4l2_err(&ctx->imr->v4l2_dev, "invalid cropping: %d/%d/%d/%d\n", x0, x1, y0, y1);
		return -EINVAL;
	}

	/* ...subpixel resolution of output buffer is not counted here */
	ctx->crop[0] = x0;
	ctx->crop[1] = x1;
	ctx->crop[2] = y0;
	ctx->crop[3] = y1;

	return 0;
}

/* ...customized I/O control processing */
static long imr_default(struct file *file, void *fh, bool valid_prio, unsigned int cmd, void *arg)
{
	struct imr_ctx     *ctx = fh_to_ctx(fh);

	switch (cmd) {
	case VIDIOC_IMR_MESH:
		/* ...set mesh data */
		return imr_ioctl_map(ctx, (struct imr_map_desc *)arg);

	case VIDIOC_IMR_MESH_RAW:
		/* ...set mesh data */
		return imr_ioctl_map_raw(ctx, (struct imr_map_desc *)arg);

	case VIDIOC_IMR_COLOR:
		/* ...set solid color code */
		return imr_ioctl_color(ctx, *(u32 *)arg);

	case VIDIOC_IMR_EXTDST:
		/* ...set V3H extension dst buffers */
		return imr_extdst_set(ctx, *(u32 **)arg);

	case VIDIOC_IMR_EXTSTRIDE:
		/* ...set V3H extension dst strides */
		return imr_extstride_set(ctx, (struct imr_rse_param *)arg);

	default:
		return -ENOIOCTLCMD;
	}
}

static const struct v4l2_ioctl_ops imr_ioctl_ops = {
	.vidioc_querycap            = imr_querycap,

	.vidioc_enum_fmt_vid_cap    = imr_enum_fmt,
	.vidioc_enum_fmt_vid_out    = imr_enum_fmt,
	.vidioc_g_fmt_vid_cap       = imr_g_fmt,
	.vidioc_g_fmt_vid_out       = imr_g_fmt,
	.vidioc_try_fmt_vid_cap     = imr_try_fmt,
	.vidioc_try_fmt_vid_out     = imr_try_fmt,
	.vidioc_s_fmt_vid_cap       = imr_s_fmt,
	.vidioc_s_fmt_vid_out       = imr_s_fmt,

	.vidioc_reqbufs             = imr_reqbufs,
	.vidioc_querybuf            = imr_querybuf,
	.vidioc_qbuf                = imr_qbuf,
	.vidioc_dqbuf               = imr_dqbuf,
	.vidioc_expbuf              = imr_expbuf,
	.vidioc_streamon            = imr_streamon,
	.vidioc_streamoff           = imr_streamoff,

	.vidioc_g_crop              = imr_g_crop,
	.vidioc_s_crop              = imr_s_crop,

	.vidioc_default             = imr_default,
};

/*******************************************************************************
 * Generic device file operations
 ******************************************************************************/

static int imr_open(struct file *file)
{
	struct imr_device      *imr = video_drvdata(file);
	struct video_device    *vfd = video_devdata(file);
	struct imr_ctx         *ctx;
	int                     ret;

	/* ...allocate processing context associated with given instance */
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	/* ...initialize per-file-handle structure */
	v4l2_fh_init(&ctx->fh, vfd);
	//ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	/* ...set default source / destination formats - need that? */
	ctx->imr = imr;
	ctx->queue[0].fmt.pixelformat = 0;
	ctx->queue[1].fmt.pixelformat = 0;

	/* ...set default cropping parameters */
	ctx->crop[1] = ctx->crop[3] = 0x3FF;

	/* ...set default color */
	ctx->color = 0x808080;

	/* ...initialize M2M processing context */
	ctx->m2m_ctx = v4l2_m2m_ctx_init(imr->m2m_dev, ctx, imr_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = PTR_ERR(ctx->m2m_ctx);
		goto v4l_prepare_rollback;
	}

#if 0
	/* ...initialize controls and stuff */
	ret = imr_controls_create(ctx);
	if (ret < 0)
		goto v4l_prepare_rollback;
#endif

	/* ...lock access to global device data */
	if (mutex_lock_interruptible(&imr->mutex)) {
		ret = -ERESTARTSYS;
		goto v4l_prepare_rollback;
	}

	/* ...bring-up device as needed */
	if (imr->refcount == 0) {
		ret = clk_prepare_enable(imr->clock);
		if (ret < 0)
			goto device_prepare_rollback;
	}

	imr->refcount++;

	mutex_unlock(&imr->mutex);

	v4l2_dbg(1, debug, &imr->v4l2_dev, "IMR device opened (refcount=%u)\n", imr->refcount);

	return 0;

device_prepare_rollback:
	/* ...unlock global device data */
	mutex_unlock(&imr->mutex);

v4l_prepare_rollback:
	/* ...destroy context */
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	return ret;
}

static int imr_release(struct file *file)
{
	struct imr_device  *imr = video_drvdata(file);
	struct imr_ctx     *ctx = fh_to_ctx(file->private_data);

	/* ...I don't need to get a device-scope lock here really - tbd */
	mutex_lock(&imr->mutex);

	/* ...destroy M2M device processing context */
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	//v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);

	/* ...drop active configuration as needed */
	imr_cfg_unref(ctx, ctx->cfg);

	/* ...make sure there are no more active configs */
	WARN_ON(ctx->cfg_num);

	/* ...destroy context data */
	kfree(ctx);

	/* ...disable hardware operation */
	if (--imr->refcount == 0)
		clk_disable_unprepare(imr->clock);

	mutex_unlock(&imr->mutex);

	v4l2_dbg(1, debug, &imr->v4l2_dev, "closed device instance\n");

	return 0;
}

static unsigned int imr_poll(struct file *file, struct poll_table_struct *wait)
{
	struct imr_device  *imr = video_drvdata(file);
	struct imr_ctx     *ctx = fh_to_ctx(file->private_data);
	unsigned int        res;

	if (mutex_lock_interruptible(&imr->mutex))
		return -ERESTARTSYS;

	res = v4l2_m2m_poll(file, ctx->m2m_ctx, wait);

	mutex_unlock(&imr->mutex);

	v4l2_dbg(3, debug, &imr->v4l2_dev, "poll result: %X (ctx=%p)\n", res, ctx);

	return res;
}

static int imr_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct imr_device  *imr = video_drvdata(file);
	struct imr_ctx     *ctx = fh_to_ctx(file->private_data);
	int                 ret;

	/* ...should we protect all M2M operations with mutex? - tbd */
	if (mutex_lock_interruptible(&imr->mutex))
		return -ERESTARTSYS;

	ret = v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);

	mutex_unlock(&imr->mutex);

	return ret;
}

static const struct v4l2_file_operations imr_fops = {
	.owner          = THIS_MODULE,
	.open           = imr_open,
	.release        = imr_release,
	.poll           = imr_poll,
	.mmap           = imr_mmap,
	.unlocked_ioctl	= video_ioctl2,
};

/*******************************************************************************
 * M2M device interface
 ******************************************************************************/

/* ...job cleanup function */
#if 0
static void imr_cleanup(struct imr_ctx *ctx)
{
	struct imr_device      *imr = ctx->imr;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	unsigned long           flags;

	/* ...interlock buffer handling with interrupt */
	spin_lock_irqsave(&imr->lock, flags);

	while ((src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx)) != NULL)
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);

	while ((dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx)) != NULL)
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);

	/* ...release lock before we mark current job as finished */
	spin_unlock_irqrestore(&imr->lock, flags);
}
#endif

/* ...job execution function */
static void imr_device_run(void *priv)
{
	struct imr_ctx         *ctx = priv;
	struct imr_device      *imr = ctx->imr;
	struct imr_cfg         *cfg;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	u32                     src_addr, dst_addr;
	unsigned long           flags;
	int			i;

	v4l2_dbg(3, debug, &imr->v4l2_dev, "run next job...\n");

	/* ...protect access to internal device state */
	spin_lock_irqsave(&imr->lock, flags);

	/* ...retrieve input/output buffers */
	src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	dst_buf = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);

	/* ...put source/destination buffers sequence numbers */
	dst_buf->sequence = src_buf->sequence = ctx->sequence++;

	/* ...take configuration pointer associated with input buffer */
	cfg = to_imr_buffer(src_buf)->cfg;

	/* ...cancel software reset state as needed */
	iowrite32(0, imr->mmio + IMR_CR);

	/* ...set cropping data with respect to destination sub-pixel mode */
	iowrite32(ctx->crop[0] << cfg->dst_subpixel, imr->mmio + IMR_XMINR);
	iowrite32(ctx->crop[1] << cfg->dst_subpixel, imr->mmio + IMR_XMAXR);
	iowrite32(ctx->crop[2] << cfg->dst_subpixel, imr->mmio + IMR_YMINR);
	iowrite32(ctx->crop[3] << cfg->dst_subpixel, imr->mmio + IMR_YMAXR);

	/* ...adjust source/destination parameters of the program (interleaved / semiplanar) */
	*cfg->src_pa_ptr[0] = src_addr = (u32)vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
	*cfg->dst_pa_ptr[0] = dst_addr = (u32)vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);

	for (i = 0; i < RSE_PLANES_NUM; i++) {
		if (cfg->rscr_ptr) *cfg->rscr_ptr = (1 << IMR_RSCR_RSE) | (cfg->rscr_sc8 << IMR_RSCR_SC8) |
						    (cfg->rscr_sc4 << IMR_RSCR_SC4) |(cfg->rscr_sc2 << IMR_RSCR_SC2);

		if (cfg->dstn_pa_ptr[i]) *cfg->dstn_pa_ptr[i] = dst_addr + cfg->dstnr_offsets[i];
		if (cfg->dstr_pa_ptr[i]) *cfg->dstr_pa_ptr[i] = dst_addr + cfg->dstnr_offsets[i + RSE_PLANES_NUM];

		if (cfg->striden_ptr[i]) *cfg->striden_ptr[i] = cfg->dstnr_strides[i];
		if (cfg->strider_ptr[i]) *cfg->strider_ptr[i] = cfg->dstnr_strides[i + RSE_PLANES_NUM];
	}

	/* ...adjust source/destination parameters of the UV-plane as needed */
	if (cfg->src_pa_ptr[1] && cfg->dst_pa_ptr[1]) {
		*cfg->src_pa_ptr[1] = src_addr + ctx->queue[0].fmt.bytesperline * ctx->queue[0].fmt.height;
		*cfg->dst_pa_ptr[1] = dst_addr + ctx->queue[1].fmt.bytesperline * ctx->queue[1].fmt.height;
	}

	v4l2_dbg(3, debug, &imr->v4l2_dev, "process buffer-pair 0x%08x:0x%08x\n",
		*cfg->src_pa_ptr[0], *cfg->dst_pa_ptr[0]);

	/* ...force clearing of status register bits */
	iowrite32(0x7, imr->mmio + IMR_SRCR);

	/* ...unmask/enable interrupts */
	iowrite32(ioread32(imr->mmio + IMR_ICR) | (IMR_ICR_TRAEN | IMR_ICR_IEREN | IMR_ICR_INTEN), imr->mmio + IMR_ICR);
	iowrite32(ioread32(imr->mmio + IMR_IMR) & ~(IMR_ICR_TRAEN | IMR_ICR_IEREN | IMR_ICR_INTEN), imr->mmio + IMR_IMR);

	/* ...set display list address */
	iowrite32(cfg->dl_dma_addr + cfg->dl_start_offset, imr->mmio + IMR_DLSAR);

	/* ...enable texture prefetching */
	iowrite32(0xACCE5501, imr->mmio + IMR_TPOR);

	/* ...explicitly flush any pending write operations (don't need that, I guess) */
	wmb();

	/* ...start rendering operation */
	iowrite32(IMR_CR_RS, imr->mmio + IMR_CR);

	/* ...timestamp input buffer */
	src_buf->vb2_buf.timestamp = ktime_get_ns();

	/* ...unlock device access */
	spin_unlock_irqrestore(&imr->lock, flags);

	v4l2_dbg(1, debug, &imr->v4l2_dev, "rendering started: status=%X, DLSAR=0x%08X, DLPR=0x%08X\n", ioread32(imr->mmio + IMR_SR), ioread32(imr->mmio + IMR_DLSAR), ioread32(imr->mmio + IMR_DLSR));
}

/* ...check whether a job is ready for execution */
static int imr_job_ready(void *priv)
{
	/* ...no specific requirements on the job readiness */
	return 1;
}

/* ...abort currently processed job */
static void imr_job_abort(void *priv)
{
	struct imr_ctx         *ctx = priv;
	struct imr_device      *imr = ctx->imr;
	unsigned long           flags;

	/* ...protect access to internal device state */
	spin_lock_irqsave(&imr->lock, flags);

	/* ...make sure current job is still current (may get finished by interrupt already) */
	if (v4l2_m2m_get_curr_priv(imr->m2m_dev) == ctx) {
		v4l2_dbg(1, debug, &imr->v4l2_dev, "abort job: status=%X, DLSAR=0x%08X, DLPR=0x%08X\n",
			ioread32(imr->mmio + IMR_SR), ioread32(imr->mmio + IMR_DLSAR), ioread32(imr->mmio + IMR_DLSR));

		/* ...force device reset to stop processing of the buffers */
		//iowrite32(IMR_CR_SWRST, imr->mmio + IMR_CR);

		/* ...resetting the module while operation is active may lead to hw-stall */
		spin_unlock_irqrestore(&imr->lock, flags);

		/* ...finish current job as interrupt will probably not occur */
		//v4l2_m2m_job_finish(imr->m2m_dev, ctx->m2m_ctx);
	} else {
		spin_unlock_irqrestore(&imr->lock, flags);
		v4l2_dbg(1, debug, &imr->v4l2_dev, "job has completed already\n");
	}
}

/* ...M2M interface definition */
static struct v4l2_m2m_ops imr_m2m_ops = {
	.device_run	= imr_device_run,
	.job_ready	= imr_job_ready,
	.job_abort	= imr_job_abort,
};

/*******************************************************************************
 * Interrupt handling
 ******************************************************************************/

static irqreturn_t imr_irq_handler(int irq, void *data)
{
	struct imr_device      *imr = data;
	struct imr_ctx         *ctx;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	u32                     status;
	irqreturn_t             ret = IRQ_NONE;

	/* ...check and ack interrupt status */
	status = ioread32(imr->mmio + IMR_SR);
	iowrite32(status, imr->mmio + IMR_SRCR);
	if (!(status & (IMR_SR_INT | IMR_SR_IER | IMR_SR_TRA))) {
		v4l2_err(&imr->v4l2_dev, "spurious interrupt: %x\n", status);
		return ret;
	}

	/* ...protect access to current context */
	spin_lock(&imr->lock);

	/* ...get current job context (may have been cancelled already) */
	ctx = v4l2_m2m_get_curr_priv(imr->m2m_dev);
	if (!ctx) {
		v4l2_dbg(3, debug, &imr->v4l2_dev, "no active job\n");
		goto handled;
	}

	/* ...remove buffers (may have been removed already?) */
	src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	if (!src_buf || !dst_buf) {
		v4l2_dbg(3, debug, &imr->v4l2_dev, "no buffers associated with current context\n");
		goto handled;
	}

	/* ...check for a TRAP interrupt indicating completion of current DL */
	if (status & IMR_SR_TRA) {
		/* ...operation completed normally; timestamp output buffer */
		dst_buf->vb2_buf.timestamp = ktime_get_ns();
		if (src_buf->flags & V4L2_BUF_FLAG_TIMECODE)
			dst_buf->timecode = src_buf->timecode;
		dst_buf->flags = src_buf->flags & (V4L2_BUF_FLAG_TIMECODE | V4L2_BUF_FLAG_KEYFRAME |
			 V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME | V4L2_BUF_FLAG_TSTAMP_SRC_MASK);
		//dst_buf->sequence = src_buf->sequence = ctx->sequence++;

		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);

		v4l2_dbg(3, debug, &imr->v4l2_dev, "buffers <0x%08x,0x%08x> done (ctx=%p)\n",
			(u32)vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0),
			(u32)vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0), ctx);
	} else {
		/* ...operation completed in error; no way to understand what exactly went wrong */
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);

		v4l2_dbg(3, debug, &imr->v4l2_dev, "buffers <0x%08x,0x%08x> done in error\n",
			(u32)vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0),
			(u32)vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0));
	}

	spin_unlock(&imr->lock);

	/* ...finish current job (and start any pending) */
	v4l2_m2m_job_finish(imr->m2m_dev, ctx->m2m_ctx);

	return IRQ_HANDLED;

handled:
	/* ...again, what exactly is to be protected? */
	spin_unlock(&imr->lock);

	return IRQ_HANDLED;
}

/*******************************************************************************
 * Device probing / removal interface
 ******************************************************************************/

static struct class *imr_alloc_class;

static int imr_probe(struct platform_device *pdev)
{
	struct imr_device *imr;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	int ret;
	const phandle *prop;
	struct device_node *node;
	struct device *adev;

	imr = devm_kzalloc(&pdev->dev, sizeof(*imr), GFP_KERNEL);
	if (!imr)
		return -ENOMEM;

	mutex_init(&imr->mutex);
	spin_lock_init(&imr->lock);
	imr->dev = &pdev->dev;

	/* Check RSE support */
	imr->rse = of_property_read_bool(np, "rse");

	/* ...memory-mapped registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot get memory region\n");
		return -EINVAL;
	}

	imr->mmio = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(imr->mmio))
		return PTR_ERR(imr->mmio);

	/* ...interrupt service routine registration */
	imr->irq = ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot find IRQ\n");
		return ret;
	}

	ret = devm_request_irq(&pdev->dev, imr->irq, imr_irq_handler, 0, dev_name(&pdev->dev), imr);
	if (ret) {
		dev_err(&pdev->dev, "cannot claim IRQ %d\n", imr->irq);
		return ret;
	}

	imr->clock = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(imr->clock)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return PTR_ERR(imr->clock);
	}

	/* ...create v4l2 device */
	ret = v4l2_device_register(&pdev->dev, &imr->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register v4l2 device\n");
		return ret;
	}

	/* ...create mem2mem device handle */
	imr->m2m_dev = v4l2_m2m_init(&imr_m2m_ops);
	if (IS_ERR(imr->m2m_dev)) {
		v4l2_err(&imr->v4l2_dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(imr->m2m_dev);
		goto device_register_rollback;
	}

	if (!imr_alloc_class) {
		imr_alloc_class = class_create(THIS_MODULE, "imr-alloc");
		if (IS_ERR(imr_alloc_class)) {
			v4l2_err(&imr->v4l2_dev, "Failed to create alloc-device class\n");
			ret = PTR_ERR(imr_alloc_class);
			goto m2m_init_rollback;
		}
	}

	adev = device_create(imr_alloc_class, imr->dev, MKDEV(0, 0), NULL,
			     "%s_alloc", dev_name(&pdev->dev));
	if (IS_ERR(adev)) {
		v4l2_err(&imr->v4l2_dev, "Failed to create alloc-device\n");
		ret = PTR_ERR(adev);
		goto m2m_init_rollback;
	}

	adev->dma_mask = &adev->coherent_dma_mask;
	adev->coherent_dma_mask = DMA_BIT_MASK(32);
	imr->alloc_dev = adev;
	prop = of_get_property(np, "alloc-dev", NULL);
	if (prop) {
		node = of_find_node_by_phandle(be32_to_cpup(prop));
		of_dma_configure(adev, node, true);
	}

	strlcpy(imr->video_dev.name, dev_name(&pdev->dev), sizeof(imr->video_dev.name));
	imr->video_dev.fops         = &imr_fops;
	imr->video_dev.ioctl_ops    = &imr_ioctl_ops;
	imr->video_dev.minor        = -1;
	imr->video_dev.release      = video_device_release_empty;
	imr->video_dev.lock         = &imr->mutex;
	imr->video_dev.v4l2_dev     = &imr->v4l2_dev;
	imr->video_dev.vfl_dir      = VFL_DIR_M2M;

	ret = video_register_device(&imr->video_dev, VFL_TYPE_GRABBER, -1);
	if (ret) {
		v4l2_err(&imr->v4l2_dev, "Failed to register video device\n");
		goto m2m_init_rollback;
	}

	video_set_drvdata(&imr->video_dev, imr);
	platform_set_drvdata(pdev, imr);
	//pm_runtime_enable(&pdev->dev);

	v4l2_info(&imr->v4l2_dev, "IMR device (pdev: %d) registered as /dev/video%d\n", pdev->id, imr->video_dev.num);

	return 0;

m2m_init_rollback:
	v4l2_m2m_release(imr->m2m_dev);

device_register_rollback:
	v4l2_device_unregister(&imr->v4l2_dev);

	return ret;
}

static int imr_remove(struct platform_device *pdev)
{
	struct imr_device *imr = platform_get_drvdata(pdev);

	//pm_runtime_disable(imr->v4l2_dev.dev);
	video_unregister_device(&imr->video_dev);
	v4l2_m2m_release(imr->m2m_dev);
	v4l2_device_unregister(&imr->v4l2_dev);

	return 0;
}

/*******************************************************************************
 * Power management
 ******************************************************************************/

#ifdef CONFIG_PM_SLEEP

/* ...device suspend hook; clock control only - tbd */
static int imr_pm_suspend(struct device *dev)
{
	struct imr_device *imr = dev_get_drvdata(dev);

	WARN_ON(mutex_is_locked(&imr->mutex));

	if (imr->refcount == 0)
		return 0;

	clk_disable_unprepare(imr->clock);

	return 0;
}

/* ...device resume hook; clock control only */
static int imr_pm_resume(struct device *dev)
{
	struct imr_device *imr = dev_get_drvdata(dev);

	WARN_ON(mutex_is_locked(&imr->mutex));

	if (imr->refcount == 0)
		return 0;

	clk_prepare_enable(imr->clock);

	return 0;
}

#endif  /* CONFIG_PM_SLEEP */

/* ...power management callbacks */
static const struct dev_pm_ops imr_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(imr_pm_suspend, imr_pm_resume)
};

/* ...device table */
static const struct of_device_id imr_of_match[] = {
	{ .compatible = "renesas,imr-lx4" },
	{ },
};

/* ...platform driver interface */
static struct platform_driver imr_platform_driver = {
	.probe		= imr_probe,
	.remove		= imr_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "imr",
		.pm	= &imr_pm_ops,
		.of_match_table = imr_of_match,
	},
};

static int __init imr_module_init(void)
{
	return platform_driver_register(&imr_platform_driver);
}

static int imr_device_destroy(struct device *dev, void *data)
{
	device_destroy(imr_alloc_class, dev->devt);
	return 0;
}

static void __exit imr_module_exit(void)
{
	class_for_each_device(imr_alloc_class, NULL, NULL, imr_device_destroy);
	class_destroy(imr_alloc_class);
	platform_driver_unregister(&imr_platform_driver);
}

module_init(imr_module_init);
module_exit(imr_module_exit);

MODULE_ALIAS("imr");
MODULE_AUTHOR("Cogent Embedded Inc. <sources@cogentembedded.com>");
MODULE_DESCRIPTION("Renesas IMR-LX4 Driver");
MODULE_LICENSE("GPL");
