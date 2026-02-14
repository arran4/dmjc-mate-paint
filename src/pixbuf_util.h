#ifndef _PIXBUF_UTIL_H_
#define _PIXBUF_UTIL_H_

#include <gtk/gtk.h>

#define col(r, g ,b)  ((unsigned int) (((unsigned char) (b) | \
					  ((unsigned short) (g) << 8)) | \
					  (((unsigned int) (unsigned char) (r)) << 16)))

#define col_rgba(r,g,b,a) ((guint)(((guchar)(a))| \
						  (((guint)(guchar)(b))<<8)| \
						  (((guint)(guchar)(g))<<16)| \
						  (((guint)(guchar)(r))<<24)))

#define getr(x) (((x >> 24) & 0x0FF))
#define getg(x) (((x >> 16) & 0x0FF))
#define getb(x) (((x >> 8) & 0x0FF))
#define geta(x) ((x & 0x0FF))

/* Modified to work on GdkPixbuf directly */
GdkRectangle fill_draw(GdkPixbuf *pixbuf, guint fill_color, guint x, guint y);

gboolean get_pixel_from_pixbuf(GdkPixbuf *pixbuf, guint *color,
                               guint x, guint y);


#endif
