/* Eye of Gnome image viewer - main eog_window widget
 *
 * Copyright (C) 2000 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *         Jens Finke <jens@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef EOG_WINDOW_H
#define EOG_WINDOW_H

#include <bonobo.h>

G_BEGIN_DECLS 

#define EOG_TYPE_WINDOW            (eog_window_get_type ())
#define EOG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_WINDOW, EogWindow))
#define EOG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EOG_TYPE_WINDOW, EogWindowClass))
#define EOG_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_WINDOW))
#define EOG_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_WINDOW))
#define EOG_WINDOW_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_WINDOW, EogWindowClass))


typedef struct _EogWindow         EogWindow;
typedef struct _EogWindowClass    EogWindowClass;

typedef struct _EogWindowPrivate  EogWindowPrivate;


struct _EogWindow {
	BonoboWindow win;

	/* Private data */
	EogWindowPrivate *priv;
};

struct _EogWindowClass {
	BonoboWindowClass parent_class;
};


GType        eog_window_get_type                    (void);
GtkWidget *eog_window_new (void);
void eog_window_construct (EogWindow *eog_window);

void eog_window_close (EogWindow *eog_window);

void eog_window_open_dialog (EogWindow *eog_window);
gboolean eog_window_open (EogWindow *eog_window, const char *text_uri);
gboolean eog_window_open_list (EogWindow *eog_window, GList *text_uri_list);

void eog_window_set_auto_size (EogWindow *eog_window, gboolean bool);
gboolean eog_window_get_auto_size (EogWindow *eog_window);

const char *eog_window_get_uri (EogWindow *eog_window);

Bonobo_PropertyControl eog_window_get_property_control (EogWindow *eog_window,
							CORBA_Environment *ev);

GList *eog_get_window_list (void);

void eog_window_close_all (void);

G_END_DECLS

#endif
