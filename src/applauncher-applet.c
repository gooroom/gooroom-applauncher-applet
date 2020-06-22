/*
 *  Copyright (c) 2009 Brian Tarricone <brian@terricone.org>
 *  Copyright (C) 1999 Olivier Fourdan <fourdan@xfce.org>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <X11/Xatom.h>
#include <libsn/sn.h>
//#include <keybinder.h>

#include <panel-applet.h>

#include "panel-glib.h"
#include "applauncher-window.h"
#include "applauncher-applet.h"


#define TRAY_ICON_SIZE             (24)
#define XFCE_SPAWN_STARTUP_TIMEOUT (30)


struct _GooroomApplauncherAppletPrivate
{
	GtkWidget         *button;

	ApplauncherWindow *popup_window;
};

typedef struct
{
  /* startup notification data */
  SnLauncherContext *sn_launcher;
  guint              timeout_id;

  /* child watch data */
  guint              watch_id;
  GPid               pid;
  GClosure          *closure;
} XfceSpawnData;


G_DEFINE_TYPE_WITH_PRIVATE (GooroomApplauncherApplet, gooroom_applauncher_applet, PANEL_TYPE_APPLET)


static void
show_error_dialog (GtkWindow *parent,
                   GdkScreen *screen,
                   const char *primary_text)
{
	gchar *msg = NULL;
	GtkWidget *dialog;

	if (!primary_text) {
		msg = g_markup_printf_escaped ("An Error occurred while trying to launch application");
		primary_text = msg;
	}

	dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", primary_text);
	if (screen)
		gtk_window_set_screen (GTK_WINDOW (dialog), screen);

	if (!parent)
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Application Launching Error"));
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (G_OBJECT (dialog), "response",
                              G_CALLBACK (gtk_widget_destroy), G_OBJECT (dialog));

	if (msg) g_free (msg);
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_timeout () */
static gboolean
xfce_spawn_startup_timeout (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GTimeVal       now;
  gdouble        elapsed;
  glong          tv_sec;
  glong          tv_usec;

  g_return_val_if_fail (spawn_data->sn_launcher != NULL, FALSE);

  /* determine the amount of elapsed time */
  g_get_current_time (&now);
  sn_launcher_context_get_last_active_time (spawn_data->sn_launcher, &tv_sec, &tv_usec);
  elapsed = now.tv_sec - tv_sec + ((gdouble) (now.tv_usec - tv_usec) / G_USEC_PER_SEC);

  return elapsed < XFCE_SPAWN_STARTUP_TIMEOUT;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_timeout_destroy () */
static void
xfce_spawn_startup_timeout_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GPid           pid;

  spawn_data->timeout_id = 0;

  if (G_LIKELY (spawn_data->sn_launcher != NULL))
   {
     /* abort the startup notification */
     sn_launcher_context_complete (spawn_data->sn_launcher);
     sn_launcher_context_unref (spawn_data->sn_launcher);
     spawn_data->sn_launcher = NULL;
   }

  /* if there is no closure to watch the child, also stop
   * the child watch */
  if (G_LIKELY (spawn_data->closure == NULL
                && spawn_data->watch_id != 0))
    {
      pid = spawn_data->pid;
      g_source_remove (spawn_data->watch_id);
      g_child_watch_add (pid,
                         (GChildWatchFunc) (void (*)(void)) g_spawn_close_pid,
                         NULL);
    }
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_watch () */
static void
xfce_spawn_startup_watch (GPid     pid,
                          gint     status,
                          gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GValue         instance_and_params[2] = { { 0, }, { 0, } };

  g_return_if_fail (spawn_data->pid == pid);

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      /* xfce spawn has no instance */
      g_value_init (&instance_and_params[0], G_TYPE_POINTER);
      g_value_set_pointer (&instance_and_params[0], NULL);

      g_value_init (&instance_and_params[1], G_TYPE_INT);
      g_value_set_int (&instance_and_params[1], status);

      g_closure_set_marshal (spawn_data->closure, g_cclosure_marshal_VOID__INT);

      g_closure_invoke (spawn_data->closure, NULL,
                        2, instance_and_params, NULL);
    }

  /* don't leave zombies */
  g_spawn_close_pid (pid);
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_watch_destroy () */
static void
xfce_spawn_startup_watch_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;

  spawn_data->watch_id = 0;

  if (spawn_data->timeout_id != 0)
    g_source_remove (spawn_data->timeout_id);

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      g_closure_invalidate (spawn_data->closure);
      g_closure_unref (spawn_data->closure);
    }

  g_slice_free (XfceSpawnData, spawn_data);
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_get_active_workspace_number () */
static gint
xfce_spawn_get_active_workspace_number (GdkScreen *screen)
{
  GdkWindow *root;
  gulong     bytes_after_ret = 0;
  gulong     nitems_ret = 0;
  guint     *prop_ret = NULL;
  Atom       _NET_CURRENT_DESKTOP;
  Atom       _WIN_WORKSPACE;
  Atom       type_ret = None;
  gint       format_ret;
  gint       ws_num = 0;

  gdk_error_trap_push ();

  root = gdk_screen_get_root_window (screen);

  /* determine the X atom values */
  _NET_CURRENT_DESKTOP = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_NET_CURRENT_DESKTOP", False);
  _WIN_WORKSPACE = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_WIN_WORKSPACE", False);

  if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                          gdk_x11_get_default_root_xwindow(),
                          _NET_CURRENT_DESKTOP, 0, 32, False, XA_CARDINAL,
                          &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                          (gpointer) &prop_ret) != Success)
    {
      if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                              gdk_x11_get_default_root_xwindow(),
                              _WIN_WORKSPACE, 0, 32, False, XA_CARDINAL,
                              &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                              (gpointer) &prop_ret) != Success)
        {
          if (G_UNLIKELY (prop_ret != NULL))
            {
              XFree (prop_ret);
              prop_ret = NULL;
            }
        }
    }

  if (G_LIKELY (prop_ret != NULL))
    {
      if (G_LIKELY (type_ret != None && format_ret != 0))
        ws_num = *prop_ret;
      XFree (prop_ret);
    }

  gdk_error_trap_pop_ignored ();

  return ws_num;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_on_screen_with_child_watch () */
static gboolean
xfce_spawn_on_screen_with_child_watch (GdkScreen    *screen,
                                       const gchar  *working_directory,
                                       gchar       **argv,
                                       gchar       **envp,
                                       GSpawnFlags   flags,
                                       gboolean      startup_notify,
                                       guint32       startup_timestamp,
                                       const gchar  *startup_icon_name,
                                       GClosure     *child_watch_closure,
                                       GError      **error)
{
  gboolean            succeed;
  gchar             **cenvp;
  guint               n;
  guint               n_cenvp;
  gchar              *display_name;
  GPid                pid;
  XfceSpawnData      *spawn_data;
  SnLauncherContext  *sn_launcher = NULL;
  SnDisplay          *sn_display = NULL;
  gint                sn_workspace;
  const gchar        *startup_id;
  const gchar        *prgname;

  g_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail ((flags & G_SPAWN_DO_NOT_REAP_CHILD) == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* setup the child environment (stripping $DESKTOP_STARTUP_ID and $DISPLAY) */
  if (G_LIKELY (envp == NULL))
    envp = g_get_environ ();

  for (n = 0; envp[n] != NULL; ++n);
    cenvp = g_new0 (gchar *, n + 3);
  for (n_cenvp = n = 0; envp[n] != NULL; ++n)
    {
      if (strncmp (envp[n], "DESKTOP_STARTUP_ID", 18) != 0
          && strncmp (envp[n], "DISPLAY", 7) != 0)
        cenvp[n_cenvp++] = g_strdup (envp[n]);
    }

  /* add the real display name for the screen */
  display_name = gdk_screen_make_display_name (screen);
  cenvp[n_cenvp++] = g_strconcat ("DISPLAY=", display_name, NULL);
  g_free (display_name);

  /* initialize the sn launcher context */
  if (G_LIKELY (startup_notify))
    {
      sn_display = sn_display_new (GDK_SCREEN_XDISPLAY (screen),
                                   (SnDisplayErrorTrapPush) (void (*)(void)) gdk_error_trap_push,
                                   (SnDisplayErrorTrapPop) (void (*)(void)) gdk_error_trap_pop);

      if (G_LIKELY (sn_display != NULL))
        {
          sn_launcher = sn_launcher_context_new (sn_display, GDK_SCREEN_XNUMBER (screen));
          if (G_LIKELY (sn_launcher != NULL))
            {
              /* initiate the sn launcher context */
              sn_workspace = xfce_spawn_get_active_workspace_number (screen);
              sn_launcher_context_set_workspace (sn_launcher, sn_workspace);
              sn_launcher_context_set_binary_name (sn_launcher, argv[0]);
              sn_launcher_context_set_icon_name (sn_launcher, startup_icon_name != NULL ?
                                                 startup_icon_name : "applications-other");

              if (G_LIKELY (!sn_launcher_context_get_initiated (sn_launcher)))
                {
                  prgname = g_get_prgname ();
                  sn_launcher_context_initiate (sn_launcher, prgname != NULL ?
                                                prgname : "unknown", argv[0],
                                                startup_timestamp);
                }

              /* add the real startup id to the child environment */
              startup_id = sn_launcher_context_get_startup_id (sn_launcher);
              if (G_LIKELY (startup_id != NULL))
                cenvp[n_cenvp++] = g_strconcat ("DESKTOP_STARTUP_ID=", startup_id, NULL);
            }
        }
    }

  /* watch the child process */
  flags |= G_SPAWN_DO_NOT_REAP_CHILD;

  /* test if the working directory exists */
  if (working_directory == NULL || *working_directory == '\0')
    {
      /* not worth a warning */
      working_directory = NULL;
    }
  else if (!g_file_test (working_directory, G_FILE_TEST_IS_DIR))
    {
      /* print warning for user */
      g_printerr (_("Working directory \"%s\" does not exist. It won't be used "
                    "when spawning \"%s\"."), working_directory, *argv);
      working_directory = NULL;
    }

  /* try to spawn the new process */
  succeed = g_spawn_async (working_directory, argv, cenvp, flags, NULL,
                           NULL, &pid, error);

  g_strfreev (cenvp);

  if (G_LIKELY (succeed))
    {
      /* setup data to watch the child */
      spawn_data = g_slice_new0 (XfceSpawnData);
      spawn_data->pid = pid;
      if (child_watch_closure != NULL)
        {
          spawn_data->closure = g_closure_ref (child_watch_closure);
          g_closure_sink (spawn_data->closure);
        }

      spawn_data->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                                     xfce_spawn_startup_watch,
                                                     spawn_data,
                                                     xfce_spawn_startup_watch_destroy);

      if (G_LIKELY (sn_launcher != NULL))
        {
          /* start a timeout to stop the startup notification sequence after
           * a certain about of time, to handle applications that do not
           * properly implement startup notify */
          spawn_data->sn_launcher = sn_launcher;
          spawn_data->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_LOW,
                                                               XFCE_SPAWN_STARTUP_TIMEOUT,
                                                               xfce_spawn_startup_timeout,
                                                               spawn_data,
                                                               xfce_spawn_startup_timeout_destroy);
        }
    }
  else
    {
      if (G_LIKELY (sn_launcher != NULL))
        {
          /* abort the startup notification sequence */
          sn_launcher_context_complete (sn_launcher);
          sn_launcher_context_unref (sn_launcher);
        }
    }

  /* release the sn display */
  if (G_LIKELY (sn_display != NULL))
    sn_display_unref (sn_display);

  return succeed;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_on_screen () */
static gboolean
xfce_spawn_on_screen (GdkScreen    *screen,
                      const gchar  *working_directory,
                      gchar       **argv,
                      gchar       **envp,
                      GSpawnFlags   flags,
                      gboolean      startup_notify,
                      guint32       startup_timestamp,
                      const gchar  *startup_icon_name,
                      GError      **error)
{
  return xfce_spawn_on_screen_with_child_watch (screen, working_directory, argv,
                                                envp, flags, startup_notify,
                                                startup_timestamp, startup_icon_name,
                                                NULL, error);
}

static void
get_monitor_geometry (GooroomApplauncherApplet *applet,
                      GdkRectangle             *geometry)
{
	GdkDisplay *d;
	GdkWindow  *w;
	GdkMonitor *m;

	d = gdk_display_get_default ();
	w = gtk_widget_get_window (GTK_WIDGET (applet));
	m = gdk_display_get_monitor_at_window (d, w);

	gdk_monitor_get_geometry (m, geometry);
}

static void
get_workarea (GooroomApplauncherApplet *applet, GdkRectangle *workarea)
{
	GtkOrientation orientation;
	gint applet_width = 0, applet_height = 0;

	get_monitor_geometry (applet, workarea);

	gtk_widget_get_preferred_width (GTK_WIDGET (applet), NULL, &applet_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (applet), NULL, &applet_height);

	orientation = panel_applet_get_gtk_orientation (PANEL_APPLET (applet));

	switch (orientation) {
		case PANEL_APPLET_ORIENT_DOWN:
			workarea->y += applet_height;
			workarea->height -= applet_height;
		break;

		case PANEL_APPLET_ORIENT_UP:
			workarea->height -= applet_height;
		break;

		case PANEL_APPLET_ORIENT_RIGHT:
			workarea->x += applet_width;
			workarea->width -= applet_width;
		break;

		case PANEL_APPLET_ORIENT_LEFT:
			workarea->width -= applet_width;
		break;

		default:
			g_assert_not_reached ();
	}
}

static gchar *
desktop_working_directory_get (const gchar *id)
{
	gchar *wd = NULL;

	GDesktopAppInfo *dt_info = g_desktop_app_info_new (id);
	if (dt_info) {
		wd = g_desktop_app_info_get_string (dt_info, G_KEY_FILE_DESKTOP_KEY_PATH);
	}

	return wd;
}

static gboolean
launch_command (GdkScreen  *screen, const char *command, const char *workding_directory)
{
	gboolean    result = FALSE;
	GError     *error = NULL;
	char      **argv;
	int         argc;

	const gchar *s;
	GString *string = g_string_sized_new (256);

	for (s = command; *s; ++s) {
		if (*s == '%') {
			switch (*++s) {
				case '%':
					g_string_append_c (string, '%');
					break;
			}
		} else {
			g_string_append_c (string, *s);
		}
	}

	if (g_shell_parse_argv (string->str, &argc, &argv, NULL)) {
		result = xfce_spawn_on_screen (screen, workding_directory,
                                       argv, NULL, G_SPAWN_SEARCH_PATH,
                                       TRUE, gtk_get_current_event_time (),
                                       NULL, &error);
	}

	if (!result || error) {
		if (error) {
			g_warning ("Failed to launch application : %s", error->message);
			g_error_free (error);
			result = FALSE;
		}
	}

	g_string_free (string, TRUE);
	g_strfreev (argv);

	return result;
}

static gboolean
launch_desktop_id (const char  *desktop_id, GdkScreen *screen)
{
	gboolean retval;
	GError *error = NULL;
	GDesktopAppInfo *appinfo = NULL;

	g_return_val_if_fail (desktop_id != NULL, FALSE);
	g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

	if (g_path_is_absolute (desktop_id))
		appinfo = g_desktop_app_info_new_from_filename (desktop_id);

	if (appinfo == NULL)
		return FALSE;

	const char *cmdline = g_app_info_get_commandline (G_APP_INFO (appinfo));
	gchar *command = g_strdup (cmdline);
	command = g_strchug (command);
	if (!command || !command[0]) {
		g_free (command);
		retval = FALSE;
		goto out;
	}

	gchar *wd = desktop_working_directory_get (g_app_info_get_id (G_APP_INFO (appinfo)));
	retval = launch_command (screen, command, wd);
	g_free (wd);

	g_free (command);

out:
	g_object_unref (appinfo);

	return retval;
}


static gboolean
set_popup_window_position (GooroomApplauncherApplet *applet)
{
	GdkRectangle geometry;
	PanelAppletOrient orientation;
	gint x, y;
	gint popup_width, popup_height;
	gint applet_width, applet_height;

	GooroomApplauncherAppletPrivate *priv = applet->priv;

	g_return_val_if_fail (priv->popup_window != NULL, FALSE);

	orientation = panel_applet_get_orient (PANEL_APPLET (applet));

	gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (applet)), &x, &y);

	gtk_widget_get_preferred_width (GTK_WIDGET (applet), NULL, &applet_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (applet), NULL, &applet_height);

	gtk_widget_get_preferred_width (GTK_WIDGET (priv->popup_window), NULL, &popup_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (priv->popup_window), NULL, &popup_height);

	get_monitor_geometry (applet, &geometry);

	switch (orientation) {
		case PANEL_APPLET_ORIENT_DOWN:
			if (x + popup_width > geometry.x + geometry.width)
				x -= ((x + popup_width) - (geometry.x + geometry.width));
			y += applet_height;
		break;

		case PANEL_APPLET_ORIENT_UP:
			if (x + popup_width > geometry.x + geometry.width)
				x -= ((x + popup_width) - (geometry.x + geometry.width));
			y -= popup_height;
		break;

		case PANEL_APPLET_ORIENT_RIGHT:
			x += applet_width;
			if (y + popup_height > geometry.y + geometry.height)
				y -= ((y + popup_height) - (geometry.y + geometry.height));
		break;

		case PANEL_APPLET_ORIENT_LEFT:
			x -= popup_width;
			if (y + popup_height > geometry.y + geometry.height)
				y -= ((y + popup_height) - (geometry.y + geometry.height));
		break;

		default:
			g_assert_not_reached ();
	}

	gtk_window_move (GTK_WINDOW (priv->popup_window), x, y);

	return FALSE;
}

static void
launch_desktop_cb (ApplauncherWindow *window, const gchar *desktop_id, gpointer data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->button), FALSE);
	gtk_widget_destroy (GTK_WIDGET (window));

	GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (applet->priv->button));
	if (!launch_desktop_id (desktop_id, screen)) {
		show_error_dialog (NULL, screen, _("Failed to launch application"));
	}
}

static void
popup_window_closed_cb (ApplauncherWindow *window,
                        gint               reason,
                        gpointer           data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->button), FALSE);
	gtk_widget_destroy (GTK_WIDGET (window));
	applet->priv->popup_window = NULL;
}

static void
popup_window_realize_cb (ApplauncherWindow *window,
                         gpointer           data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);

	set_popup_window_position (applet);
}

static void
applauncher_window_popup (GooroomApplauncherApplet *applet)
{
	GdkRectangle workarea;
	ApplauncherWindow *window;

	GooroomApplauncherAppletPrivate *priv = applet->priv;

	window = priv->popup_window = applauncher_window_new ();
	gtk_window_set_screen (GTK_WINDOW (window),
                           gtk_widget_get_screen (GTK_WIDGET (applet)));

	get_workarea (applet, &workarea);
	applauncher_window_set_workarea (window, &workarea);

	g_signal_connect (G_OBJECT (window), "realize", G_CALLBACK (popup_window_realize_cb), applet);
	g_signal_connect (G_OBJECT (window), "closed", G_CALLBACK (popup_window_closed_cb), applet);
	g_signal_connect (G_OBJECT (window), "launch-desktop", G_CALLBACK (launch_desktop_cb), applet);

	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_window_present_with_time (GTK_WINDOW (window), gtk_get_current_event_time ());
}

static void
on_applet_button_toggled (GtkToggleButton *button, gpointer data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);
	GooroomApplauncherAppletPrivate *priv = applet->priv;

	if (gtk_toggle_button_get_active (button))
		applauncher_window_popup (applet);
}

//static gboolean
//popup_window_idle (gpointer data)
//{
//	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);
//	GooroomApplauncherAppletPrivate *priv = applet->priv;
//
//	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), TRUE);
//
//	return FALSE;
//}

static void
screen_size_changed_cb (GdkScreen *screen,
                        gpointer   data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);
	GooroomApplauncherAppletPrivate *priv = applet->priv;

	if (priv->popup_window) {
		gtk_widget_destroy (GTK_WIDGET (priv->popup_window));
		priv->popup_window = NULL;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->button), FALSE);

//		g_timeout_add (1000, popup_window_idle, applet);
	}
}

static void
monitors_changed_cb (GdkScreen *screen,
                     gpointer   data)
{
	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (data);
	GooroomApplauncherAppletPrivate *priv = applet->priv;

	if (priv->popup_window) {
		gtk_widget_destroy (GTK_WIDGET (priv->popup_window));
		priv->popup_window = NULL;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->button), FALSE);

//		g_timeout_add (1000, popup_window_idle, applet);
	}
}

static void
gooroom_applauncher_applet_realize (GtkWidget *widget)
{
	if (GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->realize) {
		GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->realize (widget);
	}

	g_signal_connect (gtk_widget_get_screen (widget), "size_changed",
                      G_CALLBACK (screen_size_changed_cb), widget);
}

static void
gooroom_applauncher_applet_unrealize (GtkWidget *widget)
{
	g_signal_handlers_disconnect_by_func (gtk_widget_get_screen (widget),
                                          screen_size_changed_cb, widget);

	if (GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->unrealize)
		GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->unrealize (widget);
}


static void
gooroom_applauncher_applet_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation)
{
	gint size;
	GtkOrientation orientation;

	GooroomApplauncherApplet *applet = GOOROOM_APPLAUNCHER_APPLET (widget);
	GooroomApplauncherAppletPrivate *priv = applet->priv;

	orientation = panel_applet_get_gtk_orientation (PANEL_APPLET (applet));

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		size = allocation->height;
	else
		size = allocation->width;

	gtk_widget_set_size_request (priv->button, size, size);

	if (GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->size_allocate)
		GTK_WIDGET_CLASS (gooroom_applauncher_applet_parent_class)->size_allocate (widget, allocation);
}

static void
gooroom_applauncher_applet_init (GooroomApplauncherApplet *applet)
{
	GdkDisplay *display;
	GooroomApplauncherAppletPrivate *priv;

	/* Initialize i18n */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	priv = applet->priv = gooroom_applauncher_applet_get_instance_private (applet);

	priv->popup_window = NULL;
//	keybinder_init ();

//	keybinder_bind ("Super_L", window_key_pressed_cb, applet);

	panel_applet_set_flags (PANEL_APPLET (applet), PANEL_APPLET_EXPAND_MINOR);

	display = gdk_display_get_default ();

	priv->button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button), GTK_RELIEF_NONE);
	gtk_widget_set_name (GTK_WIDGET (priv->button), "applauncher-button");
	gtk_container_add (GTK_CONTAINER (applet), priv->button);

	GtkWidget *icon = gtk_image_new_from_icon_name ("start-here-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (icon), TRAY_ICON_SIZE);
	gtk_container_add (GTK_CONTAINER (priv->button), icon);

	g_signal_connect (G_OBJECT (priv->button), "toggled",
                      G_CALLBACK (on_applet_button_toggled), applet);

	g_signal_connect (gdk_display_get_default_screen (display), "monitors-changed",
                      G_CALLBACK (monitors_changed_cb), applet);
}

static void
gooroom_applauncher_applet_finalize (GObject *object)
{
	G_OBJECT_CLASS (gooroom_applauncher_applet_parent_class)->finalize (object);
}

static void
gooroom_applauncher_applet_class_init (GooroomApplauncherAppletClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = gooroom_applauncher_applet_finalize;

	widget_class->realize       = gooroom_applauncher_applet_realize;
	widget_class->unrealize     = gooroom_applauncher_applet_unrealize;
	widget_class->size_allocate = gooroom_applauncher_applet_size_allocate;
}

static gboolean
gooroom_applauncher_applet_fill (GooroomApplauncherApplet *applet)
{
	g_return_val_if_fail (PANEL_IS_APPLET (applet), FALSE);

	gtk_widget_show_all (GTK_WIDGET (applet));

	return TRUE;
}

static gboolean
gooroom_applauncher_applet_factory (PanelApplet *applet,
                                    const gchar *iid,
                                    gpointer     data)
{
	gboolean retval = FALSE;

	if (!g_strcmp0 (iid, "GooroomApplauncherApplet"))
		retval = gooroom_applauncher_applet_fill (GOOROOM_APPLAUNCHER_APPLET (applet));

	return retval;
}

PANEL_APPLET_IN_PROCESS_FACTORY ("GooroomApplauncherAppletFactory",
                                 GOOROOM_TYPE_APPLAUNCHER_APPLET,
                                 gooroom_applauncher_applet_factory,
                                 NULL)
