/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Contributed by Juan Balderas

	This file is part of mate-paint.
****************************************************************************/

#include <gtk/gtk.h>
#include <math.h>

#include "cv_paintbrush_tool.h"
#include "file.h"
#include "gp-image.h"
#include "toolbar.h"
#include "undo.h"
#include "cv_drawing.h"

#define BRUSH_WIDTH		17
#define BRUSH_HEIGHT	17

typedef enum{
	GP_BRUSH_TYPE_ROUND,
	GP_BRUSH_TYPE_RECTANGLE,
	GP_BRUSH_TYPE_FWRD_SLASH,
	GP_BRUSH_TYPE_BACK_SLASH,
	GP_BRUSH_TYPE_PIXBUF
}GPBrushType;

typedef void (DrawBrushFunc)(cairo_t *cr, int x, int y);

static const double EPSILON = 0.00001;

/* TODO: These should be part of gp_canvas */
static gint g_prev_brush_size = GP_BRUSH_ROUND_LARGE;
static gint g_brush_type = GP_BRUSH_TYPE_ROUND;

// static GdkPixbuf *g_pixbuf = NULL; // Unused for now

static void brush_interpolate(cairo_t *cr, DrawBrushFunc *draw_brush_func, int x, int y);
static void set_brush_values(GPBrushType type, GPBrushSize size);
static void set_brush_size(GPBrushSize size);

/* Private drawing functions */
static void draw_round_brush(cairo_t *cr, int x, int y);
static void draw_rectangular_brush(cairo_t *cr, int x, int y);
static void draw_back_slash_brush(cairo_t *cr, int x, int y);
static void draw_fwd_slash_brush(cairo_t *cr, int x, int y);
// static void draw_pixbuf_brush(cairo_t *cr, int x, int y);

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
    DrawBrushFunc	*draw_brush;
    GPBrushType		brush_type;
    gint			width, height; /* Brush width & height */
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
		m_priv->brush_type	=	g_brush_type;
		m_priv->width		=	BRUSH_WIDTH;
		m_priv->height		=	BRUSH_HEIGHT;

		set_brush_values(m_priv->brush_type, g_prev_brush_size);
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_paintbrush_init ( gp_canvas * canvas )
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
		if( m_priv->is_draw )
		{
			m_priv->button = event->button;
            // save_background(); // Undo setup
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
            // Initial dot
            if (m_priv->cv->surface) {
                cairo_t *cr = cairo_create(m_priv->cv->surface);
                brush_interpolate(cr, m_priv->draw_brush, m_priv->x0, m_priv->y0);
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
		m_priv->x0 = x;
		m_priv->y0 = y;

        if (m_priv->cv->surface) {
            cairo_t *cr = cairo_create(m_priv->cv->surface);
            brush_interpolate(cr, m_priv->draw_brush, m_priv->x0, m_priv->y0);
            cairo_destroy(cr);
		gtk_widget_queue_draw ( m_priv->cv->widget );
        }
	}
	return TRUE;
}

static void	
draw ( cairo_t *cr )
{
    // Nothing to do in expose/draw handler as we draw directly to surface
}

static void 
reset ( void )
{
	GdkCursor *cursor;
	
    // Simple crosshair cursor
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
	g_print("paintbrush tool destroy\n");
}

static void 
brush_interpolate(cairo_t *cr, DrawBrushFunc *draw_brush_func, int x, int y)
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
            
			draw_brush_func(cr, x, y);
		}
	 }
     // Check if we didn't move enough for interpolation but it's a start (moved == 0 handled by loop condition usually, but if distance=0 and final=0, loop doesn't run. But we want initial dot.)
     // Handled in button_press separately? Or assume distance starts at 0.
     // If moved is small, we update drag.

	 m_priv->drag.x = x; // Update drag to current target
	 m_priv->drag.y = y;
	 m_priv->distance = final; // Or reset distance? No, keep it relative to spacing.
     // In original code: m_priv->distance = final;

     // Original code didn't update drag inside the loop, only outside?
     // Original: m_priv->drag.x = m_priv->x0; in draw_in_pixmap.
     // Here I'm updating it.

     // The original code passed m_priv->x0, m_priv->y0 to brush_interpolate, which were updated in motion.
     // brush_interpolate used m_priv->drag (last pos) and x,y (new pos).
     // After brush_interpolate returned, draw_in_pixmap updated m_priv->drag to x0,y0.

     // My implementation:
     // m_priv->drag is updated to the NEW x,y passed in.
}

/* m_priv->spacing = 2.0 is best for this brush */
static void draw_round_brush(cairo_t *cr, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_new_path(cr);
    cairo_rectangle(cr, x, y, m_priv->width, m_priv->height);
    // Wait, round brush should be arc?
    // Original: gdk_draw_arc(..., 360*64).

    cairo_new_path(cr);
    cairo_arc(cr, x + m_priv->width/2.0, y + m_priv->height/2.0, m_priv->width/2.0, 0, 2*M_PI);
    cairo_fill(cr);
}

/* m_priv->spacing = 2.0 is best for this brush */
static void draw_rectangular_brush(cairo_t *cr, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_rectangle(cr, x, y, m_priv->width, m_priv->height);
    cairo_fill(cr);
}

/* m_priv->spacing = 1.0 is best for this brush */
static void draw_back_slash_brush(cairo_t *cr, int x, int y)
{
    gint w, h;
    w = m_priv->width - 1;
    h = m_priv->height - 1;
    x -= (m_priv->width / 2);
    y -= (m_priv->height / 2);

    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_set_line_width(cr, 1);

    cairo_move_to(cr, x + 0.5, y + 0.5);
    cairo_line_to(cr, x + w + 0.5, y + h + 0.5);
    cairo_stroke(cr);

    // Gap filling
    cairo_move_to(cr, x + 1.5, y + 0.5);
    cairo_line_to(cr, x + w + 0.5, y + h - 1.0 + 0.5);
    cairo_stroke(cr);
}

/* m_priv->spacing = 1.0 is best for this brush */
static void draw_fwd_slash_brush(cairo_t *cr, int x, int y)
{
    gint w, h;
    w = m_priv->width - 1;
    h = m_priv->height - 1;
    x -= (m_priv->width / 2);
    y -= (m_priv->height / 2);
   
    gdk_cairo_set_source_rgba(cr, &m_priv->color);
    cairo_set_line_width(cr, 1);

    cairo_move_to(cr, x + 0.5, y + h + 0.5);
    cairo_line_to(cr, x + w + 0.5, y + 0.5);
    cairo_stroke(cr);

    // Gap filling
    cairo_move_to(cr, x + 1.5, y + h + 0.5);
    cairo_line_to(cr, x + w + 0.5, y + 1.0 + 0.5);
    cairo_stroke(cr);
}

void on_brush_size_toggled(GtkWidget *widget, gpointer data)
{
	static gint size;
	if(NULL != data)
	{
		size = *((gint *)data);
		if(g_prev_brush_size != size)
		{
			if((size >= GP_BRUSH_RECT_LARGE) && (size <= GP_BRUSH_RECT_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_ROUND, size);
			}
			else if((size >= GP_BRUSH_ROUND_LARGE) && (size <= GP_BRUSH_ROUND_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_RECTANGLE, size);
			}
			else if((size >= GP_BRUSH_FWRD_LARGE) && (size <= GP_BRUSH_FWRD_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_FWRD_SLASH, size);
			}
			else if((size >= GP_BRUSH_BACK_LARGE) && (size <= GP_BRUSH_BACK_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_BACK_SLASH, size);
			}
			g_prev_brush_size = size;
            // Update cursor? Skipped for now.
		}
	}
}
 
static void set_brush_values(GPBrushType type, GPBrushSize size)
{
	switch(type)
		{
			case GP_BRUSH_TYPE_ROUND:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_round_brush;
				m_priv->spacing 	=	2.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_RECTANGLE:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_rectangular_brush;
				m_priv->spacing 	=	2.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_FWRD_SLASH:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_fwd_slash_brush;
				m_priv->spacing 	=	1.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_BACK_SLASH:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_back_slash_brush;
				m_priv->spacing 	=	1.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_PIXBUF:
                // Not supported yet
				break;
			default:
				break;
		}
	g_brush_type = type;
}

static void set_brush_size(GPBrushSize size)
{
	switch(size)
		{
			case GP_BRUSH_RECT_LARGE:	/* Fall through*/
			case GP_BRUSH_ROUND_LARGE:	/* Fall through*/
			case GP_BRUSH_FWRD_LARGE:	/* Fall through*/
			case GP_BRUSH_BACK_LARGE:
				m_priv->width  = BRUSH_WIDTH;
				m_priv->height = BRUSH_HEIGHT;
				break;
			case GP_BRUSH_RECT_MEDIUM:	/* Fall through*/
			case GP_BRUSH_ROUND_MEDIUM:	/* Fall through*/
			case GP_BRUSH_FWRD_MEDIUM:	/* Fall through*/
			case GP_BRUSH_BACK_MEDIUM:
				m_priv->width = BRUSH_WIDTH / 2;
				m_priv->height = BRUSH_WIDTH / 2;
				break;
			case GP_BRUSH_RECT_SMALL:	/* Fall through*/
			case GP_BRUSH_ROUND_SMALL:	/* Fall through*/
			case GP_BRUSH_FWRD_SMALL:	/* Fall through*/
			case GP_BRUSH_BACK_SMALL:	/* Fall through*/
				m_priv->width = BRUSH_WIDTH / 4;
				m_priv->height = BRUSH_WIDTH / 4;
				break;
			default:
				return;
		}
	if(0 == m_priv->width % 2){
		m_priv->width++;
	}
	if(0 == m_priv->height % 2){
		m_priv->height++;
	}
}
