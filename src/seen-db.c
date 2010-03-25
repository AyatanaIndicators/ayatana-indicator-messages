#include "seen-db.h"

SeenDB *
seen_db_init(void)
{
	GHashTable * hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return hash;
}

void
seen_db_add (SeenDB * seendb, const gchar * desktop)
{
	g_hash_table_insert(seendb,
	                    g_strdup(desktop),
	                    GINT_TO_POINTER(TRUE));

	return;
}

gboolean
seen_db_seen (SeenDB * seendb, const gchar * desktop)
{
	return GPOINTER_TO_INT(g_hash_table_lookup(seendb, desktop));
}
