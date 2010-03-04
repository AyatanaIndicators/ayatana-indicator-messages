
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

struct default_db_t *
get_default_helper (gchar * desktop_path)
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
get_default_name (gchar * desktop_path)
{
	struct default_db_t * db = get_default_helper(desktop_path);

	if (db == NULL)
		return NULL;
	return db->name;
}
