#include <re.h>
#include <rem_vid.h>
#include <rem_vidconv.h>
#include "vconv.h"


static const char *vidfmt_name(enum vidfmt fmt)
{
	switch (fmt) {

	case VID_FMT_NONE:    return "(NONE)";
	case VID_FMT_YUV420P: return "YUV420P";
	case VID_FMT_UYVY422: return "UYVY422";
	case VID_FMT_YUYV422: return "YUYV422";
	case VID_FMT_RGB32:   return "RGB32";
	case VID_FMT_ARGB:    return "ARGB";
	default:              return "???";
	}
}


/**
 * Convert from packed YUYV422 to planar YUV420P
 */
static void yuyv_to_yuv420p(struct vidframe *dst, const struct vidframe *src)
{
	const uint32_t *p1 = (uint32_t *)src->data[0];
	const uint32_t *p2 = (uint32_t *)(src->data[0] + src->linesize[0]);
	uint16_t *y  = (uint16_t *)dst->data[0];
	uint16_t *y2 = (uint16_t *)(dst->data[0] + dst->linesize[0]);
	uint8_t *u = dst->data[1], *v = dst->data[2];
	int h, w;

	/* todo: fix endianness assumptions */

	/* 2 lines */
	for (h=0; h<dst->size.h/2; h++) {

		for (w=0; w<dst->size.w/2; w++) {

			/* Y1 */
			y[w]   =   p1[w] & 0xff;
			y[w]  |= ((p1[w] >> 16) & 0xff) << 8;

			/* Y2 */
			y2[w]   =   p2[w] & 0xff;
			y2[w]  |= ((p2[w] >> 16) & 0xff) << 8;

			/* U and V */
			u[w] = (p1[w] >>  8) & 0xff;
			v[w] = (p1[w] >> 24) & 0xff;
		}

		p1 += src->linesize[0] / 2;
		p2 += src->linesize[0] / 2;

		y  += dst->linesize[0];
		y2 += dst->linesize[0];
		u  += dst->linesize[1];
		v  += dst->linesize[2];
	}
}


/**
 * Convert from RGB32 to planar YUV420P
 */
static void rgb32_to_yuv420p(struct vidframe *dst, const struct vidframe *src)
{
	const uint8_t *p1 = src->data[0];
	const uint8_t *p2 = src->data[0] + src->linesize[0];
	uint16_t *y  = (uint16_t *)dst->data[0];
	uint16_t *y2 = (uint16_t *)(dst->data[0] + dst->linesize[0]);
	uint8_t *u = dst->data[1], *v = dst->data[2];
	int h, w, j;

	/* 2 lines */
	for (h = 0; h < dst->size.h/2; h++) {

		/* 2 pixels */
		for (w = 0; w < dst->size.w/2; w++) {

			j = w * 8;

			y [w]  = rgb2y(p1[j+2], p1[j+1], p1[j+0]) << 0;
			y [w] |= rgb2y(p1[j+6], p1[j+5], p1[j+4]) << 8;

			y2[w]  = rgb2y(p2[j+2], p2[j+1], p2[j+0]) << 0;
			y2[w] |= rgb2y(p2[j+6], p2[j+5], p2[j+4]) << 8;

			u[w] = rgb2u(p1[j+2], p1[j+1], p1[j+0]);
			v[w] = rgb2v(p1[j+2], p1[j+1], p1[j+0]);
		}

		p1 += src->linesize[0] * 2;
		p2 += src->linesize[0] * 2;

		y  += dst->linesize[0];
		y2 += dst->linesize[0];
		u  += dst->linesize[1];
		v  += dst->linesize[2];
	}
}


void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     int rotate, bool hflip, bool vflip)
{
	if (!dst || !src)
		return;

	/* unused for now */
	(void)rotate;
	(void)hflip;
	(void)vflip;

	re_printf("vidconv: %s:%dx%d ---> %s:%dx%d\n",
		  vidfmt_name(src->fmt), src->size.w, src->size.h,
		  vidfmt_name(dst->fmt), dst->size.w, dst->size.h);

	if (src->fmt == dst->fmt) {

		int i;

		for (i=0; i<4; i++) {
			dst->data[i]     = src->data[i];
			dst->linesize[i] = src->linesize[i];
		}
	}
	else if (src->fmt == VID_FMT_RGB32 && dst->fmt == VID_FMT_YUV420P)
		rgb32_to_yuv420p(dst, src);
	else if (src->fmt == VID_FMT_YUYV422 && dst->fmt == VID_FMT_YUV420P)
		yuyv_to_yuv420p(dst, src);
	else {
#ifdef USE_FFMPEG
		printf("vidconv: using swscale\n");
		vidconv_sws(dst, src);
#else
		printf("unsupported pixelformat\n");
#endif
	}
}