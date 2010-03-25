#include "seen-db.h"

GHashTable * seendb = NULL;
gchar * filename = NULL;

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

	}

	return;
}

void
seen_db_add (const gchar * desktop)
{
	g_hash_table_insert(seendb,
	                    g_strdup(desktop),
	                    GINT_TO_POINTER(TRUE));

	return;
}

gboolean
seen_db_seen (const gchar * desktop)
{
	return GPOINTER_TO_INT(g_hash_table_lookup(seendb, desktop));
}
