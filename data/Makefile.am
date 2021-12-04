gsettings_schemas_in_in = apps.gooroom-applauncher-applet.gschema.xml.in.in
gsettings_schemas_in = $(gsettings_schemas_in_in:.xml.in.in=.xml.in)
gsettings_SCHEMAS = $(gsettings_schemas_in:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

@GSETTINGS_RULES@

iconsdir = $(datadir)/icons/hicolor/scalable/apps
icons_DATA = gooroom-applauncher-applet.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then                 \
		echo "Updating Gtk icon cache.";            \
		$(gtk_update_icon_cache);               \
	else                                \
		echo "*** Icon cache not updated. Remember to run:";    \
		echo "***";                     \
		echo "***   $(gtk_update_icon_cache)";          \
		echo "***";                     \
	fi

EXTRA_DIST = \
	$(gsettings_schemas_in_in)

CLEANFILES = \
	$(gsettings_schemas_in) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid
