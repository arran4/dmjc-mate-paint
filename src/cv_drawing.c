/***************************************************************************
 *            cv_drawing.c
 *
 *  Sun Jun  7 11:31:18 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 ****************************************************************************/

#include "cv_drawing.h"
#include "cv_resize.h"
#include "cv_color_pick_tool.h"
#include "cv_flood_fill_tool.h"
#include "cv_line_tool.h"
#include "cv_pencil_tool.h"
#include "cv_rectangle_tool.h"
#include "cv_ellipse_tool.h"
#include "cv_polygon_tool.h"
#include "cv_paintbrush_tool.h"
#include "cv_rounded_rectangle_tool.h"
#include "cv_airbrush_tool.h"
#include "cv_curve_tool.h"
#include "cv_rect_select.h"
#include "undo.h"
#include "color-picker.h"
#include "cv_eraser_tool.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/*Member functions*/
static void		cv_create_pixmap	(gint width, gint height, gboolean b_resize);
static void		cv_print_pos		( gint x, gint y );


/* private data  */
static gp_canvas	cv;
static gp_tool		*cv_tool		=	NULL;
static GdkRGBA 	white_color		=	{ 1.0, 1.0, 1.0, 1.0 };
static GdkRGBA 	black_color		=	{ 0.0, 0.0, 0.0, 1.0 };
static GtkWidget	*lb_pos			=	NULL;
static gboolean		b_pos_pressed	=	FALSE;
static gint			x_pos,y_pos;


/*
 *   CODE
 */


void
cv_redraw ( void )
{
    gtk_widget_queue_draw (cv.widget);
}

void
cv_set_color_bg	( GdkRGBA *color )
{
    cv.color_bg = *color;
	gtk_widget_queue_draw ( cv.widget );
}

void
cv_set_color_fg	( GdkRGBA *color )
{
    cv.color_fg = *color;
	gtk_widget_queue_draw ( cv.widget );
}

void
cv_set_line_width	( gint width )
{
    cv.line_width = width;
	gtk_widget_queue_draw ( cv.widget );
}

void
cv_set_filled ( gp_filled filled )
{
	cv.filled	=	filled;
	gtk_widget_queue_draw ( cv.widget );
}

/* Set whether or not selections are transparent.
 * TRUE   - if transparent
 * FALSE  - if opaque
 */
void
cv_set_transparent ( gboolean transparent)
{
	cv.transparent	=	transparent;
	gtk_widget_queue_draw ( cv.widget );
}

void
cv_set_tool ( gp_tool_enum tool )
{
	if (cv_tool != NULL) cv_tool->destroy(NULL);
    switch ( tool )
    {
        default:
        case TOOL_NONE:
	        cv_tool = NULL;
            break;
        case TOOL_FREE_SELECT:
	        cv_tool = NULL;
            break;
        case TOOL_RECT_SELECT:
	        cv_tool = tool_rect_select_init ( &cv );
            break;
        case TOOL_ERASER:
	        cv_tool = tool_eraser_init ( &cv );
            break;
        case TOOL_COLOR_PICKER:
        	//cv_tool = tool_color_pick_init ( &cv );
            cv_tool = NULL;
            break;
        case TOOL_PENCIL:
            cv_tool = tool_pencil_init ( &cv );
            break;
        case TOOL_AIRBRUSH:
	        cv_tool = tool_airbrush_init ( &cv );
            break;
        case TOOL_BUCKET_FILL:
            cv_tool = tool_flood_fill_init ( &cv );
            break;
        case TOOL_ZOOM:
	        cv_tool = NULL;
            break;
        case TOOL_PAINTBRUSH:
	        cv_tool = tool_paintbrush_init ( &cv );
            break;
        case TOOL_TEXT:
	        cv_tool = NULL;
            break;
        case TOOL_LINE:
            cv_tool = tool_line_init ( &cv );
            break;
        case TOOL_RECTANGLE:
            cv_tool = tool_rectangle_init ( &cv );
            break;
        case TOOL_ELLIPSE:
            cv_tool = tool_ellipse_init ( &cv );
            break;
        case TOOL_CURVE:
	        cv_tool = tool_curve_init ( &cv );
            break;
        case TOOL_POLYGON:
            cv_tool = tool_polygon_init ( &cv );
            break;
        case TOOL_ROUNDED_RECTANGLE:
	        cv_tool = tool_rounded_rectangle_init ( &cv );
            break;
    }

    if (cv_tool != NULL) {
        cv_tool->reset();
    }

    // Cursor handling needs to be done via GdkWindow if realized, or GtkWidget in GTK3
    if (gtk_widget_get_realized(cv.widget)) {
        GdkWindow *window = gtk_widget_get_window(cv.widget);
        if (cv_tool == NULL)
             gdk_window_set_cursor(window, NULL);
    }
}


void  my_g_object_unref(gpointer data)
{
	g_print("g_object_unref\n");
	g_object_unref( G_OBJECT(data) );
}


void
cv_resize_pixmap ( gint width, gint height )
{
	cv_create_pixmap (width, height, TRUE); 
}


void
cv_set_pixbuf	(const GdkPixbuf	*pixbuf)
{
	if (pixbuf != NULL)
	{
	    GdkPixbuf *tmp ;
	    tmp = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
		gint width	=	gdk_pixbuf_get_width (pixbuf);
		gint height	=	gdk_pixbuf_get_height (pixbuf);

		cv_create_pixmap (width, height, FALSE);

		cairo_t *cr = cairo_create(cv.surface);
		gdk_cairo_set_source_pixbuf(cr, tmp, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);

        g_object_unref(tmp);
		gtk_widget_queue_draw (cv.widget);
	}
}

GdkPixbuf *
cv_get_pixbuf ( void )
{
	GdkPixbuf * pixbuf	=	NULL;
	if ( cv.surface != NULL )
	{
		gint w,h;
        w = cairo_image_surface_get_width(cv.surface);
        h = cairo_image_surface_get_height(cv.surface);

        pixbuf = gdk_pixbuf_get_from_surface(cv.surface, 0, 0, w, h);
	}
	return pixbuf;
}

gp_canvas *
cv_get_canvas ( void )
{
	return &cv;
}

void        
cv_get_rect_size ( GdkRectangle *rectangle )
{
    rectangle->x = 0;
    rectangle->y = 0;
    if (cv.surface) {
        rectangle->width = cairo_image_surface_get_width(cv.surface);
        rectangle->height = cairo_image_surface_get_height(cv.surface);
    } else {
        rectangle->width = 0;
        rectangle->height = 0;
    }
    return;
}


/* GUI CallBacks */

void
on_cv_drawing_realize (GtkWidget *widget, gpointer user_data)
{
	cv.widget		=	widget;
	cv.toplevel		=	gtk_widget_get_toplevel( widget );

	cv.surface		=	NULL;

	cv_set_color_fg ( &black_color );
	cv_set_color_bg ( &white_color );
	cv_set_line_width ( 1 );
    
	cv_set_filled ( FILLED_NONE );
	cv_set_transparent ( FALSE );

	cv_resize_set_canvas ( &cv );

	/* Create initial surface */
	cv_create_pixmap ( 320, 200, TRUE);
	cv.pb_clipboard = NULL;
}

void 
on_cv_drawing_unrealize	(GtkWidget *widget, gpointer user_data)
{
	/*free all private data*/
	g_print("unrealize canvas\n");
	if (cv.surface) {
	    cairo_surface_destroy(cv.surface);
	    cv.surface = NULL;
	}
	cv_set_tool ( TOOL_NONE );
}

void 
on_lb_pos_realize (GtkWidget *widget, gpointer user_data)
{
	lb_pos	=	widget;
}

/* events */
gboolean
on_cv_drawing_button_press_event (	GtkWidget	   *widget, 
                               		GdkEventButton *event,
                                	gpointer       user_data )
{
	gboolean ret	=	TRUE;
	b_pos_pressed	=	TRUE;
	x_pos	= (gint)event->x;
	y_pos	= (gint)event->y;
	cv_print_pos ( event->x, event->y );

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_press( event );
	}

	return ret;
}

gboolean
on_cv_drawing_button_release_event (	GtkWidget	   *widget, 
                                    	GdkEventButton *event,
                                    	gpointer       user_data )
{
	gboolean ret	=	TRUE;
	b_pos_pressed	=	FALSE;
	cv_print_pos ( event->x, event->y );

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_release( event );
	}

	return ret;
}
gboolean 
on_cv_drawing_leave_notify_event ( GtkWidget        *widget,
                                 GdkEventCrossing *event,
                                 gpointer          user_data)
{
	gtk_label_set_text( GTK_LABEL(lb_pos), "" );
    return TRUE;
}
									
gboolean
on_cv_drawing_motion_notify_event (	GtkWidget      *widget,
		                        	GdkEventMotion *event,
                                	gpointer        user_data)
{
	gboolean ret = TRUE;
	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_motion( event );
	}
	cv_print_pos ( event->x, event->y );
    gdk_event_request_motions(event);
	return ret;
}

gboolean 
on_cv_drawing_draw	(   GtkWidget	   *widget,
						cairo_t        *cr,
               					gpointer       user_data )
{
    if (cv.surface) {
	    cairo_set_source_surface (cr, cv.surface, 0, 0);
        cairo_paint(cr);
    } else {
        // Fallback clear
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_paint(cr);
    }

	if ( cv_tool != NULL )
	{
		cv_tool->draw(cr);
	}

	// cv_resize_draw(cr); // TODO: Port cv_resize_draw to accept cr
    return TRUE;
}

/*private functions*/

static void
cv_create_pixmap ( gint width, gint height, gboolean b_resize )
{
	cairo_surface_t *new_surface;
    cairo_t *cr;

    // Create image surface (simpler and safer than window surface for offscreen)
    new_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    cr = cairo_create(new_surface);

    // Fill with background (white)
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_paint(cr);

	if (b_resize && cv.surface)
	{
        // Draw old surface
        cairo_set_source_surface(cr, cv.surface, 0, 0);
        cairo_paint(cr);

        cairo_surface_destroy(cv.surface);
	}
    cairo_destroy(cr);

	cv.surface	=	new_surface;

	gtk_widget_set_size_request ( cv.widget, width, height );
	cv_resize_adjust_box_size (width, height);
    gtk_widget_queue_draw(cv.widget);
}

static void		
cv_print_pos ( gint x, gint y )
{
	GString *str = g_string_new("");
	if ( b_pos_pressed )
	{
		gint w	=	x - x_pos;
		gint h	=	y - y_pos;
		w = ( w < 0 )?(w-1):(w+1);
		h = ( h < 0 )?(h-1):(h+1);
		g_string_printf (str, "%d,%d->%d,%d (%dx%d)", x_pos, y_pos, x, y, w, h);
	}
	else
	{
		g_string_printf (str, "%d,%d", x, y );
	}
	gtk_label_set_text( GTK_LABEL(lb_pos), str->str );
	g_string_free( str, TRUE);
}
