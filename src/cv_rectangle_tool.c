/***************************************************************************
 *            cv_rectangle_tool.c
 *
 *  Thu Set 10 22:35:13 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 *  by Jacson
 ****************************************************************************/
 
#include <gtk/gtk.h>

#include "cv_rectangle_tool.h"
#include "file.h"
#include "cv_drawing.h"
#include "undo.h"
#include "gp_point_array.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void		draw_rectangle_cairo	( cairo_t *cr );
// static void     save_undo       ( void );


/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
	GdkRGBA         color_f; // Fore
	GdkRGBA         color_b; // Back
    gp_point_array  *pa;
	guint			button;
	gboolean 		is_draw;
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
		m_priv->is_draw	=	FALSE;
        m_priv->pa      =   gp_point_array_new();
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
tool_rectangle_init ( gp_canvas * canvas )
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
            m_priv->color_f = m_priv->cv->color_fg;
            m_priv->color_b = m_priv->cv->color_bg;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
            m_priv->color_f = m_priv->cv->color_bg;
            m_priv->color_b = m_priv->cv->color_fg;
		}
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw ) m_priv->button = event->button;

        gp_point_array_clear ( m_priv->pa );
		/*add two point*/
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        
		if( !m_priv->is_draw ) gtk_widget_queue_draw ( m_priv->cv->widget );
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
                    draw_rectangle_cairo(cr);
                    cairo_destroy(cr);
                }
				file_set_unsave ();
			}
			gtk_widget_queue_draw ( m_priv->cv->widget );
            gp_point_array_clear ( m_priv->pa );
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
        gp_point_array_set ( m_priv->pa, 1, (gint)event->x, (gint)event->y );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void	
draw ( cairo_t *cr )
{
	if ( m_priv->is_draw )
	{
		draw_rectangle_cairo (cr);
	}
}

static void 
reset ( void )
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display (display, GDK_DOTBOX);
	g_assert(cursor);
    if (gtk_widget_get_window(m_priv->cv->widget))
	    gdk_window_set_cursor ( gtk_widget_get_window(m_priv->cv->widget), cursor );
	g_object_unref ( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("rectangle tool destroy\n");
}

static void
draw_rectangle_cairo ( cairo_t *cr )
{
    GdkPoint        *p = gp_point_array_data (m_priv->pa);
    if (gp_point_array_size(m_priv->pa) < 2) return;

    gint x = MIN(p[0].x, p[1].x);
    gint y = MIN(p[0].y, p[1].y);
    gint w = ABS(p[1].x - p[0].x);
    gint h = ABS(p[1].y - p[0].y);

	if ( m_priv->cv->filled == FILLED_BACK )
	{
        gdk_cairo_set_source_rgba(cr, &m_priv->color_b);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
	}
	else if ( m_priv->cv->filled == FILLED_FORE )
	{
        gdk_cairo_set_source_rgba(cr, &m_priv->color_f);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
	}

    // Outline
    gdk_cairo_set_source_rgba(cr, &m_priv->color_f);
    cairo_set_line_width(cr, m_priv->cv->line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    cairo_rectangle(cr, x + 0.5, y + 0.5, w, h);
    cairo_stroke(cr);
}
