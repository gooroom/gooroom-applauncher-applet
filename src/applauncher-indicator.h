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


#ifndef __APPLAUNCHER_INDICATOR_H__
#define __APPLAUNCHER_INDICATOR_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS  

#define APPLAUNCHER_TYPE_INDICATOR           (applauncher_indicator_get_type ())
#define APPLAUNCHER_INDICATOR(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), APPLAUNCHER_TYPE_INDICATOR, ApplauncherIndicator))
#define APPLAUNCHER_INDICATOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), APPLAUNCHER_TYPE_INDICATOR, ApplauncherIndicatorClass))
#define GTK_IS_INDICATOR(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APPLAUNCHER_TYPE_INDICATOR))
#define GTK_IS_INDICATOR_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), APPLAUNCHER_TYPE_INDICATOR))
#define APPLAUNCHER_INDICATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), APPLAUNCHER_TYPE_INDICATOR, ApplauncherIndicatorClass))

typedef struct _ApplauncherIndicator              ApplauncherIndicator;
typedef struct _ApplauncherIndicatorPrivate       ApplauncherIndicatorPrivate;
typedef struct _ApplauncherIndicatorClass         ApplauncherIndicatorClass;


struct _ApplauncherIndicatorClass
{
	GtkBoxClass __parent_class__;

	void (*child_activate) (ApplauncherIndicator *indicator);
};

struct _ApplauncherIndicator
{
	GtkBox __parent__;

	ApplauncherIndicatorPrivate *priv;
};

GType                 applauncher_indicator_get_type                (void) G_GNUC_CONST;
ApplauncherIndicator *applauncher_indicator_new                     (void);

GList                *applauncher_indicator_get_children            (ApplauncherIndicator *indicator);

void                  applauncher_indicator_append                  (ApplauncherIndicator *indicator);

void                  applauncher_indicator_reset                   (ApplauncherIndicator *indicator);

gint                  applauncher_indicator_get_active              (ApplauncherIndicator *indicator);

void                  applauncher_indicator_set_active              (ApplauncherIndicator *indicator,
                                                                     gint                  index);

void                  applauncher_indicator_set_active_no_signal    (ApplauncherIndicator *indicator,
                                                                     gint                  index);

G_END_DECLS

#endif /* __APPLAUNCHER_INDICATOR_H__ */
