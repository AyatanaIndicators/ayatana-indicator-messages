
#ifndef __STATUS_ITEMS_H__
#define __STATUS_ITEMS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef void (*StatusUpdateFunc) (void);

GList * status_items_build (StatusUpdateFunc update_func);
const gchar * status_current_panel_icon (gboolean alert);
void status_items_cleanup (void);

G_END_DECLS

#endif /* __STATUS_ITEMS_H__ */

