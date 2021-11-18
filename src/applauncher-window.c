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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gio/gdesktopappinfo.h>

#include <gtk/gtk.h>

#include <math.h>

#include <gmenu-tree.h>

#include "panel-glib.h"
#include "applauncher-window.h"
#include "applauncher-indicator.h"
#include "applauncher-appitem.h"
#include "applauncher-directory-item.h"

#define	DEFAULT_GRID_X    4
#define	DEFAULT_GRID_Y    5
#define	DEFAULT_ICON_SIZE 48


static gboolean on_directory_item_enter_notify_event_cb (GtkWidget     *widget,
                                                         GdkEventFocus *event,
                                                         gpointer       data);

static gboolean on_directory_item_leave_notify_event_cb (GtkWidget     *widget,
                                                         GdkEventFocus *event,
                                                         gpointer       data);

static GSList *get_all_applications_from_dir (GMenuTreeDirectory  *directory,
                                              GSList              *list);


struct _ApplauncherWindowPrivate
{
	GtkWidget *grid;
	GtkWidget *ent_search;
	GtkWidget *stk_bottom;
	GtkWidget *lbx_dirs;
	GtkWidget *cur_dir_button;
	GtkWidget *event_box_appitem;

	GtkRadioButton *directory_group;

	GtkButton *selected_appitem;

	ApplauncherIndicator *pages;

	GSList *dirs;
	GSList *apps;
	GSList *cur_apps;
	GSList *filtered_apps;

	GList *grid_children;

	int grid_x;
	int grid_y;
	int icon_size;

	int x;
	int y;
	int width;
	int height;

	int init_popup_width;
	int init_popup_height;

	GdkRectangle workarea;

	gchar *filter_text;

	guint idle_entry_changed_id;
	guint idle_directory_changed_id;

	GdkDevice *grab_pointer;
};

enum {
	CLOSED,
	LAUNCH_DESKTOP,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (ApplauncherWindow, applauncher_window, GTK_TYPE_WINDOW)


static gboolean
find_entry (GSList *list, GMenuTreeEntry *entry)
{
	const gchar *application_name;
	if (entry) {
		GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));
		if (app_info) {
			application_name = g_app_info_get_name (app_info);
		} else {
			application_name = NULL;
		}
	} else {
		application_name = NULL;
	}

	if (!application_name) return FALSE;

	GSList *l = NULL;
	for (l = list; l; l = l->next) {
		GMenuTreeEntry *entry = (GMenuTreeEntry *)l->data;
		if (entry) {
			GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));
			if (app_info) {
				const gchar *name = g_app_info_get_name (app_info);
				return (g_str_equal (name, application_name));
			}
		}
	}

	return FALSE;
}

static gboolean
match_desktop (const gchar *desktop_id, const gchar *match_str)
{
	gboolean ret = FALSE;
	GDesktopAppInfo *dt_info = NULL;

	if (!desktop_id || !match_str || g_str_equal (match_str, ""))
		return FALSE;

	if (g_str_equal (desktop_id, match_str))
		return TRUE;

	dt_info = g_desktop_app_info_new (desktop_id);
	if (!dt_info)
		return FALSE;

	while (1) {
		gchar *name = NULL, *locale_name = NULL, *exec = NULL;

		name = g_desktop_app_info_get_string (dt_info, G_KEY_FILE_DESKTOP_KEY_NAME);
		if (name && ((g_utf8_collate (name, match_str) == 0) || (panel_g_utf8_strstrcase (name, match_str) != NULL))) {
			g_free (name);
			ret = TRUE;
			break;
		}

		locale_name = g_desktop_app_info_get_locale_string (dt_info, G_KEY_FILE_DESKTOP_KEY_NAME);
		if (locale_name && ((g_utf8_collate (locale_name, match_str) == 0) || (panel_g_utf8_strstrcase (locale_name, match_str) != NULL))) {
			g_free (locale_name);
			ret = TRUE;
			break;
		}

		exec = g_desktop_app_info_get_string (dt_info, G_KEY_FILE_DESKTOP_KEY_EXEC);
		if (exec && ((g_utf8_collate (exec, match_str) == 0) || (panel_g_utf8_strstrcase (exec, match_str) != NULL))) {
			g_free (exec);
			ret = TRUE;
			break;
		}

		break;
	}

	g_object_unref (dt_info);

	return ret;
}

gchar *
get_applications_menu (void)
{
	return g_strdup ("gnome-applications.menu");
}


/* Copied from gnome-panel-3.26.0/gnome-panel/panel-run-dialog.c:
 * get_all_applications_from_alias () */
static GSList *
get_all_applications_from_alias (GMenuTreeAlias *alias,
                                 GSList         *list)
{
	switch (gmenu_tree_alias_get_aliased_item_type (alias))
	{
		case GMENU_TREE_ITEM_ENTRY: {
			GMenuTreeEntry *entry = gmenu_tree_alias_get_aliased_entry (alias);
			list = g_slist_append (list, entry);
			break;
		}

		case GMENU_TREE_ITEM_DIRECTORY: {
			GMenuTreeDirectory *directory = gmenu_tree_alias_get_aliased_directory (alias);
			list = get_all_applications_from_dir (directory, list);
			gmenu_tree_item_unref (directory);
			break;
		}

		default:
			break;
	}

	return list;
}

/* Copied from gnome-panel-3.26.0/gnome-panel/panel-run-dialog.c:
 * get_all_applications_from_dir () */
static GSList *
get_all_applications_from_dir (GMenuTreeDirectory  *directory,
                               GSList              *list)
{
	GMenuTreeIter *iter;
	GMenuTreeItemType next_type;

	iter = gmenu_tree_directory_iter (directory);

	while ((next_type = gmenu_tree_iter_next (iter)) != GMENU_TREE_ITEM_INVALID) {
		switch (next_type) {
			case GMENU_TREE_ITEM_ENTRY: {
				GMenuTreeEntry *entry = gmenu_tree_iter_get_entry (iter);
				list = g_slist_append (list, entry);
				break;
			}

			case GMENU_TREE_ITEM_DIRECTORY: {
				GMenuTreeDirectory *dir = gmenu_tree_iter_get_directory (iter);
				list = get_all_applications_from_dir (dir, list);
				gmenu_tree_item_unref (dir);
				break;
			}

			case GMENU_TREE_ITEM_ALIAS: {
				GMenuTreeAlias *alias = gmenu_tree_iter_get_alias (iter);
				list = get_all_applications_from_alias (alias, list);
				gmenu_tree_item_unref (alias);
				break;
			}

			default:
			break;
		}
	}

	gmenu_tree_iter_unref (iter);

	return list;
}

static GSList *
get_all_directories (void)
{
	GMenuTree          *tree;
	GMenuTreeDirectory *root;
	GSList             *list = NULL;
	gchar *applications_menu = NULL;

	applications_menu = get_applications_menu ();

	tree = gmenu_tree_new (applications_menu, GMENU_TREE_FLAGS_SORT_DISPLAY_NAME);
	g_free (applications_menu);

	if (!gmenu_tree_load_sync (tree, NULL)) {
		g_object_unref (tree);
		return NULL;
	}

	root = gmenu_tree_get_root_directory (tree);

	list = g_slist_append (list, root);

	GMenuTreeIter *iter;
	GMenuTreeItemType next_type;

	iter = gmenu_tree_directory_iter (root);

	while ((next_type = gmenu_tree_iter_next (iter)) != GMENU_TREE_ITEM_INVALID) {
		switch (next_type) {
			case GMENU_TREE_ITEM_DIRECTORY:
			{
				list = g_slist_append (list, gmenu_tree_iter_get_directory (iter));
				break;
			}

			default:
				break;
		}
	}

	g_object_unref (tree);

	return list;
}

/* Copied from gnome-panel-3.26.0/gnome-panel/panel-run-dialog.c:
 * get_all_applications () */
static GSList *
get_all_applications (void)
{
	GMenuTree          *tree;
	GMenuTreeDirectory *root;
	GSList             *list = NULL;
	gchar *applications_menu = NULL;

	applications_menu = get_applications_menu ();

	tree = gmenu_tree_new (applications_menu, GMENU_TREE_FLAGS_SORT_DISPLAY_NAME);
	g_free (applications_menu);

	if (!gmenu_tree_load_sync (tree, NULL)) {
		g_object_unref (tree);
		return NULL;
	}

	root = gmenu_tree_get_root_directory (tree);

	list = get_all_applications_from_dir (root, NULL);

	gmenu_tree_item_unref (root);
	g_object_unref (tree);

	return list;
}

static void
apply_blacklist (void)
{
	gboolean has_exec_perm = TRUE;
	GList *all_apps = NULL, *l = NULL;

	all_apps = g_app_info_get_all ();

	for (l = all_apps; l; l = l->next) {
		GAppInfo *appinfo = G_APP_INFO (l->data);
		if (!appinfo)
			continue;

		const gchar *exec = g_app_info_get_executable (appinfo);

		if (exec) {
			gchar **argv;
			g_shell_parse_argv (exec, NULL, &argv, NULL);

			if (g_strv_length (argv) > 0) {
				gchar *cmd;
				GStatBuf stat_buf;

				cmd = g_find_program_in_path (argv[0]);

				if (cmd && g_stat (cmd, &stat_buf) == 0) {
					/* 블랙리스트 처리된 앱은 실행권한이 없음 */
					has_exec_perm = !(stat_buf.st_mode & S_IXOTH);
				}
				g_free (cmd);
			}
			g_strfreev (argv);
		}

		if (!has_exec_perm) {
			const gchar *full_dt_id = g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (appinfo));

			GKeyFile *keyfile = g_key_file_new ();

			/* 블랙리스트 처리된 데스크톱 파일을 저장해서 상태를 변경해야
             * libgnome-menu 에서 desktop entry 캐쉬를 업데이트한다.*/
			if (g_key_file_load_from_file (keyfile, full_dt_id,
                                           G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                           NULL)) {
				g_key_file_save_to_file (keyfile, full_dt_id, NULL);
			}

			g_key_file_free (keyfile);
		}
    }

	g_list_free_full (all_apps, g_object_unref);
}

static int
get_total_pages (ApplauncherWindow *window, GSList *list)
{
	ApplauncherWindowPrivate *priv = window->priv;

	guint size = 0;
	gint  num_pages = 0;

	size = g_slist_length (list);

	if (size > 0 && priv->grid_x > 0 && priv->grid_y > 0) {
		num_pages = (int)(size / (priv->grid_y * priv->grid_x));

		if ((size %  (priv->grid_y * priv->grid_x)) > 0) {
			num_pages += 1;
		}
	}

	return num_pages;
}

static void
update_pages (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	g_return_if_fail (priv->filtered_apps != NULL);

	guint size = 0;
	gint filtered_pages = 0;

	size = g_slist_length (priv->filtered_apps);
	filtered_pages = (int)(size / (priv->grid_y * priv->grid_x));

	if ((size %  (priv->grid_y * priv->grid_x)) > 0) {
		filtered_pages += 1;
	}

	// Update pages
	if (filtered_pages > 1) {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "indicator");
		GList *children = applauncher_indicator_get_children (priv->pages);
		guint total_pages = g_list_length (children);

		int p;
		for (p = 1; p <= total_pages; p++) {
			GtkWidget *child = g_list_nth_data (children, p - 1);
			if (child) {
				gboolean visible = (p > filtered_pages) ? FALSE : TRUE;
				gtk_widget_set_visible (child, visible);
			}
		}
	} else {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "fake");
	}
}

static void
update_grid (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	gint r, c;
	if (priv->filtered_apps == NULL) {
		for (r = 0; r < priv->grid_x; r++) {
			for (c = 0; c < priv->grid_y; c++) {
				gint pos = c + (r * priv->grid_y);
				ApplauncherAppItem *item = g_list_nth_data (priv->grid_children, pos);
				if (item) {
					gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
					applauncher_appitem_change_app (item, NULL, NULL, NULL);
				}
			}
		}
		return;
	}

	GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

	gint active = applauncher_indicator_get_active (priv->pages);
	gint item_iter = active * priv->grid_y * priv->grid_x;

	for (r = 0; r < priv->grid_x; r++) {
		for (c = 0; c < priv->grid_y; c++) {
			gint pos = c + (r * priv->grid_y); // position in table right now
			ApplauncherAppItem *item = g_list_nth_data (priv->grid_children, pos);
			gtk_widget_set_state_flags (GTK_WIDGET (item), GTK_STATE_FLAG_NORMAL, TRUE);
			if (item_iter < g_slist_length (priv->filtered_apps)) {
				GMenuTreeEntry *entry = g_slist_nth_data (priv->filtered_apps, item_iter);

				if (!entry) {
					item_iter++;
					gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
					continue;
				}

				GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));

				if (!app_info) {
					item_iter++;
					gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
					continue;
				}

				GIcon *icon = g_app_info_get_icon (app_info);
				const gchar *name = g_app_info_get_name (app_info);
				const gchar *desc = g_app_info_get_description (app_info);

				gtk_widget_set_sensitive (GTK_WIDGET (item), TRUE);
				if (desc == NULL || g_strcmp0 (desc, "") == 0) {
					applauncher_appitem_change_app (item, icon, name, name);
				} else {
					gchar *tooltip = g_strdup_printf ("%s:\n%s", name, desc);
					applauncher_appitem_change_app (item, icon, name, tooltip);
					g_free (tooltip);
				}
			} else { // fill with a blank one
				gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);
				applauncher_appitem_change_app (item, NULL, NULL, NULL);
			}

			item_iter++;
		}
	}

	// Update number of pages
	update_pages (window);
}

static void
pages_activate_cb (ApplauncherIndicator *indicator, gpointer data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);

	update_grid (window);
}

static void
do_search (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	GSList *l = NULL, *apps = NULL;
	for (l = priv->cur_apps; l; l = l->next) {
		GMenuTreeEntry *entry = (GMenuTreeEntry *)l->data;
		if (!entry) continue;

		GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));
		if (!app_info) continue;

		if (g_str_equal (priv->filter_text, "")) {
			if (!find_entry (apps, entry))
				apps = g_slist_append (apps, entry);
		} else {
			const gchar *id = g_app_info_get_id (app_info);
			if (match_desktop (id, priv->filter_text)) {
				if (!find_entry (apps, entry))
					apps = g_slist_append (apps, entry);

				continue;
			}
		}
	}

	g_slist_free (priv->filtered_apps);
	priv->filtered_apps = apps;

	int total_pages = get_total_pages (window, priv->filtered_apps);
	if (total_pages > 1) {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "indicator");
		applauncher_indicator_set_active (priv->pages, 0);
	} else {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "fake");
		update_grid (window);
	}
}

static void
grab_pointer (ApplauncherWindow *window)
{
	GdkSeat *seat;
	GdkDevice *device, *pointer;

	device = gtk_get_current_event_device ();

	if (!device) {
		GdkDisplay *display;

		display = gtk_widget_get_display (GTK_WIDGET (window));
		device = gdk_seat_get_pointer (gdk_display_get_default_seat (display));
	}

	if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
		window->priv->grab_pointer = gdk_device_get_associated_device (device);
	else
		window->priv->grab_pointer = device;

	gtk_widget_grab_focus (GTK_WIDGET (window));

	seat = gdk_device_get_seat (window->priv->grab_pointer);
	gdk_seat_grab (seat, gtk_widget_get_window (GTK_WIDGET (window)),
                          GDK_SEAT_CAPABILITY_ALL, TRUE,
                          NULL, NULL, NULL, NULL);
}

static void
ungrab_pointer (ApplauncherWindow *window)
{
	if (window->priv->grab_pointer) {
		gdk_seat_ungrab (gdk_device_get_seat (window->priv->grab_pointer));
		window->priv->grab_pointer = NULL;
	}
}

static void
on_appitem_button_clicked_cb (GtkButton *button, gpointer data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	priv->selected_appitem = button;

	gint index = g_list_index (priv->grid_children, button);

	if (index < 0)
		return;

	gint active = applauncher_indicator_get_active (priv->pages);
	gint pos = index + (active * priv->grid_y * priv->grid_x);

	GMenuTreeEntry *entry = g_slist_nth_data (priv->filtered_apps, pos);
	if (!entry)
		return;

	GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));
	const gchar *desktop_id = g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (app_info));

	g_signal_emit (G_OBJECT (window), signals[LAUNCH_DESKTOP], 0, desktop_id);
}

static void
search_entry_changed_idle_destroyed (gpointer data)
{
	APPLAUNCHER_WINDOW (data)->priv->idle_entry_changed_id = 0;
}

static gboolean
search_entry_changed_idle (gpointer data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	const gchar *text = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->ent_search)));

	g_free (priv->filter_text);
	priv->filter_text = (text == NULL) ? g_strdup ("") : g_strdup (text);

	do_search (window);

	return FALSE;
}

static void
on_search_entry_changed_cb (ApplauncherWindow *window)
{
	const gchar *text;
	ApplauncherWindowPrivate *priv = window->priv;

	if (priv->idle_entry_changed_id != 0) {
		g_source_remove (priv->idle_entry_changed_id);
		priv->idle_entry_changed_id = 0;
	}

	text = gtk_entry_get_text (GTK_ENTRY (priv->ent_search));

	g_clear_pointer (&priv->filter_text, g_free);
	priv->filter_text = (text == NULL) ? g_strdup ("") : g_strdup (text);

	priv->idle_entry_changed_id =
		gdk_threads_add_idle_full (G_PRIORITY_DEFAULT,
                                   search_entry_changed_idle,
                                   window,
                                   search_entry_changed_idle_destroyed);
}

static void
on_search_entry_preedit_changed_cb (GtkEntry *entry,
                                    gchar    *preedit,
                                    gpointer  data)
{
	const gchar *text;
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	if (priv->idle_entry_changed_id != 0) {
		g_source_remove (priv->idle_entry_changed_id);
		priv->idle_entry_changed_id = 0;
	}

	text = gtk_entry_get_text (entry);

	g_clear_pointer (&priv->filter_text, g_free);
	priv->filter_text = preedit ? g_strdup_printf ("%s%s", text, preedit) : g_strdup (text);

	priv->idle_entry_changed_id =
		gdk_threads_add_idle_full (G_PRIORITY_DEFAULT,
                                   search_entry_changed_idle,
                                   window,
                                   search_entry_changed_idle_destroyed);
}
static gboolean
search_entry_populate_popup_cb (GtkWidget *widget,
                                gpointer   data)
{
  return GDK_EVENT_STOP;
}

/* button press handler used to inhibit popup menu */
static gint
search_entry_button_press_cb (GtkWidget      *widget,
                              GdkEventButton *event)
{
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		return TRUE;
	}

	return FALSE;
}


static void
on_search_entry_activate_cb (GtkEditable *entry,
                             gpointer     data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	guint size = g_slist_length (priv->filtered_apps);
	if (size == 0) return;

	GtkWidget *focus = gtk_container_get_focus_child (GTK_CONTAINER (priv->grid));

	if (focus) {
		gtk_button_clicked (GTK_BUTTON (focus));
		return;
	}

	ApplauncherAppItem *item = g_list_nth_data (priv->grid_children, 0);

	gtk_widget_grab_focus (GTK_WIDGET (item));
	if (size == 1) {
		gtk_button_clicked (GTK_BUTTON (item));
	}
}

static void
on_search_entry_icon_release_cb (GtkEntry             *entry,
                                 GtkEntryIconPosition  icon_pos,
                                 GdkEvent             *event,
                                 gpointer              user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		gtk_entry_reset_im_context (GTK_ENTRY (entry));
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
}

static void
directory_changed_idle_destroyed (gpointer data)
{
	APPLAUNCHER_WINDOW (data)->priv->idle_directory_changed_id = 0;
}

static gboolean
directory_changed_idle (gpointer data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->cur_dir_button), TRUE);

	return FALSE;
}

static gboolean
on_directory_item_enter_notify_event_cb (GtkWidget     *widget,
                                         GdkEventFocus *event,
                                         gpointer       data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	priv->cur_dir_button = widget;

	if (priv->idle_directory_changed_id != 0) {
		g_source_remove (priv->idle_directory_changed_id);
		priv->idle_directory_changed_id = 0;
	}

	priv->idle_directory_changed_id =
		gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT,
                                      300,
                                      directory_changed_idle,
                                      window,
                                      directory_changed_idle_destroyed);

	return FALSE;
}

static gboolean
on_directory_item_leave_notify_event_cb (GtkWidget     *widget,
                                         GdkEventFocus *event,
                                         gpointer       data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	if (priv->idle_directory_changed_id != 0) {
		g_source_remove (priv->idle_directory_changed_id);
		priv->idle_directory_changed_id = 0;
	}

	return FALSE;
}

static void
on_directory_item_toggled_cb (GtkToggleButton *button,
                              gpointer         data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);
	ApplauncherWindowPrivate *priv = window->priv;

	gint pos = 0;
	GList *children, *l = NULL;
	GMenuTreeDirectory *cur_dir = NULL;

	if (priv->idle_directory_changed_id != 0) {
		g_source_remove (priv->idle_directory_changed_id);
		priv->idle_directory_changed_id = 0;
	}

	children = gtk_container_get_children (GTK_CONTAINER (priv->lbx_dirs));

	for (l = children; l; l = l->next) {
		GtkWidget *child = GTK_WIDGET (l->data);

		if (GTK_WIDGET (button) == GTK_WIDGET (l->data)) {
			cur_dir = g_slist_nth_data (priv->dirs, pos);
			break;
		}

		pos++;
	}

	if (cur_dir) {
		GSList *list = get_all_applications_from_dir (cur_dir, NULL);

		g_slist_free (priv->filtered_apps);
		g_slist_free (priv->cur_apps);
		priv->filtered_apps = NULL;
		priv->cur_apps = NULL;

		GSList *l = NULL;
		for (l = list; l; l = l->next) {
			if (!find_entry (priv->filtered_apps, l->data)) {
				priv->filtered_apps = g_slist_append (priv->filtered_apps, l->data);
				priv->cur_apps = g_slist_append (priv->cur_apps, l->data);
			}
		}

		int total_pages = get_total_pages (window, priv->filtered_apps);
		if (total_pages > 1) {
			gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "indicator");
			applauncher_indicator_set_active (priv->pages, 0);
		} else {
			gtk_stack_set_visible_child_name (GTK_STACK (priv->stk_bottom), "fake");
			update_grid (window);
		}
	}
}

static gint
get_max_size_of_appitem (ApplauncherWindow *window)
{
	GSList *l = NULL;
	gint max_item_size = 0, item_width = 0, item_height = 0;

	for (l = window->priv->apps; l; l = l->next) {
		GMenuTreeEntry *entry = (GMenuTreeEntry *)l->data;
		if (entry) {
			GAppInfo *app_info = G_APP_INFO (gmenu_tree_entry_get_app_info (entry));
			if (app_info) {
				GIcon *icon = g_app_info_get_icon (app_info);
				const gchar *name = g_app_info_get_name (app_info);

				ApplauncherAppItem *item = applauncher_appitem_new (window->priv->icon_size);
				applauncher_appitem_change_app (item, icon, name, NULL);
				gtk_widget_show (GTK_WIDGET (item));

				gtk_grid_attach (GTK_GRID (window->priv->grid), GTK_WIDGET (item), 0, 0, 1, 1);

				gint max = 0, pref_w = 0, pref_h = 0;
				gtk_widget_get_preferred_width (GTK_WIDGET (item), NULL, &pref_w);
				gtk_widget_get_preferred_height (GTK_WIDGET (item), NULL, &pref_h);

				max = (pref_w > pref_h) ? pref_w : pref_h;
				max_item_size = (max_item_size > max) ? max_item_size: max;

				gtk_widget_destroy (GTK_WIDGET (item));
			}
		}
	}

	return max_item_size;
}

static void
populate_dirs (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	GSList *l = NULL;
	for (l = priv->dirs; l; l = l->next) {
		GMenuTreeDirectory *dir = (GMenuTreeDirectory *)l->data;

		GIcon *icon;
		const char *name;
		if (gmenu_tree_directory_get_desktop_file_path (dir) != NULL) {
			icon = gmenu_tree_directory_get_icon (dir);
			name = gmenu_tree_directory_get_name (dir);
		} else {
			icon = icon = g_themed_icon_new ("applications-other");
			name = _("All Programs");
		}

		GSList *l = NULL;
		ApplauncherDirectoryItem *item = applauncher_directory_item_new (icon, name);
		if (priv->directory_group) {
			l = gtk_radio_button_get_group (priv->directory_group);
		} else {
			priv->cur_dir_button = GTK_WIDGET (item);
		}
		gtk_radio_button_set_group (GTK_RADIO_BUTTON (item), l);
		priv->directory_group = GTK_RADIO_BUTTON (item);

		gtk_widget_show (GTK_WIDGET (item));
		gtk_box_pack_start (GTK_BOX (priv->lbx_dirs), GTK_WIDGET (item), TRUE, FALSE, 0);

		g_signal_connect (G_OBJECT (item), "enter-notify-event", G_CALLBACK (on_directory_item_enter_notify_event_cb), window);
		g_signal_connect (G_OBJECT (item), "leave-notify-event", G_CALLBACK (on_directory_item_leave_notify_event_cb), window);
		g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (on_directory_item_toggled_cb), window);
	}
}

static void
get_rows_and_columns (ApplauncherWindow *window,
                      GdkRectangle      *workarea,
                      int                item_size,
                      int               *row,
                      int               *col)
{
	ApplauncherAppItem *item;
	ApplauncherWindowPrivate *priv = window->priv;

	int size = 0;
	int c = 0, r = 0;
	int popup_width, popup_height;
	int row_spacing, col_spacing;

	row_spacing = gtk_grid_get_row_spacing (GTK_GRID (priv->grid));
	col_spacing = gtk_grid_get_column_spacing (GTK_GRID (priv->grid));

#if 0
	size = item_size;
	for (i = 1; i < DEFAULT_GRID_Y; i++) {
		gtk_widget_set_size_request (priv->grid, size, -1);
		gtk_widget_get_preferred_width (GTK_WIDGET (window), NULL, &popup_width);

		if (popup_width >= priv->init_popup_width)
			break;

		size += (item_size + col_spacing);
	}

	for (c = i + 1; c < DEFAULT_GRID_Y; c++) {
		if (popup_width >= workarea->width)
			break;
		popup_width += (item_size + col_spacing);
	}

	if (col) *col = c;

	size = item_size;
	for (i = 0; i < DEFAULT_GRID_X; i++) {
		gtk_widget_set_size_request (priv->grid, -1, size);
		gtk_widget_get_preferred_height (GTK_WIDGET (window), NULL, &popup_height);

		if (popup_height >= priv->init_popup_height)
			break;

		size += (item_size + row_spacing);
	}

	for (r = i + 1; r < DEFAULT_GRID_X; r++) {
		if (popup_height >= workarea->height)
			break;
		popup_height += (item_size + row_spacing);
	}

	if (row) *row = r;

	gtk_widget_set_size_request (priv->grid, 0, 0);
#endif

	size = item_size;
	for (c = 0; c < DEFAULT_GRID_Y; c++) {
		gtk_widget_set_size_request (priv->grid, size, -1);
		gtk_widget_get_preferred_width (GTK_WIDGET (window), NULL, &popup_width);

		if (popup_width >= workarea->width)
			break;

		size += (item_size + col_spacing);
	}

	size = item_size;
	for (r = 0; r < DEFAULT_GRID_X; r++) {
		gtk_widget_set_size_request (priv->grid, -1, size);
		gtk_widget_get_preferred_height (GTK_WIDGET (window), NULL, &popup_height);

		if (popup_height >= workarea->height)
			break;

		size += (item_size + row_spacing);
	}
	if (col) *col = c;
	if (row) *row = r;

	gtk_widget_set_size_request (priv->grid, 0, 0);
}

static void
populate_apps (ApplauncherWindow *window, GdkRectangle *workarea)
{
	GList *l = NULL;
	int r, c, item_size;

	ApplauncherWindowPrivate *priv = window->priv;

	for (l = priv->grid_children; l; l = l->next) {
		GtkWidget *item = GTK_WIDGET (l->data);
		if (item) {
			g_signal_handlers_disconnect_by_func (item, on_appitem_button_clicked_cb, window);
			gtk_widget_destroy (GTK_WIDGET (item));
			item = NULL;
		}
	}
	g_list_free (priv->grid_children);
	priv->grid_children = NULL;

	// get max size of items
	item_size = get_max_size_of_appitem (window);

	if (workarea) {
		gint grid_x = 0, grid_y = 0;

		get_rows_and_columns (window, workarea, item_size, &grid_x, &grid_y);

		priv->grid_x = grid_x;
		priv->grid_y = grid_y;
	}

	for (r = 0; r < priv->grid_x; r++) {
		for (c = 0; c < priv->grid_y; c++) {
			ApplauncherAppItem *item = applauncher_appitem_new (priv->icon_size);
			gtk_widget_set_size_request (GTK_WIDGET (item), item_size, item_size);

			gtk_grid_attach (GTK_GRID (priv->grid), GTK_WIDGET (item), c, r, 1, 1);

			priv->grid_children = g_list_append (priv->grid_children, item);

			g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (on_appitem_button_clicked_cb), window);
		}
	}
}

static void
applauncher_window_page_left (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	gint active = applauncher_indicator_get_active (priv->pages);

	if (active >= 1) {
		applauncher_indicator_set_active (priv->pages, active - 1);
	}
}

static void
applauncher_window_page_right (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv = window->priv;

	gint total_pages = get_total_pages (window, priv->filtered_apps);
	gint active = applauncher_indicator_get_active (priv->pages);

	if ((active + 1) < total_pages) {
		applauncher_indicator_set_active (priv->pages, active + 1);
	}
}

static gboolean
applauncher_window_scroll (GtkWidget      *widget,
                           GdkEventScroll *event,
                           gpointer        data)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (data);

	if (event->direction == GDK_SCROLL_UP) {
		applauncher_window_page_left (window);
	} else if (event->direction == GDK_SCROLL_DOWN) {
		applauncher_window_page_right (window);
	} else {
		return FALSE;
	}

	return TRUE;
}

static void
applauncher_window_select_all_programs (ApplauncherWindow *window)
{
	on_directory_item_toggled_cb (GTK_TOGGLE_BUTTON (window->priv->cur_dir_button), window);
}

static gboolean
applauncher_window_configure_event (GtkWidget         *widget,
                                    GdkEventConfigure *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

	if (event->width && event->height) {
		priv->x = event->x;
		priv->y = event->y;
		priv->width = event->width;
		priv->height = event->height;
	}

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->configure_event (widget, event);
}

static gboolean
applauncher_window_key_press_event (GtkWidget   *widget,
                                    GdkEventKey *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);

	if (event->keyval == GDK_KEY_Escape) {
		ungrab_pointer (window);
		g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, APPLAUNCHER_WINDOW_CLOSED);
		return TRUE;
	}

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->key_press_event (widget, event);
}

static gint
applauncher_window_button_release_event (GtkWidget      *widget,
                                         GdkEventButton *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

	GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

	if (GTK_IS_WINDOW (toplevel)) {
		GtkWidget *focus = gtk_window_get_focus (GTK_WINDOW (toplevel));

		if (focus && gtk_widget_is_ancestor (focus, widget))
			return gtk_widget_event (focus, (GdkEvent*) event);
	}

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->button_release_event (widget, event);
}

static gint
applauncher_window_button_press_event (GtkWidget      *widget,
                                       GdkEventButton *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

	// destroy window if user clicks outside
	if ((event->x_root <= priv->x) ||
        (event->y_root <= priv->y) ||
        (event->x_root >= priv->x + priv->width) ||
        (event->y_root >= priv->y + priv->height))
	{
		ungrab_pointer (window);
		g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, APPLAUNCHER_WINDOW_CLOSED);
	} else {
		gtk_widget_grab_focus (GTK_WIDGET (priv->ent_search));
	}

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->button_press_event (widget, event);
}

static void
applauncher_window_realize (GtkWidget *widget)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

	gtk_widget_get_preferred_width (widget, NULL, &priv->init_popup_width);
	gtk_widget_get_preferred_height (widget, NULL, &priv->init_popup_height);

	applauncher_window_reload_apps (window, &priv->workarea);

	if (GTK_WIDGET_CLASS (applauncher_window_parent_class)->realize) {
		GTK_WIDGET_CLASS (applauncher_window_parent_class)->realize (widget);
	}
}

static gboolean
applauncher_window_map_event (GtkWidget   *widget,
                              GdkEventAny *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

//	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);

	grab_pointer (window);

    // Focus search entry
	gtk_widget_grab_focus (GTK_WIDGET (priv->ent_search));

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->map_event (widget, event);
}

static gboolean
applauncher_window_focus_out_event (GtkWidget     *widget,
                                    GdkEventFocus *event)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (widget);
	ApplauncherWindowPrivate *priv = window->priv;

	ungrab_pointer (window);

	g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, APPLAUNCHER_WINDOW_CLOSED);

	return GTK_WIDGET_CLASS (applauncher_window_parent_class)->focus_out_event (widget, event);
}

static void
applauncher_window_init (ApplauncherWindow *window)
{
	ApplauncherWindowPrivate *priv;

	priv = window->priv = applauncher_window_get_instance_private (window);
	gtk_widget_init_template (GTK_WIDGET (window));

	priv->directory_group = NULL;
	priv->dirs = NULL;
	priv->apps = NULL;
	priv->filtered_apps = NULL;
	priv->grid_children = NULL;
	priv->cur_apps = NULL;
	priv->selected_appitem = NULL;
	priv->filter_text = NULL;
	priv->idle_entry_changed_id = 0;
	priv->idle_directory_changed_id = 0;
	priv->grid_x = DEFAULT_GRID_X;
	priv->grid_y = DEFAULT_GRID_Y;
	priv->icon_size = DEFAULT_ICON_SIZE;
	priv->grab_pointer = NULL;

	GdkMonitor *m = gdk_display_get_primary_monitor (gdk_display_get_default ());
	gdk_monitor_get_geometry (m, &priv->workarea);

	gtk_window_stick (GTK_WINDOW (window));
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

	GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (window));
	if(gdk_screen_is_composited(screen)) {
		GdkVisual *visual = gdk_screen_get_rgba_visual (screen);
		if (visual == NULL)
			visual = gdk_screen_get_system_visual (screen);

		gtk_widget_set_visual (GTK_WIDGET(window), visual);
	}

	apply_blacklist ();

	priv->dirs = get_all_directories ();
	priv->apps = get_all_applications ();

	populate_dirs (window);

	priv->pages = applauncher_indicator_new ();
	gtk_stack_add_named (GTK_STACK (priv->stk_bottom), GTK_WIDGET (priv->pages), "indicator");
	/* 최초 윈도우 너비를 고려하여 1개만 삽입한다. */
	applauncher_indicator_append (priv->pages);

	g_signal_connect (G_OBJECT (priv->pages), "child-activate",
                      G_CALLBACK (pages_activate_cb), window);

	g_signal_connect_swapped (G_OBJECT (priv->ent_search), "changed",
                              G_CALLBACK (on_search_entry_changed_cb), window);

	g_signal_connect (G_OBJECT (priv->ent_search), "preedit-changed",
                      G_CALLBACK (on_search_entry_preedit_changed_cb), window);

	g_signal_connect (G_OBJECT (priv->ent_search), "icon-release",
                      G_CALLBACK (on_search_entry_icon_release_cb), window);

	g_signal_connect (G_OBJECT (priv->ent_search), "activate",
                      G_CALLBACK (on_search_entry_activate_cb), window);

	g_signal_connect (G_OBJECT (priv->ent_search), "popup-menu",
                      G_CALLBACK (search_entry_populate_popup_cb), window);

	/* button press handler used to inhibit popup menu */
	g_signal_connect (G_OBJECT (priv->ent_search), "button_press_event",
                      G_CALLBACK (search_entry_button_press_cb), window);

	g_signal_connect (G_OBJECT (priv->event_box_appitem), "scroll-event",
                      G_CALLBACK (applauncher_window_scroll), window);

	gtk_widget_add_events (priv->event_box_appitem, GDK_SCROLL_MASK);
}

static void
applauncher_window_finalize (GObject *object)
{
	ApplauncherWindow *window = APPLAUNCHER_WINDOW (object);
	ApplauncherWindowPrivate *priv = window->priv;

	g_slist_free_full (priv->dirs, (GDestroyNotify)gmenu_tree_item_unref);
	g_slist_free_full (priv->apps, (GDestroyNotify)gmenu_tree_item_unref);
	g_slist_free_full (priv->cur_apps, (GDestroyNotify)gmenu_tree_item_unref);
	g_slist_free (priv->filtered_apps);

	g_list_free (priv->grid_children);

	if (priv->idle_entry_changed_id != 0) {
		g_source_remove (priv->idle_entry_changed_id);
		priv->idle_entry_changed_id = 0;
	}

	if (priv->idle_directory_changed_id != 0) {
		g_source_remove (priv->idle_directory_changed_id);
		priv->idle_directory_changed_id = 0;
	}

	G_OBJECT_CLASS (applauncher_window_parent_class)->finalize (object);
}

static void
applauncher_window_class_init (ApplauncherWindowClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = applauncher_window_finalize;
	widget_class->focus_out_event = applauncher_window_focus_out_event;
	widget_class->realize = applauncher_window_realize;
	widget_class->map_event = applauncher_window_map_event;
	widget_class->button_press_event = applauncher_window_button_press_event;
	widget_class->button_release_event = applauncher_window_button_release_event;
	widget_class->configure_event = applauncher_window_configure_event;
	widget_class->key_press_event = applauncher_window_key_press_event;

    signals[CLOSED] = g_signal_new ("closed",
                                    WINDOW_TYPE_APPLAUNCHER,
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET(ApplauncherWindowClass,
                                    closed),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__INT,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            WINDOW_TYPE_APPLAUNCHER,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(ApplauncherWindowClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);

	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
			"/kr/gooroom/applauncher/ui/applauncher-window.ui");

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherWindow, ent_search);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherWindow, grid);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherWindow, stk_bottom);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherWindow, lbx_dirs);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), ApplauncherWindow, event_box_appitem);
}

ApplauncherWindow *
applauncher_window_new (GtkWidget *parent)
{
	GtkWidget *toplevel = gtk_widget_get_toplevel (parent);

	return g_object_new (WINDOW_TYPE_APPLAUNCHER,
                         "type", GTK_WINDOW_TOPLEVEL,
                         "type-hint", GDK_WINDOW_TYPE_HINT_POPUP_MENU,
                         "transient-for", toplevel,
                         "modal", TRUE,
                         NULL);
}

void
applauncher_window_reload_apps (ApplauncherWindow *window,
                                GdkRectangle      *workarea)
{
	g_return_if_fail (workarea != NULL);

	ApplauncherWindowPrivate *priv = window->priv;

	populate_apps (window, workarea);

	applauncher_indicator_reset (priv->pages);

	int p = 0;
	int total_pages = get_total_pages (window, priv->apps);
	for (p = 0; p < total_pages; p++) {
		applauncher_indicator_append (priv->pages);
	}

	applauncher_window_select_all_programs (window);
}

void
applauncher_window_set_workarea (ApplauncherWindow *window,
                                 GdkRectangle      *workarea)
{
	g_return_if_fail (workarea != NULL);

	ApplauncherWindowPrivate *priv = window->priv;

	priv->workarea.x = workarea->x;
	priv->workarea.y = workarea->y;
	priv->workarea.width = workarea->width;
	priv->workarea.height = workarea->height;
}
