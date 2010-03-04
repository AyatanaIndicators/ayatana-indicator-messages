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
#include "default-applications.h"

struct default_db_t {
	const gchar * desktop_file;
	const gchar * name;
	const gchar * setupname;
};

struct default_db_t default_db[] = {
	{"evolution.desktop", N_("Mail"),           N_("Set Up Mail...")},
	{"empathy.desktop",   N_("Chat"),           N_("Set Up Chat...")},
	{"gwibber.desktop",   N_("Microblogging"),  N_("Set Up Microblogging...")},
	{NULL, NULL}
};

static struct default_db_t *
get_default_helper (const gchar * desktop_path)
{
	g_return_val_if_fail(desktop_path != NULL, NULL);
	gchar * basename = g_path_get_basename(desktop_path);
	g_return_val_if_fail(basename != NULL, NULL);

	gint i;
	for (i = 0; default_db[i].desktop_file != NULL; i++) {
		if (g_strcmp0(default_db[i].desktop_file, basename) == 0) {
			break;
		}
	}

	g_free(basename);

	if (default_db[i].desktop_file != NULL) {
		return &default_db[i];
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
