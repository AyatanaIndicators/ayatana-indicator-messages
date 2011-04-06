/*
Looking for the default applications.  A quick lookup.

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

#ifndef DEFAULT_APPLICATIONS_H__
#define DEFAULT_APPLICATIONS_H__ 1

/* Used for override icons in the normal case, but didn't
   have a better place to put it. */
#define ICON_KEY  "X-Ayatana-Messaging-Menu-Icon"

const gchar * get_default_name  (const gchar * desktop_path);
const gchar * get_default_setup (const gchar * desktop_path);
const gchar * get_default_icon  (const gchar * desktop_path);

#endif /* DEFAULT_APPLICATIONS_H__ */

