/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
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

#ifndef __g_tuple_action_h__
#define __g_tuple_action_h__

#include <gio/gio.h>

#define G_TYPE_TUPLE_ACTION  (g_tuple_action_get_type ())
#define G_TUPLE_ACTION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_TUPLE_ACTION, GTupleAction))
#define G_IS_TUPLE_ACTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_TUPLE_ACTION))

typedef struct _GTupleAction GTupleAction;

GType           g_tuple_action_get_type     (void) G_GNUC_CONST;

GTupleAction *  g_tuple_action_new          (const gchar *name,
                                             GVariant    *initial_state);

void            g_tuple_action_set_child    (GTupleAction *action,
                                             gsize         index,
                                             GVariant     *value);

#endif
