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

#include <glib.h>
#include <gtk/gtk.h>

#include <math.h>
#include <string.h>

#include "applauncher-appitem.h"


struct _ApplauncherAppItemPrivate
{
	GtkWidget *appitem_inner_box;
	GtkWidget *icon;
	GtkWidget *label;

	int icon_size;
};



G_DEFINE_TYPE_WITH_PRIVATE (ApplauncherAppItem, applauncher_appitem, GTK_TYPE_BUTTON);


static void
applauncher_appitem_init (ApplauncherAppItem *item)
{
	GdkRectangle area;
	GdkMonitor *primary;
	int box_spacing = 15;
	int top = 10, end = 10, bottom = 10, start = 10;
	ApplauncherAppItemPrivate *priv;

	priv = item->priv = applauncher_appitem_get_instance_private (item);

	gtk_widget_init_template (GTK_WIDGET (item));

	primary = gdk_display_get_primary_monitor (gdk_display_get_default ());
	gdk_monitor_get_geometry (primary, &area);

	while (1) {
		if (area.height <= 540) {
			top = 0, end = 0, bottom = 0, start = 0;
			break;
		}
		if (area.height <= 720) {
			box_spacing = 10;
			break;
		}
		if (area.height <= 768) {
			box_spacing = 10;
			break;
		}
		break;
	}

	gtk_box_set_spacing (GTK_BOX (priv->appitem_inner_box), box_spacing);

	gtk_widget_set_margin_top (priv->appitem_inner_box, top);
	gtk_widget_set_margin_end (priv->appitem_inner_box, end);
	gtk_widget_set_margin_bottom (priv->appitem_inner_box, bottom);
	gtk_widget_set_margin_start (priv->appitem_inner_box, start);
}

static void
applauncher_appitem_class_init (ApplauncherAppItemClass *klass)
{
	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                  "/kr/gooroom/applauncher/ui/appitem.ui");

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherAppItem, appitem_inner_box);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherAppItem, icon);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherAppItem, label);
}

ApplauncherAppItem *
applauncher_appitem_new (int size)
{
	ApplauncherAppItem *item;

	item = g_object_new (APPLAUNCHER_TYPE_APPITEM, NULL);

	item->priv->icon_size = size;

	return item;
}

void
applauncher_appitem_change_app (ApplauncherAppItem *item,
                                GIcon              *icon,
                                const gchar        *name,
                                const gchar        *tooltip)
{
	ApplauncherAppItemPrivate *priv = item->priv;

	// Icon
	gtk_image_set_from_gicon (GTK_IMAGE (priv->icon), icon, GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->icon), priv->icon_size);

	// Label
	gtk_label_set_text (GTK_LABEL (priv->label), name);

	// Tooltip
	gtk_widget_set_tooltip_text (GTK_WIDGET (item), tooltip);
}
