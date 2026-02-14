/***************************************************************************
    Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
    Contributed by Juan Balderas

    This file is part of mate-paint.
****************************************************************************/

#include <gtk/gtk.h>

#include "cv_color_pick_tool.h"
#include "pixbuf_util.h"
#include "file.h"
#include "cv_drawing.h"
#include "color.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( cairo_t *cr );
static void		reset			( void );
static void		destroy			( gpointer data  );

/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
	gint 			x0,y0;
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
tool_color_pick_init ( gp_canvas * canvas )
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

// cv_set_color_fg ( &GdkColor );
gboolean
button_press ( GdkEventButton *event )
{
	guint color = 0;
	GdkPixbuf *pixbuf = NULL;

	if ( event->type == GDK_BUTTON_PRESS )
	{
		// if ( event->button == LEFT_BUTTON )
		// {
		// 	m_priv->gc = m_priv->cv->gc_fg;
		// }
		// else if ( event->button == RIGHT_BUTTON )
		// {
		// 	m_priv->gc = m_priv->cv->gc_bg;
		// }
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw ) m_priv->button = event->button;

		m_priv->x0 = (gint)event->x;
		m_priv->y0 = (gint)event->y;

		pixbuf = cv_get_pixbuf ( );

		if( GDK_IS_PIXBUF( pixbuf ) )
		{
			if(!gdk_pixbuf_get_has_alpha ( pixbuf ) )
			{
				GdkPixbuf *tmp ;
				tmp = gdk_pixbuf_add_alpha( pixbuf, FALSE, 0, 0, 0 );
				g_object_unref(pixbuf);
				pixbuf = tmp;
			}
			if(get_pixel_from_pixbuf( pixbuf, &color, m_priv->x0, m_priv->y0) )
			{
				foreground_set_color_from_rgb  ( color );

                // If using foreground_set_color_from_rgb, ensure it updates the correct color based on button.
                // But foreground_set_color_from_rgb currently assumes foreground.
                // The tool logic might need update to set background if right click.
                // For now, consistent with existing logic (color.c logic sets foreground always in that func).

                // Logic update:
                GdkRGBA rgba;
                rgba.red = ((color >> 24) & 0xFF) / 255.0;
                rgba.green = ((color >> 16) & 0xFF) / 255.0;
                rgba.blue = ((color >> 8) & 0xFF) / 255.0;
                rgba.alpha = (color & 0xFF) / 255.0;

                if (event->button == RIGHT_BUTTON)
                    cv_set_color_bg(&rgba);
                else
                    cv_set_color_fg(&rgba); // Also updates UI
			}
			
			g_object_unref ( pixbuf );
		}
		if( !m_priv->is_draw ) gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

gboolean
button_release ( GdkEventButton *event )
{
	
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
	if ( m_priv->is_draw )
	{

	}
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
	g_print("color_pick tool destroy\n");
}
