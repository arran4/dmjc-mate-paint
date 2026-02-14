/***************************************************************************
 *            cv_resize.c
 *
 *  Sun Jun  7 09:05:56 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 ****************************************************************************/
 
#include <gtk/gtk.h>

#include "common.h"
#include "cv_resize.h"
#include "cv_drawing.h"
#include "file.h"
#include "undo.h"

#define BOX_EDGE_SIZE	4


/* private functions */
static void cv_resize_start		( void );
static void cv_resize_move		( gdouble x,  gdouble y);
static void cv_resize_stop		( gdouble x,  gdouble y);
static void cv_resize_cancel	( void );

/* private data  */
static gp_canvas	*cv				=	NULL;
static GtkWidget	*cv_ev_box		=	NULL;
static GtkWidget	*cv_top_edge	=	NULL;
static GtkWidget	*cv_bottom_edge	=	NULL;
static GtkWidget	*cv_left_edge	=	NULL;
static GtkWidget	*cv_right_edge	=	NULL;
static GtkWidget	*lb_size		=	NULL;
static GdkRGBA 	    edge_color		=	{ 0.18, 0.21, 0.59, 1.0  }; // Approx from 0x2f00 0x3600 0x9800
static gboolean		b_resize		=	FALSE;
static gboolean		b_rz_init		=	FALSE;
static gint			x_res			=	0;
static gint			y_res			=	0;

/*
 *   CODE
 */

static gint
cv_widget_get_width ( GtkWidget *widget )
{
	GtkAllocation allocation;
	gtk_widget_get_allocation (widget, &allocation);
	return allocation.width;
}

static gint
cv_widget_get_height ( GtkWidget *widget )
{
	GtkAllocation allocation;
	gtk_widget_get_allocation (widget, &allocation);
	return allocation.height;
}

void
cv_resize_set_canvas ( gp_canvas * canvas )
{
	cv = canvas;
}


void
cv_resize_draw ( void )
{
    // Draw directly via GtkDrawingArea expose/draw signal in cv_drawing.c or similar
    // This function used to draw on canvas drawing area directly.
    // In GTK3 we can't draw outside draw signal easily.
    // We update UI labels here.

	GString *str = g_string_new("");
	gint x,y;

	if (b_resize)
	{
        // Drawing handled by event box draw signal
		x = x_res;
		y = y_res;
	}
	else
	{
		x = cv_widget_get_width (cv->widget);
		y = cv_widget_get_height (cv->widget);
	}
	g_string_printf (str, "%dx%d", x, y );
	gtk_label_set_text( GTK_LABEL(lb_size), str->str );
	g_string_free( str, TRUE);
}

void
cv_resize_adjust_box_size (gint width, gint height)
{
	gint w, h;
	w = ( width  < BOX_EDGE_SIZE )?width :BOX_EDGE_SIZE;
    h = ( height < BOX_EDGE_SIZE )?height:BOX_EDGE_SIZE;
	gtk_widget_set_size_request ( cv_top_edge		, w, BOX_EDGE_SIZE );
	gtk_widget_set_size_request ( cv_bottom_edge	, w, BOX_EDGE_SIZE );
	gtk_widget_set_size_request ( cv_left_edge		, BOX_EDGE_SIZE, h );
	gtk_widget_set_size_request ( cv_right_edge		, BOX_EDGE_SIZE, h );
}

/* GUI CallBacks */
void
on_cv_ev_box_realize (GtkWidget *widget, gpointer user_data)
{
	cv_ev_box	        =	widget;	
    // GC creation removed, handled in draw signal with Cairo
}

void
on_lb_size_realize (GtkWidget *widget, gpointer user_data)
{
	lb_size	=	widget;
}


void
on_cv_right_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	cv_right_edge = widget;
	gtk_widget_set_size_request ( widget, BOX_EDGE_SIZE, BOX_EDGE_SIZE );
	gtk_widget_override_background_color (widget, GTK_STATE_FLAG_NORMAL, &edge_color);
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_RIGHT_SIDE );
	g_assert (cursor);
	gdk_window_set_cursor ( gtk_widget_get_window(widget), cursor );
	g_object_unref ( cursor );
}

void 
on_cv_bottom_right_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	gtk_widget_set_size_request ( widget, BOX_EDGE_SIZE, BOX_EDGE_SIZE );
	gtk_widget_override_background_color (widget, GTK_STATE_FLAG_NORMAL, &edge_color);
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_BOTTOM_RIGHT_CORNER );
	g_assert (cursor);
	gdk_window_set_cursor ( gtk_widget_get_window(widget), cursor );
	g_object_unref ( cursor );
}

void
on_cv_bottom_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	cv_bottom_edge = widget;
	gtk_widget_set_size_request ( widget, BOX_EDGE_SIZE, BOX_EDGE_SIZE );
	gtk_widget_override_background_color (widget, GTK_STATE_FLAG_NORMAL, &edge_color);
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_BOTTOM_SIDE );
	g_assert (cursor);
	gdk_window_set_cursor ( gtk_widget_get_window(widget), cursor );
	g_object_unref ( cursor );
}

void
on_cv_top_realize (GtkWidget *widget, gpointer user_data)
{
	cv_top_edge = widget;
	on_cv_other_edge_realize( widget, user_data );
}

void
on_cv_left_realize (GtkWidget *widget, gpointer user_data)
{
	cv_left_edge = widget;
	on_cv_other_edge_realize( widget, user_data );
}


void
on_cv_other_edge_realize (GtkWidget *widget, gpointer user_data)
{
	// gtk_widget_override_color (widget, GTK_STATE_FLAG_NORMAL, &edge_color);
	gtk_widget_set_size_request ( widget, BOX_EDGE_SIZE, BOX_EDGE_SIZE );
}


/* events */
gboolean on_cv_other_edge_draw    (GtkWidget    *widget,
                                    cairo_t      *cr,
                                    gpointer       user_data )
{
    // Simple filled rect
    gdk_cairo_set_source_rgba(cr, &edge_color);
    cairo_paint(cr);
	return TRUE;
}

gboolean 
on_cv_ev_box_draw ( GtkWidget    *widget,
                        	cairo_t      *cr,
                        	gpointer       user_data )
{
	if (b_resize)
	{
        // Draw resize outline
		GtkAllocation allocation;
		gint x_offset;
		gint y_offset;
		gint x;
		gint y;

		gtk_widget_get_allocation (cv_ev_box, &allocation);

        // This assumes ev_box is parent of canvas widget? Or canvas is inside.
		x_offset = cv_widget_get_width (cv->widget) - allocation.x; // Logic might depend on hierarchy
		y_offset = cv_widget_get_height (cv->widget) - allocation.y;

        // Simplified logic: draw rectangle from 0,0 to x_res, y_res relative to canvas top-left
        // But we are in cv_ev_box coordinates.
        // Assuming cv_ev_box covers the canvas area + some margin?

		x = x_res;
		y = y_res;

        cairo_set_source_rgb (cr, 0, 0, 0);
        double dash[] = {4.0, 4.0};
        cairo_set_dash(cr, dash, 2, 0);
        cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 0.5, 0.5, x, y);
		cairo_stroke (cr);
	}
	gtk_widget_set_app_paintable ( cv_ev_box, b_resize );
	return TRUE;
}

// Alias for old signal names if needed
#define on_cv_other_edge_expose_event on_cv_other_edge_draw
#define on_cv_ev_box_expose_event on_cv_ev_box_draw


gboolean 
on_cv_bottom_right_button_press_event	(   GtkWidget	   *widget, 
											GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == LEFT_BUTTON )
		{
			cv_resize_start();
		}
		else if ( event->button == RIGHT_BUTTON )
		{
			cv_resize_cancel();
		}
	}
	return TRUE;
}


gboolean
on_cv_bottom_right_motion_notify_event (	GtkWidget      *widget,
                                     		GdkEventMotion *event,
                                            gpointer        user_data)
{
	cv_resize_move( event->x, event->y );
	return TRUE;
}

gboolean 
on_cv_bottom_right_button_release_event (   GtkWidget	   *widget, 
                                            GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( event->x,  event->y );
	}
	return TRUE;
}


gboolean 
on_cv_bottom_button_press_event (   GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	return on_cv_bottom_right_button_press_event( widget, event, user_data );
}

gboolean 
on_cv_bottom_motion_notify_event (  GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( 0.0, event->y );
	return TRUE;
}

gboolean
on_cv_bottom_button_release_event ( GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                  	gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( 0.0,  event->y );
	}
	return TRUE;
}

gboolean
on_cv_right_button_press_event (	GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	return on_cv_bottom_right_button_press_event( widget, event, user_data );
}

gboolean
on_cv_right_motion_notify_event (   GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( event->x, 0.0 );
	return TRUE;
}

gboolean
on_cv_right_button_release_event (  GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( event->x,  0.0 );
	}
	return TRUE;
}

/* private */
static void
cv_resize_start ( void )
{
	b_rz_init	=	TRUE;
}

static void
cv_resize_move ( gdouble x,  gdouble y)
{
	if( b_rz_init )
	{
		b_resize = TRUE;
		x_res = cv_widget_get_width(cv->widget) + (gint)x;
		y_res = cv_widget_get_height(cv->widget) + (gint)y;
		x_res	= (x_res<1)?1:x_res;
		y_res	= (y_res<1)?1:y_res;
		gtk_widget_queue_draw (cv_ev_box);
        cv_resize_draw();
	}
}

static void
cv_resize_stop ( gdouble x,  gdouble y)
{
	if( b_resize )
	{
		gint width, height;
		width = cv_widget_get_width(cv->widget) + (gint)x;
		width = (width<1)?1:width;
		height = cv_widget_get_height(cv->widget) + (gint)y;
		height = (height<1)?1:height;

        undo_add_resize ( width, height );
        cv_resize_pixmap ( width, height );
		file_set_unsave ();
	}
	cv_resize_cancel();
}

static void
cv_resize_cancel ( void )
{
	b_rz_init		= FALSE;
	b_resize 	= FALSE;
	gtk_widget_queue_draw (cv_ev_box);
    cv_resize_draw();
}
