/*-
 * Copyright (C) 2018-2019 Gooroom <gooroom@gooroom.kr>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __APPLAUNCHER_WINDOW_H__
#define __APPLAUNCHER_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WINDOW_TYPE_APPLAUNCHER            (applauncher_window_get_type ())
#define APPLAUNCHER_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_TYPE_APPLAUNCHER, ApplauncherWindow))
#define APPLAUNCHER_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_TYPE_APPLAUNCHER, ApplauncherWindowClass))
#define WINDOW_IS_APPLAUNCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_TYPE_APPLAUNCHER))
#define WINDOW_IS_APPLAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_TYPE_APPLAUNCHER))
#define APPLAUNCHER_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_TYPE_APPLAUNCHER, ApplauncherWindowClass))

typedef struct _ApplauncherWindowPrivate  ApplauncherWindowPrivate;
typedef struct _ApplauncherWindowClass    ApplauncherWindowClass;
typedef struct _ApplauncherWindow         ApplauncherWindow;

enum {
	APPLAUNCHER_WINDOW_CLOSED = 1
};


struct _ApplauncherWindowClass
{
	GtkWindowClass __parent__;

    /*< signals >*/
	void (*closed)(ApplauncherWindow *window, gint reason);

	void (*launch_desktop)(ApplauncherWindow *window, const gchar *desktop);
};

struct _ApplauncherWindow
{
	GtkWindow __parent__;

	ApplauncherWindowPrivate *priv;
};


GType              applauncher_window_get_type (void) G_GNUC_CONST;

ApplauncherWindow *applauncher_window_new             (GtkWidget *parent);

void               applauncher_window_reload_apps     (ApplauncherWindow *window,
                                                       GdkRectangle      *workarea);

void               applauncher_window_set_workarea    (ApplauncherWindow *window,
                                                       GdkRectangle      *workarea);


G_END_DECLS

#endif /* !__APPLAUNCHER_WINDOW_H__ */
