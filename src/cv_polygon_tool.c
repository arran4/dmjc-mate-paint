/***************************************************************************
 *            cv_polygon_tool.c
 *
 *  Thu Set 10 22:35:13 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 *  by Jacson
 ****************************************************************************/
 
 #include <gtk/gtk.h>

#include "cv_polygon_tool.h"
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
static void		draw_polygon_cairo	( cairo_t *cr, gboolean closed );
// static void     save_undo       ( void );

/*private data*/
typedef enum
{
	POLY_NONE,
	POLY_DRAWING,
	POLY_WAITING
} gp_poly_state;

typedef struct {
	gp_tool			tool;
	gp_canvas       *cv;
    GdkRGBA         color_f;
    GdkRGBA         color_b;
	guint			button;
	gboolean 		is_draw;
    gp_point_array  *pa;
	gp_poly_state	state;
} private_data;

static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv			=	NULL;
		m_priv->button		=	NONE_BUTTON;
		m_priv->is_draw		=	FALSE;
        m_priv->pa          =   gp_point_array_new();
		m_priv->state		=	POLY_NONE;		
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
tool_polygon_init ( gp_canvas * canvas )
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
		switch ( m_priv->state )
		{
			case POLY_NONE:
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
				m_priv->state = POLY_DRAWING;
				m_priv->is_draw	= TRUE;
				m_priv->button	= event->button;

                gp_point_array_clear ( m_priv->pa );
				/*add two point*/
                gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
                gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
                gtk_widget_queue_draw ( m_priv->cv->widget );
				break;
			}
			case POLY_DRAWING:
			{
				if ( m_priv->button != event->button )
				{
					/*cancel*/
					m_priv->state 		= POLY_NONE;
					m_priv->is_draw		= FALSE;
                    gp_point_array_clear ( m_priv->pa );
				}
				break;
			}
			case POLY_WAITING:
			{
				if ( m_priv->button == event->button )
				{
					/*next point*/
					m_priv->state = POLY_DRAWING;
                    gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
				}
				else
				{
 					/*finish*/
                    // save_undo ();
					m_priv->state = POLY_NONE;
					m_priv->is_draw	= FALSE;

                    if (m_priv->cv->surface) {
                        cairo_t *cr = cairo_create(m_priv->cv->surface);
                        draw_polygon_cairo(cr, TRUE);
                        cairo_destroy(cr);
                    }

					file_set_unsave ();
                    gp_point_array_clear ( m_priv->pa );
				}
				break;
			}
		}
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
    else if (event->type == GDK_2BUTTON_PRESS && m_priv->state != POLY_NONE) {
        // Finish on double click
        // save_undo ();
        m_priv->state = POLY_NONE;
        m_priv->is_draw	= FALSE;

        if (m_priv->cv->surface) {
            cairo_t *cr = cairo_create(m_priv->cv->surface);
            draw_polygon_cairo(cr, TRUE);
            cairo_destroy(cr);
        }

        file_set_unsave ();
        gp_point_array_clear ( m_priv->pa );
        gtk_widget_queue_draw ( m_priv->cv->widget );
    }
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if ( m_priv->state == POLY_DRAWING )
		{
			if( m_priv->button == event->button )
			{
				m_priv->state = POLY_WAITING;
			}
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->state == POLY_DRAWING )
	{
        gint index = gp_point_array_size ( m_priv->pa ) - 1;
        gp_point_array_set ( m_priv->pa, index, (gint)event->x, (gint)event->y );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void	
draw ( cairo_t *cr )
{
	if ( m_priv->is_draw )
	{
		draw_polygon_cairo (cr, FALSE);
	}
}

static void 
reset ( void )
{
	GdkCursor *cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_DOTBOX);
	g_assert(cursor);
    if (gtk_widget_get_window(m_priv->cv->widget))
	    gdk_window_set_cursor ( gtk_widget_get_window(m_priv->cv->widget), cursor );
	g_object_unref( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	gtk_widget_queue_draw ( m_priv->cv->widget );
	destroy_private_data ();
	g_print("polygon tool destroy\n");
}

static void
draw_polygon_cairo ( cairo_t *cr, gboolean closed )
{
	if ( gp_point_array_size (m_priv->pa) > 0 )
	{
		GdkPoint *	points		=	gp_point_array_data (m_priv->pa);
		gint		n_points	=	gp_point_array_size (m_priv->pa);

        if (n_points < 2) return;

        cairo_new_path(cr);
        cairo_move_to(cr, points[0].x + 0.5, points[0].y + 0.5);
        for (int i = 1; i < n_points; i++) {
            cairo_line_to(cr, points[i].x + 0.5, points[i].y + 0.5);
        }

        if (closed || m_priv->cv->filled != FILLED_NONE) {
            cairo_close_path(cr);
        }

		if ( m_priv->cv->filled == FILLED_BACK )
		{
            gdk_cairo_set_source_rgba(cr, &m_priv->color_b);
            cairo_fill_preserve(cr);
		}
		else if ( m_priv->cv->filled == FILLED_FORE )
		{
            gdk_cairo_set_source_rgba(cr, &m_priv->color_f);
            cairo_fill_preserve(cr);
		}

        gdk_cairo_set_source_rgba(cr, &m_priv->color_f);
        cairo_set_line_width(cr, m_priv->cv->line_width);
        cairo_stroke(cr);
	}
}
