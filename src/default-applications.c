
#include <glib.h>
#include <glib/gi18n.h>
#include "default-applications.h"

struct default_db_t {
	const gchar * desktop_file;
	const gchar * name;
};

struct default_db_t default_db[] = {
	{"evolution.desktop", N_("Mail")},
	{NULL, NULL}
};

const gchar *
get_default_name (gchar * desktop_path)
{

	return NULL;


}
