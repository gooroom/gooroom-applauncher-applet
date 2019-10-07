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

#include <glib.h>
#include <gtk/gtk.h>

#include <string.h>

#include "applauncher-directory-item.h"



struct _ApplauncherDirectoryItemPrivate
{
	GtkWidget *icon;
	GtkWidget *label;
};



G_DEFINE_TYPE_WITH_PRIVATE (ApplauncherDirectoryItem, applauncher_directory_item, GTK_TYPE_RADIO_BUTTON);



static void
applauncher_directory_item_init (ApplauncherDirectoryItem *item)
{
	ApplauncherDirectoryItemPrivate *priv;

	priv = item->priv = applauncher_directory_item_get_instance_private (item);

	gtk_widget_init_template (GTK_WIDGET (item));
}

static void
applauncher_directory_item_class_init (ApplauncherDirectoryItemClass *klass)
{
	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                             "/kr/gooroom/applauncher/ui/directory-item.ui");

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherDirectoryItem, icon);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherDirectoryItem, label);
}

ApplauncherDirectoryItem *
applauncher_directory_item_new (GIcon *icon, const char *name)
{
	ApplauncherDirectoryItem *item;

	item = g_object_new (APPLAUNCHER_TYPE_DIRECTORY_ITEM, NULL);

	applauncher_directory_item_update (item, icon, name);

	return item;
}

void
applauncher_directory_item_update (ApplauncherDirectoryItem *item,
                                   GIcon                    *icon,
                                   const char               *name)
{
	ApplauncherDirectoryItemPrivate *priv = item->priv;

	// Icon
	gtk_image_set_from_gicon (GTK_IMAGE (priv->icon), icon, GTK_ICON_SIZE_BUTTON);

	// Label
	gtk_label_set_text (GTK_LABEL (priv->label), name);
}
