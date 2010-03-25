
#ifndef SEEN_DB_H__
#define SEEN_DB_H__ 1

#include <glib.h>

typedef GHashTable SeenDB;

SeenDB * seen_db_init(void);
void seen_db_add (SeenDB * seendb, const gchar * desktop);
gboolean seen_db_seen (SeenDB * seendb, const gchar * desktop);

#endif /* SEEN_DB_H__ */
