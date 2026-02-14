/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pick_color
 * Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
 * 
 * pick_color is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include  <gdk/gdk.h>
#include <math.h>

#include "color-picker.h"


enum {
	COLOR_CHANGED,
	RELEASED,
	LAST_SIGNAL
};


/* The cursor for the dropper */
#define DROPPER_WIDTH 17
#define DROPPER_HEIGHT 17
#define DROPPER_X_HOT 2
#define DROPPER_Y_HOT 16

#define BIG_STEP 20


static const guchar dropper_bits[] = {
  0xff, 0x8f, 0x01, 0xff, 0x77, 0x01, 0xff, 0xfb, 0x00, 0xff, 0xf8, 0x00,
  0x7f, 0xff, 0x00, 0xff, 0x7e, 0x01, 0xff, 0x9d, 0x01, 0xff, 0xd8, 0x01,
  0x7f, 0xd4, 0x01, 0x3f, 0xee, 0x01, 0x1f, 0xff, 0x01, 0x8f, 0xff, 0x01,
  0xc7, 0xff, 0x01, 0xe3, 0xff, 0x01, 0xf3, 0xff, 0x01, 0xfd, 0xff, 0x01,
  0xff, 0xff, 0x01, };

static const guchar dropper_mask[] = {
  0x00, 0x70, 0x00, 0x00, 0xf8, 0x00, 0x00, 0xfc, 0x01, 0x00, 0xff, 0x01,
  0x80, 0xff, 0x01, 0x00, 0xff, 0x00, 0x00, 0x7f, 0x00, 0x80, 0x3f, 0x00,
  0xc0, 0x3f, 0x00, 0xe0, 0x13, 0x00, 0xf0, 0x01, 0x00, 0xf8, 0x00, 0x00,
  0x7c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x0d, 0x00, 0x00,
  0x02, 0x00, 0x00, };


static void		shutdown_eyedropper (ColorPicker *colorpicker);
static gboolean mouse_press 		(GtkWidget         *invisible,
                            		 GdkEventButton    *event,
                            		 gpointer           data);



static guint picker_signals[LAST_SIGNAL] = { 0 };


struct _ColorPickerPrivate
{
	guint		has_grab : 1;
	GdkRGBA	color;
	/* Window for grabbing on */
	GtkWidget   *dropper_grab_widget;
    GdkSeat     *grab_seat;
};

G_DEFINE_TYPE (ColorPicker, color_picker, G_TYPE_OBJECT);

#define COLOR_PICKER_GET_PRIVATE(object)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), COLOR_TYPE_PICKER, ColorPickerPrivate))

static void
color_picker_init (ColorPicker *object)
{
	object->priv = COLOR_PICKER_GET_PRIVATE (object);
	object->priv->dropper_grab_widget = NULL;
    object->priv->grab_seat = NULL;
}

static void
color_picker_finalize (GObject *object)
{
	ColorPickerPrivate *priv;
	priv	=	COLOR_PICKER (object)->priv;
	if ( priv->dropper_grab_widget != NULL )
	{
		g_object_unref ( priv->dropper_grab_widget );
	}

	G_OBJECT_CLASS (color_picker_parent_class)->finalize (object);
}

static void
color_picker_class_init (ColorPickerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = color_picker_finalize;

	g_type_class_add_private (object_class, sizeof (ColorPickerPrivate));

	picker_signals[COLOR_CHANGED] =
		g_signal_new ("color-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (ColorPickerClass, color_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	picker_signals[RELEASED] =
		g_signal_new ("released",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (ColorPickerClass, released),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

}



static gboolean 
color_picker_grab_broken (GtkWidget          *widget,
			   			  GdkEventGrabBroken *event,
                          gpointer   data)
{
  shutdown_eyedropper (COLOR_PICKER (data));

  return TRUE;
}



static GdkCursor *
make_picker_cursor (GdkScreen *screen)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new_from_name (gdk_screen_get_display (screen),
				     "color-picker");

  if (!cursor)
    {
      // Fallback
      cursor = gdk_cursor_new_for_display(gdk_screen_get_display(screen), GDK_CROSSHAIR);
    }
      
  return cursor;
}

static void
grab_color_at_mouse (GdkScreen *screen,
		     gint       x_root,
		     gint       y_root,
		     gpointer   data)
{
  GdkPixbuf *pixbuf;
  guchar *pixels;
  gint n_channels;
  gint rowstride;
  ColorPicker *colorpicker = data;
  ColorPickerPrivate *priv;
  GdkWindow *root_window = gdk_screen_get_root_window (screen);
  
  priv = colorpicker->priv;

  pixbuf = gdk_pixbuf_get_from_window (root_window, x_root, y_root, 1, 1);
  if (!pixbuf)
      return;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  if (n_channels < 3)
    {
      g_object_unref (pixbuf);
      return;
    }
  priv->color.red = pixels[0] / 255.0;
  priv->color.green = pixels[1] / 255.0;
  priv->color.blue = pixels[2] / 255.0;
  priv->color.alpha = 1.0;

  (void)rowstride;
  g_object_unref (pixbuf);

  g_signal_emit (colorpicker, picker_signals[COLOR_CHANGED], 0);
}

static void
shutdown_eyedropper (ColorPicker *colorpicker)
{
	ColorPickerPrivate *priv;
	priv = colorpicker->priv;    

	if (priv->has_grab)
	{
		GdkDisplay *display = gtk_widget_get_display (priv->dropper_grab_widget);
        if (priv->grab_seat) {
            gdk_seat_ungrab(priv->grab_seat);
            priv->grab_seat = NULL;
        }
		gtk_grab_remove (priv->dropper_grab_widget);

		priv->has_grab = FALSE;
	}
}


static void
mouse_motion (GtkWidget      *invisible,
			  GdkEventMotion *event,
			  gpointer        data)
{
  grab_color_at_mouse (gdk_event_get_screen ((GdkEvent *)event),
		       event->x_root, event->y_root, data); 
}

static gboolean
mouse_release (GtkWidget      *invisible,
	   		   GdkEventButton *event,
	           gpointer        data)
{
	ColorPicker *colorpicker = COLOR_PICKER (data);

  if (event->button != 1)
    return FALSE;

  grab_color_at_mouse (gdk_event_get_screen ((GdkEvent *)event),
		       event->x_root, event->y_root, data);

  shutdown_eyedropper (colorpicker);
  
  g_signal_handlers_disconnect_by_func (invisible,
					mouse_motion,
					data);
  g_signal_handlers_disconnect_by_func (invisible,
					mouse_release,
					data);

	g_signal_emit (colorpicker, picker_signals[RELEASED], 0);

  return TRUE;
}

static gboolean
key_press (GtkWidget   *invisible,
           GdkEventKey *event,
           gpointer     data)
{  
  GdkDisplay *display = gtk_widget_get_display (invisible);
  GdkScreen *screen = gdk_event_get_screen ((GdkEvent *)event);
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();
  gint x, y;
  gint dx, dy;
  GdkDevice *device;
  GdkSeat *seat;

  seat = gdk_display_get_default_seat(display);
  device = gdk_seat_get_pointer(seat);
  gdk_device_get_position(device, NULL, &x, &y);

  dx = 0;
  dy = 0;

  switch (event->keyval) 
    {
    case GDK_KEY_space:
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_KP_Space:
      grab_color_at_mouse (screen, x, y, data);
      /* fall through */

    case GDK_KEY_Escape:
      shutdown_eyedropper (COLOR_PICKER (data));
      
      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);
	  g_signal_emit ( data, picker_signals[RELEASED], 0);
      
      return TRUE;

#if defined GDK_WINDOWING_X11 || defined GDK_WINDOWING_WIN32
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      dy = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      dy = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      dx = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      dx = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;
#endif

    default:
      return FALSE;
    }

  gdk_device_warp (device, screen, x + dx, y + dy);
  
  return TRUE;

}

static gboolean
mouse_press (GtkWidget      *invisible,
	     GdkEventButton *event,
	     gpointer        data)
{
  if (event->type == GDK_BUTTON_PRESS &&
      event->button == 1)
    {

      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (mouse_motion),
                        data);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (mouse_release),
                        data);
      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);
      return TRUE;
    }

  return FALSE;
}


/**
 * color_picker_new:
 * 
 * Creates a new ColorPicker.
 * 
 * Return value: a new #ColorPicker
 **/
ColorPicker *
color_picker_new (void)
{
	ColorPicker *colorpicker;

	colorpicker = g_object_new (COLOR_TYPE_PICKER, NULL);

	return colorpicker;
}

/**
 * color_picker_get_screen_color:
 * @colorpicker: a #ColorPicker.
 * @widget: a #GtkWidget .
 *
 * Get the color of the screen.
 * 
 **/
void
color_picker_get_screen_color (ColorPicker *colorpicker, GtkWidget *widget)
{
  ColorPickerPrivate *priv = colorpicker->priv;
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (widget));
  GdkCursor *picker_cursor;
  GdkGrabStatus grab_status;
  GtkWidget *grab_widget, *toplevel;
  // guint32 time = gtk_get_current_event_time ();
  GdkDisplay *display = gtk_widget_get_display(widget);
  GdkSeat *seat = gdk_display_get_default_seat(display);

	
  if ( priv->dropper_grab_widget == NULL )
  {
      grab_widget = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_screen (GTK_WINDOW (grab_widget), screen);
      gtk_window_resize (GTK_WINDOW (grab_widget), 1, 1);
      gtk_window_move (GTK_WINDOW (grab_widget), -100, -100);
      gtk_widget_show (grab_widget);

      gtk_widget_add_events (grab_widget,
                             GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
      
      toplevel = gtk_widget_get_toplevel ( widget );
  
      if (GTK_IS_WINDOW (toplevel))
      {
          GtkWindowGroup *group = gtk_window_get_group (GTK_WINDOW (toplevel));
          if (group != NULL)
              gtk_window_group_add_window (group, GTK_WINDOW (grab_widget));
      }

      priv->dropper_grab_widget = grab_widget;
   }

  picker_cursor = make_picker_cursor (screen);

  // Use gdk_seat_grab in GTK3
  grab_status = gdk_seat_grab(seat,
                              gtk_widget_get_window(priv->dropper_grab_widget),
                              GDK_SEAT_CAPABILITY_ALL,
                              TRUE,
                              picker_cursor,
                              NULL,
                              NULL,
                              NULL);

  g_object_unref (picker_cursor);
  
  if (grab_status != GDK_GRAB_SUCCESS)
  {
      return;
  }

  gtk_grab_add (priv->dropper_grab_widget);
  priv->grab_seat = seat;
  priv->has_grab = TRUE;
  
  g_signal_connect (priv->dropper_grab_widget, "grab-broken-event",
                    G_CALLBACK (color_picker_grab_broken), colorpicker);
  g_signal_connect (priv->dropper_grab_widget, "button-press-event",
                    G_CALLBACK (mouse_press), colorpicker);
  g_signal_connect (priv->dropper_grab_widget, "key-press-event",
                    G_CALLBACK (key_press), colorpicker);
}

GdkRGBA	*
color_picker_get_color (ColorPicker *colorpicker)
{
	ColorPickerPrivate *priv = colorpicker->priv;
	return &priv->color;
}
