/*
Looking for the default applications.  A quick lookup.

Copyright 2010 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include "default-applications.h"

struct default_db_t {
	const gchar * desktop_file;
	const gchar * uri_scheme;
	const gchar * name;
	const gchar * setupname;
	const gchar * icon;
};

struct default_db_t default_db[] = {
	{NULL,	              "mailto",   N_("Mail"),           N_("Set Up Mail..."),              "applications-email-panel"},
	{"empathy.desktop",   NULL,       N_("Chat"),           N_("Set Up Chat..."),              "applications-chat-panel"},
	{"gwibber.desktop",   NULL,       N_("Broadcast"),      N_("Set Up Broadcast Account..."), "applications-microblogging-panel"},
};

static struct default_db_t *
get_default_helper (const gchar * desktop_path)
{
	g_return_val_if_fail(desktop_path != NULL, NULL);
	gchar * basename = g_path_get_basename(desktop_path);
	g_return_val_if_fail(basename != NULL, NULL);

	gboolean found = FALSE;
	gint i;
	gint length = sizeof(default_db)/sizeof(default_db[0]);
	for (i = 0; i < length && !found; i++) {
		if (default_db[i].desktop_file) {
			if (g_strcmp0(default_db[i].desktop_file, basename) == 0) {
				found = TRUE;
			}
		} else if (default_db[i].uri_scheme) {
			GAppInfo *info = g_app_info_get_default_for_uri_scheme(default_db[i].uri_scheme);
			if (!info) {
				continue;
			}

			const gchar * filename = g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(info));
			if (!filename) {
				g_object_unref(info);
				continue;
			}

			gchar * default_basename = g_path_get_basename(filename);
			g_object_unref(info);
			if (g_strcmp0(default_basename, basename) == 0) {
				found = TRUE;
			}

			g_free(default_basename);
		}
	}

	g_free(basename);

	if (found) {
		return &default_db[i - 1];
	}

	return NULL;
}

const gchar *
get_default_name (const gchar * desktop_path)
{
	struct default_db_t * db = get_default_helper(desktop_path);

	if (db == NULL)
		return NULL;
	return db->name;
}

const gchar *
get_default_setup (const gchar * desktop_path)
{
	struct default_db_t * db = get_default_helper(desktop_path);

	if (db == NULL)
		return NULL;
	return db->setupname;
}

const gchar *
get_default_icon (const gchar * desktop_path)
{
	struct default_db_t * db = get_default_helper(desktop_path);

	if (db == NULL)
		return NULL;
	return db->icon;
}
