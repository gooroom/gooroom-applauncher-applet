/*
 *  Copyright (C) 2018-2021 Gooroom <gooroom@gooroom.kr>
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


#ifndef __APPLAUNCHER_APPITEM_H__
#define __APPLAUNCHER_APPITEM_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define APPLAUNCHER_TYPE_APPITEM            (applauncher_appitem_get_type ())
#define APPLAUNCHER_APPITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APPLAUNCHER_TYPE_APPITEM, ApplauncherAppItem))
#define APPLAUNCHER_APPITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APPLAUNCHER_TYPE_APPITEM, ApplauncherAppItemClass))
#define APPLAUNCHER_IS_APPITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APPLAUNCHER_TYPE_APPITEM))
#define APPLAUNCHER_IS_APPITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APPLAUNCHER_TYPE_APPITEM))
#define APPLAUNCHER_APPITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), APPLAUNCHER_TYPE_APPITEM, ApplauncherAppItemClass))

typedef struct _ApplauncherAppItemPrivate ApplauncherAppItemPrivate;
typedef struct _ApplauncherAppItemClass   ApplauncherAppItemClass;
typedef struct _ApplauncherAppItem        ApplauncherAppItem;

struct _ApplauncherAppItemClass
{
	GtkButtonClass __parent_class__;
};

struct _ApplauncherAppItem
{
	GtkButton __parent__;

	ApplauncherAppItemPrivate *priv;
};


GType               applauncher_appitem_get_type   (void) G_GNUC_CONST;

ApplauncherAppItem *applauncher_appitem_new        (int size);

void                applauncher_appitem_change_app (ApplauncherAppItem *item,
                                                    GIcon              *icon,
                                                    const gchar        *name,
                                                    const gchar        *tooltip);



G_END_DECLS

#endif /* !__APPLAUNCHER_APPITEM_H__ */
