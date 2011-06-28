
#ifndef __STATUS_ITEMS_H__
#define __STATUS_ITEMS_H__

#include <glib.h>

G_BEGIN_DECLS

GList * status_items_build (void);
const gchar * status_current_panel_icon (gboolean alert);

G_END_DECLS

#endif /* __STATUS_ITEMS_H__ */

