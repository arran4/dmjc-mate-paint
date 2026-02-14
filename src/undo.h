/***************************************************************************
 *            undo.h
 *
 *  Sat Jan 23 14:05:22 2010
 *  Copyright  2010  Rogério Ferro do Nascimento
 *  <rogerioferro@gmail.com>
 ****************************************************************************/

#include <gtk/gtk.h>
#include "toolbar.h"


void undo_create_mask   ( gint        width, 
                          gint        height, 
                          cairo_surface_t   **mask,
                          cairo_t       **cr_mask );

void undo_add           ( GdkRectangle  *rect, 
                          cairo_surface_t     *mask,
                          cairo_surface_t     *background,
                          gp_tool_enum  tool );

void undo_add_resize    ( gint width, gint height );
void undo_clear          ( void );


/* GUI CallBack */
void on_menu_undo_activate		( GtkMenuItem *menuitem, gpointer user_data );
void on_menu_redo_activate		( GtkMenuItem *menuitem, gpointer user_data );
