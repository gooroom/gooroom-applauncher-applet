gooroomappletdir = $(GNOME_PANEL_MODULES_DIR)
gooroomapplet_LTLIBRARIES = libgooroom-applauncher-applet.la

libgooroom_applauncher_applet_la_CPPFLAGS =	\
	-I$(srcdir) \
	-I$(top_srcdir) \
	-DGNOMELOCALEDIR=\""$(localedir)"\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DGMENU_I_KNOW_THIS_IS_UNSTABLE \
	-DGDK_DISABLE_DEPRECATED \
	-DSN_API_NOT_YET_FROZEN

BUILT_SOURCES = \
	applauncher-applet-resources.c \
	applauncher-applet-resources.h

libgooroom_applauncher_applet_la_SOURCES = \
	$(BUILT_SOURCES)	\
	panel-glib.h    \
	panel-glib.c    \
	applauncher-appitem.h   \
	applauncher-appitem.c   \
	applauncher-directory-item.h   \
	applauncher-directory-item.c   \
	applauncher-indicator.h \
	applauncher-indicator.c \
	applauncher-applet.h	\
	applauncher-applet.c	\
	applauncher-module.c	\
	applauncher-window.h	\
	applauncher-window.c

libgooroom_applauncher_applet_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GNOME_MENU_CFLAGS)	\
	$(LIBGNOMEPANEL_CFLAGS) \
	$(STARTUP_NOTIFICATION_CFLAGS) \
	$(AM_CFLAGS)

libgooroom_applauncher_applet_la_LDFLAGS = \
	-avoid-version \
	$(AM_LDFLAGS)

libgooroom_applauncher_applet_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(GNOME_MENU_LIBS)	\
	$(LIBGNOMEPANEL_LIBS) \
	$(STARTUP_NOTIFICATION_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
applauncher-applet-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name applauncher_applet $<
applauncher-applet-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name applauncher_applet $<

CLEANFILES = $(BUILT_SOURCES)
