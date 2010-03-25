/*
A small database of which desktop files we've seen.

Copyright 2010 Canonical Ltd.

Authors:
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

#ifndef SEEN_DB_H__
#define SEEN_DB_H__ 1

#include <glib.h>

void seen_db_init(void);
void seen_db_add (const gchar * desktop);
gboolean seen_db_seen (const gchar * desktop);

#endif /* SEEN_DB_H__ */
