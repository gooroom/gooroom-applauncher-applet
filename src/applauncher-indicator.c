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

#include <stdlib.h>

#include "applauncher-indicator.h"



struct _ApplauncherIndicatorPrivate
{
	GtkWidget *indicator_group;

	GList *children;

	gint active;
};

enum
{
  CHILD_ACTIVATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


G_DEFINE_TYPE_WITH_PRIVATE (ApplauncherIndicator, applauncher_indicator, GTK_TYPE_BOX)



static void
indicator_item_selection_changed_cb (GtkToggleButton *button,
                                     gpointer         data)
{
	ApplauncherIndicator *indicator = APPLAUNCHER_INDICATOR (data);
	ApplauncherIndicatorPrivate *priv = indicator->priv;

	if (gtk_toggle_button_get_active (button)) {
		gint idx = g_list_index (priv->children, button);

		applauncher_indicator_set_active (indicator, idx);
	}
}

static void
applauncher_indicator_finalize (GObject *object)
{
	ApplauncherIndicatorPrivate *priv = APPLAUNCHER_INDICATOR (object)->priv;

	g_list_free (priv->children);

	(*G_OBJECT_CLASS (applauncher_indicator_parent_class)->finalize) (object);
}

static void
applauncher_indicator_init (ApplauncherIndicator *indicator)
{
	ApplauncherIndicatorPrivate *priv;

	priv = indicator->priv = applauncher_indicator_get_instance_private (indicator);

	gtk_widget_init_template (GTK_WIDGET (indicator));

	priv->active = 0;
}

static void
applauncher_indicator_class_init (ApplauncherIndicatorClass *klass)
{
	GObjectClass   *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = applauncher_indicator_finalize;

	signals[CHILD_ACTIVATE] =
    g_signal_new (g_intern_static_string ("child-activate"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ApplauncherIndicatorClass, child_activate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);


	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                  "/kr/gooroom/applauncher/ui/indicator.ui");
}

ApplauncherIndicator *
applauncher_indicator_new (void)
{
	return g_object_new (APPLAUNCHER_TYPE_INDICATOR, NULL);
}

gint
applauncher_indicator_get_active (ApplauncherIndicator *indicator)
{
	g_return_val_if_fail (indicator != NULL, -1);

	ApplauncherIndicatorPrivate *priv = indicator->priv;

	return priv->active;
}

void
applauncher_indicator_set_active (ApplauncherIndicator *indicator,
                                  gint                  index)
{
	g_return_if_fail (indicator != NULL);

	ApplauncherIndicatorPrivate *priv = indicator->priv;

	GtkWidget *w = g_list_nth_data (priv->children, index);

	g_signal_handlers_block_by_func (G_OBJECT (w),
                         G_CALLBACK (indicator_item_selection_changed_cb),
                         indicator);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);

	g_signal_handlers_unblock_by_func (G_OBJECT (w),
                         G_CALLBACK (indicator_item_selection_changed_cb),
                         indicator);

	priv->active = index;

	g_signal_emit (G_OBJECT (indicator), signals[CHILD_ACTIVATE], 0);
}

GList *
applauncher_indicator_get_children (ApplauncherIndicator *indicator)
{
	g_return_val_if_fail (indicator != NULL, NULL);

	ApplauncherIndicatorPrivate *priv = indicator->priv;
	
	return priv->children;
}

void
applauncher_indicator_append (ApplauncherIndicator *indicator)
{
	g_return_if_fail (indicator != NULL);

	ApplauncherIndicatorPrivate *priv = indicator->priv;

	GtkWidget *item = NULL, *icon;

	if (priv->indicator_group) {
		item = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->indicator_group));
	} else {
		item = gtk_radio_button_new (NULL);
		priv->indicator_group = item;
	}
	gtk_widget_set_name (item, "indicator-button");
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (item), FALSE);
	icon = gtk_image_new_from_icon_name ("radio-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (icon), 16);
	gtk_container_add (GTK_CONTAINER (item), icon);

	gtk_box_pack_start (GTK_BOX (indicator), item, FALSE, FALSE, 0);
	gtk_widget_show_all (item);

	g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (indicator_item_selection_changed_cb), indicator);

	priv->children = g_list_append (priv->children, item);
}

void
applauncher_indicator_reset (ApplauncherIndicator *indicator)
{
	GList *l = NULL;
	ApplauncherIndicatorPrivate *priv = indicator->priv;

	for (l = priv->children; l; l = l->next) {
		GtkWidget *item = GTK_WIDGET (l->data);
		if (item) {
			g_signal_handlers_disconnect_by_func (item, indicator_item_selection_changed_cb, indicator);
			gtk_widget_destroy (GTK_WIDGET (item));
		}
	}
	g_list_free (priv->children);
	priv->children = NULL;
	priv->indicator_group = NULL;
	priv->active = 0;
}
