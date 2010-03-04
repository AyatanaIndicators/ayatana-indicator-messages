
#include <glib.h>
#include <glib/gi18n.h>
#include "default-applications.h"

struct default_db_t {
	const gchar * desktop_file;
	const gchar * name;
};

struct default_db_t default_db[] = {
	{"evolution.desktop", N_("Mail")},
	{"empathy.desktop", N_("Chat")},
	{"gwibber.desktop", N_("Microblogging")},
	{NULL, NULL}
};

const gchar *
get_default_name (gchar * desktop_path)
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

	if (default_db[i].name != NULL) {
		return default_db[i].name;
	}

	return NULL;
}
