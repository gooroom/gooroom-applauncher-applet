/*
 *  Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
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


#ifndef __APPLAUNCHER_DIRECTORY_ITEM_H__
#define __APPLAUNCHER_DIRECTORY_ITEM_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define APPLAUNCHER_TYPE_DIRECTORY_ITEM            (applauncher_directory_item_get_type ())
#define APPLAUNCHER_DIRECTORY_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APPLAUNCHER_TYPE_DIRECTORY_ITEM, ApplauncherDirectoryItem))
#define APPLAUNCHER_DIRECTORY_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APPLAUNCHER_TYPE_DIRECTORY_ITEM, ApplauncherDirectoryItemClass))
#define APPLAUNCHER_IS_DIRECTORY_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APPLAUNCHER_TYPE_DIRECTORY_ITEM))
#define APPLAUNCHER_IS_DIRECTORY_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APPLAUNCHER_TYPE_DIRECTORY_ITEM))
#define APPLAUNCHER_DIRECTORY_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), APPLAUNCHER_TYPE_DIRECTORY_ITEM, ApplauncherDirectoryItemClass))

typedef struct _ApplauncherDirectoryItemPrivate ApplauncherDirectoryItemPrivate;
typedef struct _ApplauncherDirectoryItemClass   ApplauncherDirectoryItemClass;
typedef struct _ApplauncherDirectoryItem        ApplauncherDirectoryItem;

struct _ApplauncherDirectoryItemClass
{
	GtkRadioButtonClass __parent_class__;
};

struct _ApplauncherDirectoryItem
{
	GtkRadioButton __parent__;

	ApplauncherDirectoryItemPrivate *priv;
};


GType                     applauncher_directory_item_get_type   (void) G_GNUC_CONST;

ApplauncherDirectoryItem *applauncher_directory_item_new        (GIcon      *icon,
                                                                 const char *name);

void                      applauncher_directory_item_update     (ApplauncherDirectoryItem *item,
                                                                 GIcon                    *icon,
                                                                 const char               *name);



G_END_DECLS

#endif /* !__APPLAUNCHER_DIRECTORY_ITEM_H__ */
