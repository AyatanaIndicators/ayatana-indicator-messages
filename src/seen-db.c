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
gchar * dirname = NULL;
guint write_process = 0;

/* Build the hashtable and then see if we have a keyfile that
   we can get the history of desktop files we've seen. */
void
seen_db_init(void)
{
	g_return_if_fail(seendb == NULL);

	seendb = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Build the filename for the seen database.  We're putting
	   it in the cache directory because it could get deleted and
	   it really wouldn't be a big deal. */
	if (dirname == NULL) {
		dirname = g_build_filename(g_get_user_cache_dir(), "indicators", "messages", NULL);
	}
	if (filename == NULL) {
		filename = g_build_filename(dirname, "seen-db.keyfile", NULL);
	}

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		GKeyFile * keyfile = g_key_file_new();
		
		/* Load from file */
		if (!g_key_file_load_from_file(keyfile, filename, G_KEY_FILE_NONE, NULL)) {
			g_key_file_free(keyfile);
			keyfile = NULL;
		}

		/* Check for keys */
		if (keyfile != NULL && !g_key_file_has_key(keyfile, GROUP_NAME, KEY_NAME, NULL)) {
			g_warning("Seen DB '%s' does not have key '%s' in group '%s'", filename, KEY_NAME, GROUP_NAME);
			g_key_file_free(keyfile);
			keyfile = NULL;
		}
		
		/* Grab them and put in DB */
		if (keyfile != NULL) {
			gchar ** desktops = g_key_file_get_string_list(keyfile, GROUP_NAME, KEY_NAME, NULL, NULL);
			gint i = 0;

			while (desktops[i] != NULL) {
				g_hash_table_insert(seendb,
				                    g_strdup(desktops[i]),
				                    GINT_TO_POINTER(TRUE));
				i++;
			}

			g_strfreev(desktops);
		}

		/* Clean up our file */
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

	/* Build up the key file */
	GKeyFile * keyfile = g_key_file_new();
	GArray * desktops = g_array_new(FALSE, FALSE, sizeof(gchar *));

	/* Get the keys from the hashtable and make them
	   into an array */
	if (keyfile != NULL) {
		GList * desktop_keys = g_hash_table_get_keys(seendb);
		GList * head = NULL;

		for (head = desktop_keys; head != NULL; head = g_list_next(head)) {
			g_array_append_val(desktops, head->data);
		}

		g_list_free(desktop_keys);
	}

	/* Use the array to dump the strings into the keyfile */
	g_key_file_set_string_list(keyfile,
	                           GROUP_NAME,
	                           KEY_NAME,
	                           (const gchar * const *)desktops->data,
	                           desktops->len);
	g_array_free(desktops, TRUE);

	/* Dump the key file to string */
	gchar * keydump = NULL;
	gsize keydumplen = 0;
	keydump = g_key_file_to_data(keyfile, &keydumplen, NULL);
	g_key_file_free(keyfile);

	/* Ensure the directory exists */
	if (g_mkdir_with_parents(dirname, 0700) != 0) {
		g_warning("Unable to make directory: %s", dirname);
		g_free(keydump);
		return FALSE;
	}

	/* Dump out the file */
	GError * error = NULL;
	if (!g_file_set_contents(filename, keydump, keydumplen, &error)) {
		g_warning("Unable to write out file '%s': %s", filename, error->message);
		g_error_free(error);
	}

	/* Clean up */
	g_free(keydump);

	return FALSE;
}

/* Add a new desktop file to the seen database.  Also sets up a timer
   to do the write out. */
void
seen_db_add (const gchar * desktop)
{
	/* If this is a new one, let's set up the timer.  If
	   there's already one clear it. */
	if (!seen_db_seen(desktop)) {
		if (write_process != 0) {
			g_source_remove(write_process);
			write_process = 0;
		}

		write_process = g_timeout_add_seconds(60, write_seen_db, NULL);
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
