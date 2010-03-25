/*
A small database of which desktop files we've seen.

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

#include "seen-db.h"

#define GROUP_NAME   "Seen Database"
#define KEY_NAME     "DesktopFiles"

GHashTable * seendb = NULL;
gchar * filename = NULL;
guint write_process = 0;

/* Build the hashtable and then see if we have a keyfile that
   we can get the history of desktop files we've seen. */
void
seen_db_init(void)
{
	if (seendb != NULL) {
		return;
	}

	seendb = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Build the filename for the seen database.  We're putting
	   it in the cache directory because it could get deleted and
	   it really wouldn't be a big deal. */
	if (filename == NULL) {
		filename = g_build_filename(g_get_user_cache_dir(), "indicators", "messages", "seen-db.keyfile", NULL);
	}

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		GKeyFile * keyfile = g_key_file_new();
		
		if (!g_key_file_load_from_file(keyfile, filename, G_KEY_FILE_NONE, NULL)) {
			g_key_file_free(keyfile);
			keyfile = NULL;
		}

		if (keyfile != NULL && !g_key_file_has_key(keyfile, GROUP_NAME, KEY_NAME, NULL)) {
			g_warning("Seen DB '%s' does not have key '%s' in group '%s'", filename, KEY_NAME, GROUP_NAME);
			g_key_file_free(keyfile);
			keyfile = NULL;
		}
		
		if (keyfile != NULL) {
			gchar ** desktops = g_key_file_get_string_list(keyfile, GROUP_NAME, KEY_NAME, NULL, NULL);
			gint i = 0;

			while (desktops[i] != NULL) {
				g_hash_table_insert(seendb,
				                    g_strdup(desktops[i]),
				                    GINT_TO_POINTER(TRUE));
			}

			g_strfreev(desktops);
		}

		if (keyfile != NULL) {
			g_key_file_free(keyfile);
		}
	}

	return;
}

/* A function to write out the seen database after it's been
   modified for a while. */
static gboolean
write_seen_db (gpointer user_data)
{
	write_process = 0;
	return FALSE;
}

/* Add a new desktop file to the seen database.  Also sets up a timer
   to do the write out. */
void
seen_db_add (const gchar * desktop)
{
	if (!seen_db_seen(desktop)) {
		if (write_process != 0) {
			g_source_remove(write_process);
			write_process = 0;
		}

		write_process = g_timeout_add_seconds(300, write_seen_db, NULL);
	}

	g_hash_table_insert(seendb,
	                    g_strdup(desktop),
	                    GINT_TO_POINTER(TRUE));

	return;
}

/* Checks to see if a desktop file has been seen. */
gboolean
seen_db_seen (const gchar * desktop)
{
	return GPOINTER_TO_INT(g_hash_table_lookup(seendb, desktop));
}
