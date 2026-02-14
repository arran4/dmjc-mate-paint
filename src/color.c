/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * color.c
 * Copyright (C) Rogério Ferro do Nascimento 2009 <rogerioferro@gmail.com>
 * 
 * color.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "color.h"
#include "cv_drawing.h"
#include <glib/gi18n.h>

#define PALETTE_SIZE 28

/* Private Data */
static GdkRGBA background_color	=	{ 1.0, 1.0, 1.0, 1.0 };
static GdkRGBA foreground_color	=	{ 0.0, 0.0, 0.0, 1.0 };
static GdkRGBA palette_colors[PALETTE_SIZE]	=
{
	{ 0.0, 0.0, 0.0, 1.0 },
	{ 0.5, 0.5, 0.5, 1.0 },
	{ 0.5, 0.0, 0.0, 1.0 },
	{ 0.5, 0.5, 0.0, 1.0 },
	{ 0.0, 0.5, 0.0, 1.0 },
	{ 0.0, 0.5, 0.5, 1.0 },
	{ 0.0, 0.0, 0.5, 1.0 },
	{ 0.5, 0.0, 0.5, 1.0 },
	{ 0.5, 0.5, 0.25, 1.0 },
	{ 0.0, 0.25, 0.25, 1.0 },
	{ 0.0, 0.5, 1.0, 1.0 },
	{ 0.0, 0.25, 0.5, 1.0 },
	{ 0.5, 0.0, 1.0, 1.0 },
	{ 0.5, 0.25, 0.0, 1.0 },
	{ 1.0, 1.0, 1.0, 1.0 },
	{ 0.75, 0.75, 0.75, 1.0 },
	{ 1.0, 0.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0, 1.0 },
	{ 0.0, 1.0, 1.0, 1.0 },
	{ 0.0, 0.0, 1.0, 1.0 },
	{ 1.0, 0.0, 1.0, 1.0 },
	{ 1.0, 1.0, 0.5, 1.0 },
	{ 0.0, 1.0, 0.5, 1.0 },
	{ 0.5, 1.0, 1.0, 1.0 },
	{ 0.5, 0.5, 1.0, 1.0 },
	{ 1.0, 0.0, 0.5, 1.0 },
	{ 1.0, 0.5, 0.25, 1.0 },
};

static GtkWidget	*background_widget = NULL;
static GtkWidget	*foreground_widget = NULL;
static GtkWidget	*pallete_widgets[PALETTE_SIZE];

/* Private functions */
static void background_show ( void );
static void foreground_show ( void );
static void pallete_show    ( gint palette );

/* CODE */

void
foreground_set_color ( GdkRGBA *color )
{
	foreground_color = *color;
	foreground_show ();
}

void
foreground_set_color_from_rgb ( guint color )
{
	/* Unused, keeping signature but implementing empty or adapting if needed.
       This function seems to take packed int.
    */
    GdkRGBA rgba;
    rgba.red = ((color >> 16) & 0xFF) / 255.0;
    rgba.green = ((color >> 8) & 0xFF) / 255.0;
    rgba.blue = (color & 0xFF) / 255.0;
    rgba.alpha = 1.0;
    foreground_set_color(&rgba);
}

void
on_color_palette_entry_realize (GtkWidget *widget, gpointer user_data)
{
    gchar *name = (gchar *)user_data;
    gint index = atoi(name);
    if (index >= 0 && index < PALETTE_SIZE) {
        pallete_widgets[index] = widget;
        pallete_show(index);
    }
}

void
on_background_color_picker_realize (GtkWidget *widget, gpointer user_data)
{
	background_widget	=	widget;
	background_show ();
}

void
on_foreground_color_picker_realize (GtkWidget *widget, gpointer user_data)
{
	foreground_widget	=	widget;
	foreground_show ();
}

gboolean
on_background_color_picker_button_release_event (   GtkWidget	   *widget,
													GdkEventButton *event,
													gpointer       user_data )
{
    GtkWidget *dialog;
    gint res;

    dialog = gtk_color_chooser_dialog_new (_("Background Color"), GTK_WINDOW(gtk_widget_get_toplevel(widget)));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &background_color);

    res = gtk_dialog_run (GTK_DIALOG (dialog));
    if (res == GTK_RESPONSE_OK)
    {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &background_color);
        background_show ();
    }
    gtk_widget_destroy (dialog);
	return TRUE;
}

gboolean
on_foreground_color_picker_button_release_event (   GtkWidget	   *widget,
													GdkEventButton *event,
													gpointer       user_data )
{
    GtkWidget *dialog;
    gint res;

    dialog = gtk_color_chooser_dialog_new (_("Foreground Color"), GTK_WINDOW(gtk_widget_get_toplevel(widget)));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &foreground_color);

    res = gtk_dialog_run (GTK_DIALOG (dialog));
    if (res == GTK_RESPONSE_OK)
    {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &foreground_color);
        foreground_show ();
    }
    gtk_widget_destroy (dialog);
	return TRUE;
}

gboolean
on_color_palette_entry_button_press_event (   GtkWidget	   *widget,
											  GdkEventButton *event,
			                                  gpointer       user_data )
{
    gchar *name = (gchar *)user_data;
    gint index = atoi(name);

	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == 1 ) /* Left Button */
		{
            foreground_color = palette_colors[index];
            foreground_show ();
		}
		else
        if ( event->button == 3 ) /* Right Button */
		{
            background_color = palette_colors[index];
            background_show ();
		}
	}
    else if ( event->type == GDK_2BUTTON_PRESS )
    {
        GtkWidget *dialog;
        gint res;

        dialog = gtk_color_chooser_dialog_new (_("Edit Palette Color"), GTK_WINDOW(gtk_widget_get_toplevel(widget)));
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &palette_colors[index]);

        res = gtk_dialog_run (GTK_DIALOG (dialog));
        if (res == GTK_RESPONSE_OK)
        {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &palette_colors[index]);
            pallete_show (index);
        }
        gtk_widget_destroy (dialog);
    }
	return TRUE;
}

/* Private functions */
static void
background_show ( void )
{
    // gtk_widget_override_background_color is deprecated but still works in many themes, or use CSS provider.
    // For now, let's stick with the override or use CSS if needed.
    // However, GtkDrawingArea (which color pickers likely are) background setting might need CSS.
    // Or if they are event boxes.

    /* Using CSS provider to set background color for GTK3 */
    GtkStyleContext *context = gtk_widget_get_style_context(background_widget);
    GtkCssProvider *provider = gtk_css_provider_new();
    gchar *css_str = g_strdup_printf("widget { background-color: rgba(%d, %d, %d, %f); }",
                                     (int)(background_color.red * 255),
                                     (int)(background_color.green * 255),
                                     (int)(background_color.blue * 255),
                                     background_color.alpha);
    gtk_css_provider_load_from_data(provider, css_str, -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_free(css_str);
    g_object_unref(provider);

	cv_set_color_bg ( &background_color );
}

static void
foreground_show ( void )
{
    GtkStyleContext *context = gtk_widget_get_style_context(foreground_widget);
    GtkCssProvider *provider = gtk_css_provider_new();
    gchar *css_str = g_strdup_printf("widget { background-color: rgba(%d, %d, %d, %f); }",
                                     (int)(foreground_color.red * 255),
                                     (int)(foreground_color.green * 255),
                                     (int)(foreground_color.blue * 255),
                                     foreground_color.alpha);
    gtk_css_provider_load_from_data(provider, css_str, -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_free(css_str);
    g_object_unref(provider);

	cv_set_color_fg ( &foreground_color );
}

static void
pallete_show ( gint palette )
{
    if (pallete_widgets[palette]) {
        GtkStyleContext *context = gtk_widget_get_style_context(pallete_widgets[palette]);
        GtkCssProvider *provider = gtk_css_provider_new();
        gchar *css_str = g_strdup_printf("widget { background-color: rgba(%d, %d, %d, %f); }",
                                         (int)(palette_colors[palette].red * 255),
                                         (int)(palette_colors[palette].green * 255),
                                         (int)(palette_colors[palette].blue * 255),
                                         palette_colors[palette].alpha);
        gtk_css_provider_load_from_data(provider, css_str, -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_free(css_str);
        g_object_unref(provider);
    }
}
