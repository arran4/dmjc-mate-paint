/***************************************************************************
    Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
    Contributed by Juan Balderas

    This file is part of mate-paint.
****************************************************************************/


#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include "pixbuf_util.h"

struct fillinfo
{
   unsigned char *rgb; 
   int rowstride;
   int pixelsize; 
   int width;
   int height;
   unsigned char or, og, ob, oa;
   unsigned char r, g, b, a;
   int gx, gw, gy, gh;
};

//static gint gx, gw, gy, gh;

static void setpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y, guint color);
static gint getpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y);
// static gint fill_shape(GdkPixbuf *pixbuf, guint x, guint y, guint nc);

static void
flood_fill_algo(struct fillinfo *info, int x, int y);


GdkRectangle fill_draw(GdkPixbuf *pixbuf, guint fill_color, guint x, guint y)
{
	struct fillinfo fillinfo;
	guchar *p;
	GdkRectangle rect = {0, 0, 0, 0};
	
	if (!pixbuf) return rect;

	fillinfo.gx = x;
	fillinfo.gw = x;
	fillinfo.gy = y;
	fillinfo.gh = y;

	fillinfo.rgb = gdk_pixbuf_get_pixels (pixbuf);
    fillinfo.width = gdk_pixbuf_get_width (pixbuf);
    fillinfo.height = gdk_pixbuf_get_height (pixbuf);
    fillinfo.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    fillinfo.pixelsize = gdk_pixbuf_get_n_channels (pixbuf);
    fillinfo.r = getr(fill_color);
    fillinfo.g = getg(fill_color);
    fillinfo.b = getb(fill_color);
    fillinfo.a = geta(fill_color);

    if (x < 0 || x >= fillinfo.width || y < 0 || y >= fillinfo.height) return rect;

    p = fillinfo.rgb + y * fillinfo.rowstride + x * fillinfo.pixelsize;
    fillinfo.or = *p;
    fillinfo.og = *(p + 1);
    fillinfo.ob = *(p + 2);
    if (fillinfo.pixelsize >= 4)
        fillinfo.oa = *(p + 3);
    else
        fillinfo.oa = 255;
    
    flood_fill_algo(&fillinfo, x, y);

    fillinfo.gw = ABS(fillinfo.gw - fillinfo.gx);
	fillinfo.gh = ABS(fillinfo.gh - fillinfo.gy);
	
	rect.x = fillinfo.gx; rect.y = fillinfo.gy;
	rect.width = fillinfo.gw + 1; rect.height = fillinfo.gh + 1;
	
	return rect;
}

/* Get a pixel's value at (x,y)
 * For pixbufs with alpha channel only.
 * Sets 'color' as rgb value
 */
gboolean get_pixel_from_pixbuf(GdkPixbuf *pixbuf, guint *color, guint x, guint y)
{
	int width, height, rowstride, n_channels;
	guchar *pixels;

	if(!GDK_IS_PIXBUF(pixbuf)){
		printf("get_pixel_from_pixbuf: !pixbuf\n");
		return 0;
	}

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	
	if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB){
		printf("get_pixel_from_pixbuf: color space  != GDK_COLORSPACE_RGB\n");
		return FALSE;
	}
	if (gdk_pixbuf_get_bits_per_sample (pixbuf) != 8){
		printf("get_pixel_from_pixbuf: bits per sample != 8\n");
		return FALSE;
	}
	// if (!gdk_pixbuf_get_has_alpha (pixbuf)){ ... } // Allow no alpha?
	if (NULL == color){
		printf("get_pixel_from_pixbuf: NULL == color\n");
		return FALSE;
	}

	*color = getpixel(pixels, rowstride, n_channels, x, y);
	// printf("get_pixel_from_pixbuf color: 0x%.08x %s %d\n", *color, __FILE__, __LINE__);

	return TRUE;
}


void set_pixel_in_pixbuf(GdkPixbuf *pixbuf, guint color, guint x, guint y)
{
	int width, height, rowstride, n_channels;
	guchar *pixels;

	if(!GDK_IS_PIXBUF(pixbuf)){
		printf("get_pixel_from_pixbuf: !pixbuf\n");
		return ;
	}

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	
	setpixel(pixels, rowstride, n_channels, x, y, color);
}


static void
setpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y, guint color)
{
	guchar *p;
	
	p = pixels + (y * rowstride + x * n_channels);
	p[0] = getr(color);/*red*/
	p[1] = getg(color);/*green*/
	p[2] = getb(color);/*blue*/
    if (n_channels >= 4)
	    p[3] = geta(color); /* alpha */

}



static gint
getpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y)
{
	guchar *p;
	gint color = 0;
	
	p = pixels + (y * rowstride + x * n_channels);

    if (n_channels >= 4)
	    color = col_rgba(p[0], p[1], p[2], p[3]);
    else
        color = col_rgba(p[0], p[1], p[2], 255);

	return color;
}

/* Copped & modified from gpaint :) */
struct fillpixelinfo
{
   int y, xl, xr, dy;
};

#define STACKSIZE 10000

#define PUSH(py, pxl, pxr, pdy) \
{ \
    struct fillpixelinfo *p = sp;\
    if (((py) + (pdy) >= 0) && ((py) + (pdy) < info->height))\
    {\
        p->y = (py);\
        p->xl = (pxl);\
        p->xr = (pxr);\
        p->dy = (pdy);\
        sp++; \
    }\
}
   
#define POP(py, pxl, pxr, pdy) \
{\
    sp--;\
    (py) = sp->y + sp->dy;\
    (pxl) = sp->xl;\
    (pxr) = sp->xr;\
    (pdy) = sp->dy;\
}

static __inline__ int
is_old_pixel_value(struct fillinfo *info, int x, int y)
{
    unsigned char *p = info->rgb + y * info->rowstride + x * info->pixelsize;
    unsigned char or, og, ob, oa;
    or = *p;
    og = *(p + 1);
    ob = *(p + 2);
    if (info->pixelsize >= 4)
        oa = *(p + 3);
    else
        oa = 255;

    if ((or == info->or) && (og == info->og) && 
    	(ob == info->ob) && (oa == info->oa))
    {
        return 1;
    }
    return 0;
}


static __inline__ void
set_new_pixel_value(struct fillinfo *info, int x, int y)
{
    unsigned char *p = info->rgb + y * info->rowstride + x * info->pixelsize;
    
    *p		 = info->r;
    *(p + 1) = info->g;
    *(p + 2) = info->b;
    if (info->pixelsize >= 4)
        *(p + 3) = info->a;
    
    if (x <= info->gx)info->gx=x;
	if (x > info->gw)info->gw=x;
	if (y <= info->gy)info->gy=y;
	if (y > info->gh)info->gh=y;
}
/*
 * algorithm based on SeedFill.c from GraphicsGems 1
 */
static void
flood_fill_algo(struct fillinfo *info, int x, int y)
{
    /* TODO: check for stack overflow? */
    /* TODO: that's a lot of memory! esp if we never use it */
    struct fillpixelinfo stack[STACKSIZE];
    struct fillpixelinfo * sp = stack;
    int l, x1, x2, dy;
    
      
    if ((x >= 0) && (x < info->width) && (y >= 0) && (y < info->height))
    {
        if ((info->or == info->r) && (info->og == info->g) && (info->ob == info->b))
        {
            if (info->pixelsize < 4 || (info->oa == info->a))
                return;
        }
        PUSH(y, x, x, 1);
        PUSH(y + 1, x, x, -1);
        while (sp > stack)  
        {
            POP(y, x1, x2, dy);
            for (x = x1; (x >= 0) && is_old_pixel_value(info, x, y); x--)
            { 
                set_new_pixel_value(info, x, y);
            }
            if (x >= x1)
            {
                goto skip;
            }
            l = x + 1;
            if (l < x1)
            {
                PUSH(y, l, x1 - 1, -dy);
            }
            x = x1 + 1;
            do
            {
                for (; (x < info->width) && is_old_pixel_value(info, x, y); x++)
                {
                    set_new_pixel_value(info, x, y);
                }
                PUSH(y, l, x - 1, dy);
                if (x > x2 + 1)
                {
                    PUSH(y, x2 + 1, x - 1, -dy);
                }
skip:
                for (x++; x <= x2 && !is_old_pixel_value(info, x, y); x++) 
                {
                    /* empty */ ;
                }
                l = x;
            } while (x <= x2);
        }
    }
}  
