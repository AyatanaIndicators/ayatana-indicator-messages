#include "seen-db.h"

GHashTable * seendb = NULL;

void
seen_db_init(void)
{
	if (seendb != NULL) {
		return;
	}

	seendb = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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
