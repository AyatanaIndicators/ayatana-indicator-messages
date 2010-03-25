
#ifndef SEEN_DB_H__
#define SEEN_DB_H__ 1

#include <glib.h>

void seen_db_init(void);
void seen_db_add (const gchar * desktop);
gboolean seen_db_seen (const gchar * desktop);

#endif /* SEEN_DB_H__ */
