/***************************************************************************
    Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
    Contributed by Juan Balderas

    This file is part of mate-paint.
****************************************************************************/
 
 #include <gtk/gtk.h>

#include "cv_flood_fill_tool.h"
#include "file.h"
#include "cv_drawing.h"
#include "pixbuf_util.h"
#include "undo.h"

guint get_fg_color_from_rgba(GdkRGBA *color);

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );
// static void		save_undo		( void );

/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
	gint 			x0,y0;
	guint			button;
	gboolean 		is_draw;
	guint			fill_color;
	GdkRectangle	rect;
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
		m_priv->button	=	0;
		m_priv->is_draw	=	FALSE;
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_flood_fill_init ( gp_canvas * canvas )
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

gboolean
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
		m_priv->x0 = (gint)event->x;
		m_priv->y0 = (gint)event->y;
		if( !m_priv->is_draw ) gtk_widget_queue_draw ( m_priv->cv->widget );

		m_priv->fill_color = get_fg_color_from_rgba(&m_priv->color);
	}
	return TRUE;
}

gboolean
button_release ( GdkEventButton *event )
{
	GdkPixbuf *pixbuf;

	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if( m_priv->button == event->button )
		{
			if( m_priv->is_draw )
			{
                // Get pixbuf from surface
				pixbuf = cv_get_pixbuf ( );
				if(GDK_IS_PIXBUF ( pixbuf ) )
				{
                    // Ensure alpha
					if(!gdk_pixbuf_get_has_alpha ( pixbuf ) )
					{
						GdkPixbuf *tmp ;
						tmp = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
						g_object_unref(pixbuf);
						pixbuf = tmp;
					}

                    // Flood fill directly on pixbuf
					m_priv->rect = fill_draw( pixbuf,
				    	      m_priv->fill_color, 
				    	      m_priv->x0, 
			    		      m_priv->y0);
					
                    // Write back to surface
                    cv_set_pixbuf(pixbuf);

					g_object_unref(pixbuf);

					// save_undo ();
					
					file_set_unsave ();
				}
			}
			gtk_widget_queue_draw ( m_priv->cv->widget );
			m_priv->is_draw = FALSE;
		}
	}
	return TRUE;
}

gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->is_draw )
	{

	}
	return TRUE;
}

void	
draw ( cairo_t *cr )
{
    // Nothing to draw during preview
}

void reset ( void )
{
	GdkCursor *cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
	g_assert(cursor);
    if (gtk_widget_get_window(m_priv->cv->widget))
	    gdk_window_set_cursor ( gtk_widget_get_window(m_priv->cv->widget), cursor );
	g_object_unref ( cursor );
	m_priv->is_draw = FALSE;
}

void destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("flood fill tool destroy\n");
}

guint get_fg_color_from_rgba(GdkRGBA *color)
{
	guint c = 0;
	c = col_rgba((guchar)(color->red * 255),
	            (guchar)(color->green * 255),
	            (guchar)(color->blue * 255),
                (guchar)(color->alpha * 255));
	return c;
}
