/*
An indicator to show information that is in messaging applications
that the user is using.

Copyright 2012 Canonical Ltd.

Authors:
    Lars Uebernickel <lars.uebernickel@canonical.com>
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __APP_SECTION_H__
#define __APP_SECTION_H__

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define APP_SECTION_TYPE            (app_section_get_type ())
#define APP_SECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APP_SECTION_TYPE, AppSection))
#define APP_SECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APP_SECTION_TYPE, AppSectionClass))
#define IS_APP_SECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APP_SECTION_TYPE))
#define IS_APP_SECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APP_SECTION_TYPE))
#define APP_SECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), APP_SECTION_TYPE, AppSectionClass))

typedef struct _AppSection        AppSection;
typedef struct _AppSectionClass   AppSectionClass;
typedef struct _AppSectionPrivate AppSectionPrivate;


struct _AppSectionClass {
	GObjectClass parent_class;
};

struct _AppSection {
	GObject parent;
	AppSectionPrivate *priv;
};

GType app_section_get_type (void);
AppSection * app_section_new (GDesktopAppInfo *appinfo);
guint app_section_get_count (AppSection * appitem);
const gchar * app_section_get_name (AppSection * appitem);
const gchar * app_section_get_desktop (AppSection * appitem);
GActionGroup * app_section_get_actions (AppSection *self);
GMenuModel * app_section_get_menu (AppSection *appitem);
GAppInfo * app_section_get_app_info (AppSection *appitem);
gboolean app_section_get_draws_attention (AppSection *appitem);
void app_section_clear_draws_attention (AppSection *appitem);
void app_section_set_object_path (AppSection *self,
				  GDBusConnection *bus,
				  const gchar *bus_name,
				  const gchar *object_path);
void app_section_unset_object_path (AppSection *self);
gboolean app_section_get_uses_chat_status (AppSection *self);

G_END_DECLS

#endif /* __APP_SECTION_H__ */

