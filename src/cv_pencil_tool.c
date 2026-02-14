/***************************************************************************
 *            cv_pencil_tool.c
 *
 *  Thu Set 10 22:35:13 2009
 *  Copyright  2009  Rogério Ferro do Nascimento
 *  <rogerioferro@gmail.com>
 ****************************************************************************/
 
#include <gtk/gtk.h>

#include "cv_pencil_tool.h"
#include "gp_point_array.h"
#include "undo.h"
#include "file.h"
#include "cv_drawing.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void		draw_in_cairo	( cairo_t *cr );
// static void     save_undo       ( void );


/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
    gp_point_array  *pa;
	guint			button;
	gboolean 		is_draw;
    GdkRGBA         color;
} private_data;

static private_data		*m_priv = NULL;

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv		=	NULL;
		m_priv->button	=	NONE_BUTTON;
        m_priv->pa      =   gp_point_array_new();
		m_priv->is_draw	=	FALSE;
	}
}

static void
destroy_private_data( void )
{
    gp_point_array_free( m_priv->pa );
	g_free (m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_pencil_init ( gp_canvas * canvas )
{
	create_private_data ();
	m_priv->cv					=	canvas;
	m_priv->tool.button_press	= 	button_press;
	m_priv->tool.button_release	= 	button_release;
	m_priv->tool.button_motion	= 	button_motion;
	m_priv->tool.draw			= 	draw;
	m_priv->tool.reset			= 	reset;
	m_priv->tool.destroy		= 	destroy;
	return &m_priv->tool;
}

static gboolean
button_press ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == LEFT_BUTTON )
		{
            m_priv->color = m_priv->cv->color_fg;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
            m_priv->color = m_priv->cv->color_bg;
		}
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw ) m_priv->button = event->button;
        gp_point_array_clear ( m_priv->pa );
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
		if( !m_priv->is_draw ) gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if( m_priv->button == event->button )
		{
			if( m_priv->is_draw )
			{
                gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
                // save_undo ();

                // Draw to permanent surface
                if (m_priv->cv->surface) {
                    cairo_t *cr = cairo_create(m_priv->cv->surface);
                    draw_in_cairo(cr);
                    cairo_destroy(cr);
                }

				file_set_unsave ();
			}
			gtk_widget_queue_draw ( m_priv->cv->widget );
			m_priv->is_draw = FALSE;
            gp_point_array_clear ( m_priv->pa );
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->is_draw )
	{
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void	
draw ( cairo_t *cr )
{
	if ( m_priv->is_draw )
	{
		draw_in_cairo (cr);
	}
}

static void 
reset ( void )
{
    GdkDisplay *display = gdk_display_get_default();
	GdkCursor *cursor = gdk_cursor_new_for_display (display, GDK_PENCIL);
	g_assert(cursor);
    if (gtk_widget_get_window(m_priv->cv->widget))
	    gdk_window_set_cursor ( gtk_widget_get_window(m_priv->cv->widget), cursor );
	g_object_unref( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("pencil tool destroy\n");
}

static void
draw_in_cairo ( cairo_t *cr )
{
	GdkPoint *	points		=	gp_point_array_data (m_priv->pa);
	gint		n_points	=	gp_point_array_size (m_priv->pa);

    if (n_points < 2) return;

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_set_line_width(cr, 1);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    // Cairo uses offset 0.5 for sharp single pixel lines
    cairo_move_to(cr, points[0].x + 0.5, points[0].y + 0.5);
    for (int i = 1; i < n_points; i++) {
        cairo_line_to(cr, points[i].x + 0.5, points[i].y + 0.5);
    }
    cairo_stroke(cr);
}

/*
static void     
save_undo ( void )
{
    // ...
 }
*/
