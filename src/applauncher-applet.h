/* 
 *  Copyright (C) 2018-2019 Gooroom <gooroom@gooroom.kr>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __GOOROOM_APPLAUNCHER_APPLET_H__
#define __GOOROOM_APPLAUNCHER_APPLET_H__

G_BEGIN_DECLS

#include <libgnome-panel/gp-applet.h>

#define GOOROOM_TYPE_APPLAUNCHER_APPLET            (gooroom_applauncher_applet_get_type ())
#define GOOROOM_APPLAUNCHER_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOOROOM_TYPE_APPLAUNCHER_APPLET, GooroomApplauncherApplet))
#define GOOROOM_APPLAUNCHER_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GOOROOM_TYPE_APPLAUNCHER_APPLET, GooroomApplauncherAppletClass))
#define GOOROOM_IS_APPLAUNCHER_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOOROOM_TYPE_APPLAUNCHER_APPLET))
#define GOOROOM_IS_APPLAUNCHER_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GOOROOM_TYPE_APPLAUNCHER_APPLET))
#define GOOROOM_APPLAUNCHER_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GOOROOM_TYPE_APPLAUNCHER_APPLET, GooroomApplauncherAppletClass))

typedef struct _GooroomApplauncherApplet        GooroomApplauncherApplet;
typedef struct _GooroomApplauncherAppletClass   GooroomApplauncherAppletClass;
typedef struct _GooroomApplauncherAppletPrivate GooroomApplauncherAppletPrivate;

struct _GooroomApplauncherApplet {
    GpApplet                         parent;
    GooroomApplauncherAppletPrivate *priv;
};

struct _GooroomApplauncherAppletClass {
    GpAppletClass                    parent_class;
};

GType gooroom_applauncher_applet_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* !__GOOROOM_APPLAUNCHER_APPLET_H__ */
