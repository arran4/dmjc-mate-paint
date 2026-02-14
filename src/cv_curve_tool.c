/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Copyright (C) Juan Balderas 2010

	cv_curve_tool.c is part of mate-paint.
****************************************************************************/
 
#include <gtk/gtk.h>

#include "cv_curve_tool.h"
#include "cv_drawing.h"
#include "file.h"
#include "undo.h"
#include "gp_point_array.h"

typedef enum{
	GP_CURVE_DO_LINE,
	GP_CURVE_DO_CURVE,
	GP_CURVE_SET
}GPCurveAction;

static void draw_bezier_cairo(cairo_t *cr, GdkPoint pt1, GdkPoint pt2, GdkPoint pt3);

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
    GdkRGBA         color;
	guint			button;
	gboolean 		is_draw;
	GdkPoint		start, crv, end;
	gint			action;
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
        m_priv->action	=	GP_CURVE_DO_LINE;
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_curve_init ( gp_canvas * canvas )
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
		if( m_priv->is_draw ) m_priv->button = event->button;

        switch(m_priv->action){
        	case GP_CURVE_DO_LINE:
        		m_priv->start.x = (gint)event->x;
        		m_priv->start.y = (gint)event->y;
        		m_priv->end = m_priv->start;
				break;
			case GP_CURVE_DO_CURVE:
				m_priv->crv.x = (gint)event->x;
        		m_priv->crv.y = (gint)event->y;
				break;
			case GP_CURVE_SET:
				break;
			default:
				break;
        }
        
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
   	            switch(m_priv->action)
   	            {
        			case GP_CURVE_DO_LINE:
        				m_priv->is_draw = TRUE;
        				m_priv->end.x = (gint)event->x;
        				m_priv->end.y = (gint)event->y;
       			 		m_priv->action = GP_CURVE_DO_CURVE;
       			 		m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
						m_priv->is_draw = FALSE;
						return TRUE;
					case GP_CURVE_DO_CURVE:
						m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
        				m_priv->action = GP_CURVE_SET;
						m_priv->is_draw = FALSE;
						return TRUE;
					case GP_CURVE_SET:
						m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
						// save_undo ();
                        if (m_priv->cv->surface) {
                            cairo_t *cr = cairo_create(m_priv->cv->surface);
                            draw_in_cairo(cr);
                            cairo_destroy(cr);
                        }
						file_set_unsave ();
            			m_priv->action = GP_CURVE_DO_LINE;
						break;
					default:
						return TRUE;
				}
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
        switch(m_priv->action){
      	  	case GP_CURVE_DO_LINE:
      	  		m_priv->end.x = (gint)event->x;
      	  		m_priv->end.y = (gint)event->y;
				break;
			case GP_CURVE_DO_CURVE:
				m_priv->crv.x = (gint)event->x;
        		m_priv->crv.y = (gint)event->y;
				break;
			case GP_CURVE_SET:
				break;
			default:
				return TRUE;
        }
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
	GdkCursor *cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
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
	g_print("curve tool destroy\n");
}

static void
draw_in_cairo ( cairo_t *cr )
{
    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_set_line_width(cr, m_priv->cv->line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    switch(m_priv->action)
    {
       	case GP_CURVE_DO_LINE:
            cairo_move_to(cr, m_priv->start.x + 0.5, m_priv->start.y + 0.5);
            cairo_line_to(cr, m_priv->end.x + 0.5, m_priv->end.y + 0.5);
            cairo_stroke(cr);
			break;
		case GP_CURVE_DO_CURVE:
			draw_bezier_cairo(cr, m_priv->start, m_priv->crv, m_priv->end);
			break;
		case GP_CURVE_SET:
			draw_bezier_cairo(cr, m_priv->start, m_priv->crv, m_priv->end);
			break;
		default:
			break;
    }
}

static void draw_bezier_cairo(cairo_t *cr, GdkPoint pt1, GdkPoint pt2, GdkPoint pt3)
{
	double t, t2;
    double x, y;
	
    cairo_new_path(cr);
    cairo_move_to(cr, pt1.x + 0.5, pt1.y + 0.5);
	
	for(t = 0.0; t < 1.0 + .02; t += .02)
	{
		t2 = t;

		x = ((1 - t2) * (1 - t2)) *
			pt1.x + 2 * t2 * (1 -t2) *
			pt2.x + (t2 * t2) *
			pt3.x ;

		y = ((1 - t2) * (1 - t2)) *
			pt1.y + 2 * t2 * (1 -t2) *
			pt2.y + (t2 * t2) *
			pt3.y ;

        cairo_line_to(cr, x + 0.5, y + 0.5);
	}
    cairo_stroke(cr);
}
