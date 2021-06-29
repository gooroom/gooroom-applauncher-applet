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

#include <glib.h>
#include <gtk/gtk.h>

#include <math.h>
#include <string.h>

#include "applauncher-appitem.h"


#define	MAX_LINES     4
#define	MAX_CHARACTER 128

struct _ApplauncherAppItemPrivate
{
	GtkWidget *appitem_inner_box;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *tooltip;

	int icon_size;
};



G_DEFINE_TYPE_WITH_PRIVATE (ApplauncherAppItem, applauncher_appitem, GTK_TYPE_BUTTON);


static gboolean
query_tooltip_cb (GtkWidget  *widget,
                  gint        x,
                  gint        y,
                  gboolean    keyboard_tip,
                  GtkTooltip *tooltip,
                  gpointer    data)
{
	GtkWidget *custom = GTK_WIDGET (data);

	gtk_tooltip_set_custom (tooltip, custom);

	return TRUE;
}

static void
applauncher_appitem_finalize (GObject *object)
{
	ApplauncherAppItemPrivate *priv = APPLAUNCHER_APPITEM (object)->priv;

	g_object_unref (priv->tooltip);

	(*G_OBJECT_CLASS (applauncher_appitem_parent_class)->finalize) (object);
}

static void
applauncher_appitem_init (ApplauncherAppItem *item)
{
	ApplauncherAppItemPrivate *priv;

	priv = item->priv = applauncher_appitem_get_instance_private (item);

	gtk_widget_init_template (GTK_WIDGET (item));

	priv->tooltip = gtk_label_new ("");
	gtk_label_set_width_chars (GTK_LABEL (priv->tooltip), -1);
	gtk_label_set_lines (GTK_LABEL (priv->tooltip), MAX_LINES);
	gtk_label_set_line_wrap (GTK_LABEL (priv->tooltip), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (priv->tooltip), MAX_CHARACTER/MAX_LINES);
	gtk_label_set_ellipsize (GTK_LABEL (priv->tooltip), PANGO_ELLIPSIZE_END);
	g_object_ref_sink (priv->tooltip);

	g_signal_connect (item, "query-tooltip",
                      G_CALLBACK (query_tooltip_cb), priv->tooltip);
}

static void
applauncher_appitem_class_init (ApplauncherAppItemClass *klass)
{
	GObjectClass   *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = applauncher_appitem_finalize;

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
	glong size;
	gchar buf[1024] = {0,};

	ApplauncherAppItemPrivate *priv = item->priv;

	// Icon
	if (icon) {
		gtk_image_set_from_gicon (GTK_IMAGE (priv->icon), icon, GTK_ICON_SIZE_BUTTON);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->icon), priv->icon_size);
	} else {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), NULL, GTK_ICON_SIZE_BUTTON);
	}

	memset (buf, 0x00, sizeof (buf));
	if (name) {
		size = g_utf8_strlen (name, -1);
		if (size > MAX_CHARACTER) {
			g_utf8_strncpy (buf, name, MAX_CHARACTER);
		} else {
			g_utf8_strncpy (buf, name, size);
		}
	} else {
		g_utf8_strncpy (buf, "", 1);
	}
	// Label
	gtk_label_set_text (GTK_LABEL (priv->label), buf);

	memset (buf, 0x00, sizeof (buf));
	if (tooltip) {
		g_object_set (item, "has-tooltip", TRUE, NULL);
		size = g_utf8_strlen (tooltip, -1);
		if (size > MAX_CHARACTER) {
			g_utf8_strncpy (buf, tooltip, MAX_CHARACTER);
		} else {
			g_utf8_strncpy (buf, tooltip, size);
		}
	} else {
		g_utf8_strncpy (buf, "", 1);
		g_object_set (item, "has-tooltip", FALSE, NULL);
	}
	// Tooltip
	gtk_label_set_text (GTK_LABEL (priv->tooltip), buf);
}
