/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Lars Uebernickel <lars.uebernickel@canonical.com>
 */

#ifndef __G_MENU_UTILS_H__
#define __G_MENU_UTILS_H__

#include <gio/gio.h>

int     g_menu_find_section             (GMenu *menu,
                                         GMenuModel *section);

void    g_menu_append_with_icon         (GMenu *menu,
                                         const gchar *label,
                                         GIcon *icon,
                                         const gchar *detailed_action);

void    g_menu_append_with_icon_name    (GMenu *menu,
                                         const gchar *label,
                                         const gchar *icon_name,
                                         const gchar *detailed_action);

#endif
