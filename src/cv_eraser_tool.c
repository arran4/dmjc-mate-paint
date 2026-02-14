/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Contributed by Juan Balderas

	This file is part of mate-paint.
****************************************************************************/

#include <gtk/gtk.h>
#include <math.h>

#include "cv_eraser_tool.h"
#include "file.h"
#include "gp-image.h"
#include "toolbar.h"
#include "undo.h"
#include "cv_drawing.h"

#define ERASER_WIDTH	17
#define ERASER_HEIGHT	17

typedef enum{
	GP_ERASER_TYPE_ROUND,
	GP_ERASER_TYPE_RECTANGLE
}GPEraserType;

typedef void (DrawEraserFunc)(cairo_t *cr, int x, int y);

static const double EPSILON = 0.00001;

static gint m_last_eraser = GP_ERASER_RECT_TINY;

// static GdkPixbuf *g_pixbuf = NULL;

static void b_rush_interpolate(cairo_t *cr, DrawEraserFunc *draw_eraser_func, int x, int y);
static void set_eraser_values(GPEraserType type, GPEraserSize size);
static void set_eraser_size(GPEraserSize size);

/* Private drawing functions */
static void draw_round_eraser(cairo_t *cr, int x, int y);
static void draw_rectangular_eraser(cairo_t *cr, int x, int y);

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );
// static void     save_undo       ( void );

/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
    GdkRGBA         color;
	gint 			x0,y0;
	gint			x_min,y_min,x_max,y_max;
	guint			button;
	gboolean 		is_draw;
	double			spacing;        
    double			distance;
    GdkPoint		drag;
    DrawEraserFunc	*draw_eraser;
    GPEraserType	eraser_type;
    gint			width, height; /* eraser width & height */
    gint			xprev;
    gint			yprev;
} private_data;

static private_data		*m_priv = NULL;

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data, 1);
		m_priv->cv			=	NULL;
		m_priv->button		=	0;
		m_priv->is_draw		=	FALSE;
		m_priv->distance	=	0;
		m_priv->eraser_type	=	GP_ERASER_TYPE_RECTANGLE; /* change type here to test other eraseres */
		m_priv->width		=	ERASER_WIDTH;
		m_priv->height		=	ERASER_HEIGHT;
		
		set_eraser_values(m_priv->eraser_type, m_last_eraser);
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_eraser_init ( gp_canvas * canvas )
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
            m_priv->color = m_priv->cv->color_bg;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
            m_priv->color = m_priv->cv->color_fg;
		}
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw )
		{
			m_priv->button = event->button;
            // save_background();
		}
		m_priv->drag.x = (gint)event->x;
		m_priv->drag.y = (gint)event->y;
		m_priv->x0 = m_priv->drag.x;
		m_priv->y0 = m_priv->drag.y;
        m_priv->x_min = G_MAXINT;
        m_priv->x_max = G_MININT;
		m_priv->y_min = G_MAXINT;
        m_priv->y_max = G_MININT;
        m_priv->distance = 0;

		if( m_priv->is_draw )
		{
            if (m_priv->cv->surface) {
                cairo_t *cr = cairo_create(m_priv->cv->surface);
                b_rush_interpolate(cr, m_priv->draw_eraser, m_priv->x0, m_priv->y0);
                cairo_destroy(cr);
			    gtk_widget_queue_draw ( m_priv->cv->widget );
            }
		}
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
	gint x, y;
	gdouble xd, yd;

	if( m_priv->is_draw )
	{
		
		if (gdk_event_get_coords ((GdkEvent *) event, &xd, &yd))
		{
			x = (gint) xd;
			y = (gint) yd;
		}
		else
		{
			x = (gint) event->x;
			y = (gint) event->y;
		}
		
		m_priv->xprev = m_priv->x0;
		m_priv->yprev = m_priv->y0;
		
		m_priv->x0 = x;
		m_priv->y0 = y;

        if (m_priv->cv->surface) {
            cairo_t *cr = cairo_create(m_priv->cv->surface);
            b_rush_interpolate(cr, m_priv->draw_eraser, m_priv->x0, m_priv->y0);
            cairo_destroy(cr);
		gtk_widget_queue_draw ( m_priv->cv->widget );
        }
	}
	return TRUE;
}

static void	
draw ( cairo_t *cr )
{
	// Do nothing
}

static void 
reset ( void )
{
	GdkCursor *cursor;
	
    cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
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
	g_print("eraser tool destroy\n");
}

/*
	The following code was copped & modified from gpaint
*/
static void 
b_rush_interpolate(cairo_t *cr, DrawEraserFunc *draw_eraser_func, int x, int y)
{
	double dx;		/* delta x */
	double dy;		/* delta y */
	double moved;	/* mouse movement */	   
	double initial;	/* initial paint distance */
	double final;	/* final paint distance   */

	double points;	/* number of paint points */
	double next;	/* distance to the next point */
	double percent;	/* linear interplotation, 0 to 1.0 */
  
	/* calculate mouse move distance */
	dx = (double)(x - m_priv->drag.x);
	dy = (double)(y - m_priv->drag.y);
	moved = sqrt(dx*dx + dy*dy);

	initial = m_priv->distance;
	final = initial + moved;

	/* paint along the movement */
	while (m_priv->distance < final)
	{
		/* calculate distance to the next point */
		points = (int) (m_priv->distance / m_priv->spacing + 1.0 + EPSILON); 
		next = points * m_priv->spacing - m_priv->distance;
		m_priv->distance += next;		  
		if (m_priv->distance <= (final + EPSILON))
		{   
		   /* calculate interpolation */ 
			percent = (m_priv->distance - initial) / moved;
			x = (int)(m_priv->drag.x + percent * dx);
			y = (int)(m_priv->drag.y + percent * dy);

		    if (m_priv->x_min>x)m_priv->x_min=x;
		    if (m_priv->x_max<x)m_priv->x_max=x;
		    if (m_priv->y_min>y)m_priv->y_min=y;
		    if (m_priv->y_max<y)m_priv->y_max=y;
            
			draw_eraser_func(cr, x, y);
		}
	 }
	 m_priv->drag.x = x;
	 m_priv->drag.y = y;
	 m_priv->distance = final;
}

static void draw_round_eraser(cairo_t *cr, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_new_path(cr);
    cairo_arc(cr, x + m_priv->width/2.0, y + m_priv->height/2.0, m_priv->width/2.0, 0, 2*M_PI);
    cairo_fill(cr);
}

static void draw_rectangular_eraser(cairo_t *cr, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_rectangle(cr, x, y, m_priv->width, m_priv->height);
    cairo_fill(cr);
}

void on_eraser_size_toggled(GtkWidget *widget, gpointer data)
{
	static gint size;

	if(NULL != data)
	{
		size = *((gint *)data);
		if(m_last_eraser != size)
		{
			if((size >= GP_ERASER_RECT_LARGE) && (size <= GP_ERASER_RECT_TINY))
			{
				set_eraser_values(GP_ERASER_TYPE_RECTANGLE, size);
			}
			else if((size >= GP_ERASER_ROUND_LARGE) && (size <= GP_ERASER_ROUND_TINY))
			{
				set_eraser_values(GP_ERASER_TYPE_ROUND, size);
			}
			else
			{
				return;
			}

			m_last_eraser = *((gint *)data);
            // reset(); // update cursor
		}
		
	}
}

static void set_eraser_values(GPEraserType type, GPEraserSize size)
{
	switch(type)
		{
			case GP_ERASER_TYPE_ROUND:
				m_priv->eraser_type  =   type;
				m_priv->draw_eraser	=	draw_round_eraser;
				m_priv->spacing 	=	2.0;
				set_eraser_size(size);
				break;
			case GP_ERASER_TYPE_RECTANGLE:
				m_priv->eraser_type  =   type;
				m_priv->draw_eraser	=	draw_rectangular_eraser;
				m_priv->spacing 	=	2.0;
				set_eraser_size(size);
				break;
			default:
				printf("Debug: eraser set_eraser_values() unknown eraser type %d\n", m_priv->eraser_type);
				break;
		}
}

static void set_eraser_size(GPEraserSize size)
{
	switch(size)
		{
			case GP_ERASER_RECT_LARGE:	/* Fall through*/
			case GP_ERASER_ROUND_LARGE:
				m_priv->width  = ERASER_WIDTH;
				m_priv->height = ERASER_HEIGHT;
				break;
			case GP_ERASER_RECT_MEDIUM:	/* Fall through*/
			case GP_ERASER_ROUND_MEDIUM:
				m_priv->width  = ERASER_WIDTH / 2;
				m_priv->height = ERASER_HEIGHT / 2;
				break;
			case GP_ERASER_RECT_SMALL:	/* Fall through*/
			case GP_ERASER_ROUND_SMALL:	
				m_priv->width  = ERASER_WIDTH / 3;
				m_priv->height = ERASER_HEIGHT / 3;
				break;
			case GP_ERASER_RECT_TINY:	/* Fall through*/
			case GP_ERASER_ROUND_TINY:
				m_priv->width  = ERASER_WIDTH / 4;
				m_priv->height = ERASER_HEIGHT / 4;
				break;
			default:
				printf("Debug: set_eraser_size() Unknown eraser size: %d\n", size);
				return;
		}
}
