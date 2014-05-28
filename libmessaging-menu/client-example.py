#!/usr/bin/env python

from gi.repository import GLib, Gio, MessagingMenu

mmapp = MessagingMenu.App(desktop_id='evolution.desktop')

# make the application appear in the messaging menu. The name and icon are taken from the desktop file above
mmapp.register()

def source_activated(mmapp, source_id):
  print('source {} activated'.format(source_id))

# do something when the user clicks on a source. The source will be removed automatically
mmapp.connect('activate-source', source_activated)

# add a 'source' (a menu item below the application's name) with the name 'Inbox' and a count of 7
icon = Gio.ThemedIcon.new_with_default_fallbacks('my-source-icon')
mmapp.append_source_with_count('inbox', icon, 'Inbox', 7)

# this is not necessary for gtk applications, which start a mainloop in gtk_main()
GLib.MainLoop().run()
