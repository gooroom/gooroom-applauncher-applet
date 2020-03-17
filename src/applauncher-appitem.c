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

	g_object_set (item, "has-tooltip", TRUE, NULL);

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
	gchar *new_name, *new_tooltip;

	ApplauncherAppItemPrivate *priv = item->priv;

	// Icon
	gtk_image_set_from_gicon (GTK_IMAGE (priv->icon), icon, GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->icon), priv->icon_size);

	new_name = name ? g_strdup (name) : g_strdup ("");
	new_tooltip = tooltip ? g_strdup (tooltip) : g_strdup ("");

	memset (buf, 0x00, sizeof (buf));
	size = g_utf8_strlen (name, -1);

	if (size > MAX_CHARACTER) {
		g_utf8_strncpy (buf, name, MAX_CHARACTER);
	} else {
		g_utf8_strncpy (buf, name, size);
	}

	// Label
	gtk_label_set_text (GTK_LABEL (priv->label), buf);

	memset (buf, 0x00, sizeof (buf));
	size = g_utf8_strlen (new_tooltip, -1);
	if (size > MAX_CHARACTER) {
		g_utf8_strncpy (buf, new_tooltip, MAX_CHARACTER);
	} else {
		g_utf8_strncpy (buf, new_tooltip, size);
	}

	// Tooltip
	gtk_label_set_text (GTK_LABEL (priv->tooltip), buf);

	g_free (new_name);
	g_free (new_tooltip);
}
