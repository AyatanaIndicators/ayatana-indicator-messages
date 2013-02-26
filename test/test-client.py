#!/usr/bin/env python3

import unittest
import dbus
import dbusmock
import subprocess
from gi.repository import GLib, Gio, MessagingMenu

class MessagingMenuTest(dbusmock.DBusTestCase):
    @classmethod
    def setUpClass(klass):
        klass.start_session_bus()
        klass.bus = klass.get_dbus(False)

    def setUp(self):
        name = 'com.canonical.indicator.messages'
        obj_path = '/com/canonical/indicator/messages/service'
        iface = 'com.canonical.indicator.messages.service'

        self.messaging_service = self.spawn_server(name, obj_path, iface, stdout=subprocess.PIPE)
        self.mock = dbus.Interface(self.bus.get_object(name, obj_path), dbusmock.MOCK_IFACE)
        self.mock.AddMethod('', 'RegisterApplication', 'so', '', '')
        self.mock.AddMethod('', 'UnregisterApplication', 's', '', '')
        self.mock.AddMethod('', 'ApplicationStoppedRunning', 's', '', '')
        self.mock.AddMethod('', 'SetStatus', 'ss', '', '')

        self.loop = GLib.MainLoop()

    def tearDown(self):
        self.messaging_service.terminate()
        self.messaging_service.wait()

    def spin_loop(self, ms=10):
        GLib.timeout_add(ms, lambda: self.loop.quit())
        self.loop.run()

    def assertMethodCalled(self, name, *expected_args):
        calls = self.mock.GetMethodCalls(name)
        self.assertEqual(len(calls), 1, 'method %s was not called' % name)
        args = calls[0][1]
        self.assertEqual(len(args), len(expected_args))
        for i in range(len(args)):
            if expected_args[i]:
                self.assertEqual(args[i], expected_args[i])

    def test_registration(self):
        mmapp = MessagingMenu.App.new('empathy.desktop')
        mmapp.register()
        self.spin_loop()
        self.assertMethodCalled('RegisterApplication', 'empathy.desktop', None)

        mmapp.unregister()
        self.spin_loop()
        self.assertMethodCalled('UnregisterApplication', 'empathy.desktop')

        # ApplicationStoppedRunning is called when the last ref on mmapp is dropped
        del mmapp
        self.spin_loop()
        self.assertMethodCalled('ApplicationStoppedRunning', 'empathy.desktop')

    def test_status(self):
        mmapp = MessagingMenu.App.new('empathy.desktop')
        mmapp.register()
        mmapp.set_status(MessagingMenu.Status.AWAY)
        self.spin_loop()
        self.assertMethodCalled('SetStatus', 'empathy.desktop', 'away')

unittest.main(testRunner=unittest.TextTestRunner())
