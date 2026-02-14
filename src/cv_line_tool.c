/***************************************************************************
 *            cv_line_tool.c
 *
 *  Wed Jun 10 21:22:13 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 ****************************************************************************/
 
 #include <gtk/gtk.h>

#include "cv_line_tool.h"
#include "cv_drawing.h"
#include "file.h"
#include "undo.h"
#include "gp_point_array.h"


/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void     draw_line_cairo ( cairo_t *cr );
// static void     save_undo       ( void );

/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
    GdkRGBA         color;
	gint 			x0,y0,x1,y1;
	guint			button;
	gboolean 		is_draw;
} private_data;

static private_data		*m_priv = NULL;

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_slice_new0 (private_data);
		m_priv->cv		=	NULL;
		m_priv->button	=	0;
		m_priv->is_draw	=	FALSE;
	}
}

static void
destroy_private_data( void )
{
	g_slice_free (private_data, m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_line_init ( gp_canvas * canvas )
{
	create_private_data ();
	m_priv->cv					= canvas;
	m_priv->tool.button_press	= button_press;
	m_priv->tool.button_release	= button_release;
	m_priv->tool.button_motion	= button_motion;
	m_priv->tool.draw			= draw;
	m_priv->tool.reset			= reset;
	m_priv->tool.destroy		= destroy;
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
		if( m_priv->is_draw ) 
		{
			m_priv->button = event->button;
		}
		m_priv->x0 = m_priv->x1 = (gint)event->x;
		m_priv->y0 = m_priv->y1 = (gint)event->y;
        gtk_widget_queue_draw ( m_priv->cv->widget );
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
                // save_undo ();
                if (m_priv->cv->surface) {
                    cairo_t *cr = cairo_create(m_priv->cv->surface);
                    draw_line_cairo(cr);
                    cairo_destroy(cr);
                }
				file_set_unsave ();
    		}
			gtk_widget_queue_draw ( m_priv->cv->widget );
			m_priv->is_draw = FALSE;
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->is_draw )
	{
		m_priv->x1 = (gint)event->x;
		m_priv->y1 = (gint)event->y;
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void
draw ( cairo_t *cr )
{
	if ( m_priv->is_draw )
	{
        draw_line_cairo(cr);
	}
}

static void reset ( void )
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display (display, GDK_DOTBOX);
	g_assert(cursor);
    if (gtk_widget_get_window(m_priv->cv->widget))
	    gdk_window_set_cursor ( gtk_widget_get_window(m_priv->cv->widget), cursor );
	g_object_unref( cursor );
	m_priv->is_draw = FALSE;
}

static void destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("line tool destroy\n");
}

static void draw_line_cairo(cairo_t *cr) {
    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_set_line_width(cr, m_priv->cv->line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_move_to(cr, m_priv->x0 + 0.5, m_priv->y0 + 0.5);
    cairo_line_to(cr, m_priv->x1 + 0.5, m_priv->y1 + 0.5);
    cairo_stroke(cr);
}

// save_undo commented out
