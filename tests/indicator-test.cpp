/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Ted Gould <ted@canonical.com>
 */

#include <gtest/gtest.h>
#include <gio/gio.h>

#include "indicator-fixture.h"
#include "accounts-service-mock.h"

#include "messaging-menu-app.h"
#include "messaging-menu-message.h"

class IndicatorTest : public IndicatorFixture
{
protected:
	IndicatorTest (void) :
		IndicatorFixture(INDICATOR_MESSAGES_SERVICE_BINARY, "com.canonical.indicator.messages")
	{
	}

	std::shared_ptr<AccountsServiceMock> as;

	virtual void SetUp() override
	{
		g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);
		g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

		g_setenv("XDG_DATA_DIRS", XDG_DATA_DIRS, TRUE);

		as = std::make_shared<AccountsServiceMock>();
		addMock(*as);

		IndicatorFixture::SetUp();
	}

	virtual void TearDown() override
	{
		as.reset();

		IndicatorFixture::TearDown();
	}

};


TEST_F(IndicatorTest, RootAction) {
	setActions("/com/canonical/indicator/messages");

	EXPECT_EVENTUALLY_ACTION_EXISTS("messages");
	EXPECT_ACTION_STATE_TYPE("messages", G_VARIANT_TYPE("a{sv}"));
	EXPECT_ACTION_STATE("messages", g_variant_new_parsed("{'icon': <('themed', <['indicator-messages-offline', 'indicator-messages', 'indicator']>)>, 'title': <'Notifications'>, 'accessible-desc': <'Messages'>, 'visible': <false>}"));
}

TEST_F(IndicatorTest, SingleMessage) {
	setActions("/com/canonical/indicator/messages");

	auto app = std::shared_ptr<MessagingMenuApp>(messaging_menu_app_new("test.desktop"), [](MessagingMenuApp * app) { g_clear_object(&app); });
	ASSERT_NE(nullptr, app);
	messaging_menu_app_register(app.get());

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.launch");

	auto msg = std::shared_ptr<MessagingMenuMessage>(messaging_menu_message_new(
		"testid",
		nullptr, /* no icon */
		"Test Title",
		"A subtitle too",
		"You only like me for my body",
		0), [](MessagingMenuMessage * msg) { g_clear_object(&msg); });
	messaging_menu_app_append_message(app.get(), msg.get(), nullptr, FALSE);

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.msg.testid");

	setMenu("/com/canonical/indicator/messages/phone");

	EXPECT_EVENTUALLY_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "x-canonical-type", "com.canonical.indicator.messages.messageitem");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "label", "Test Title");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "x-canonical-message-id", "testid");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "x-canonical-subtitle", "A subtitle too");
	EXPECT_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "x-canonical-text", "You only like me for my body");
}

static void
messageReplyActivate (GObject * obj, gchar * name, GVariant * value, gpointer user_data) {
	std::string * res = reinterpret_cast<std::string *>(user_data);
	*res = g_variant_get_string(value, nullptr);
}

TEST_F(IndicatorTest, MessageReply) {
	setActions("/com/canonical/indicator/messages");

	auto app = std::shared_ptr<MessagingMenuApp>(messaging_menu_app_new("test.desktop"), [](MessagingMenuApp * app) { g_clear_object(&app); });
	ASSERT_NE(nullptr, app);
	messaging_menu_app_register(app.get());

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.launch");

	auto msg = std::shared_ptr<MessagingMenuMessage>(messaging_menu_message_new(
		"messageid",
		nullptr, /* no icon */
		"Reply Message",
		"A message to reply to",
		"In-app replies are for wimps, reply here to save yourself time and be cool.",
		0), [](MessagingMenuMessage * msg) { g_clear_object(&msg); });
	messaging_menu_message_add_action(msg.get(),
		"replyid",
		"Reply",
		G_VARIANT_TYPE_STRING,
		nullptr);
	messaging_menu_app_append_message(app.get(), msg.get(), nullptr, FALSE);

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.msg.messageid");
	EXPECT_EVENTUALLY_ACTION_EXISTS("test.msg-actions.messageid.replyid");
	EXPECT_ACTION_ACTIVATION_TYPE("test.msg-actions.messageid.replyid", G_VARIANT_TYPE_STRING);

	EXPECT_ACTION_ENABLED("remove-all", true);

	setMenu("/com/canonical/indicator/messages/phone");

	EXPECT_EVENTUALLY_MENU_ATTRIB(std::vector<int>({0, 0, 0}), "x-canonical-type", "com.canonical.indicator.messages.messageitem");

	std::string activateResponse;
	g_signal_connect(msg.get(), "activate", G_CALLBACK(messageReplyActivate), &activateResponse);

	activateAction("test.msg-actions.messageid.replyid", g_variant_new_string("Reply to me"));

	EXPECT_EVENTUALLY_EQ("Reply to me", activateResponse);

	EXPECT_EVENTUALLY_ACTION_ENABLED("remove-all", false);
}

TEST_F(IndicatorTest, IconNotification) {
	auto normalicon = std::shared_ptr<GVariant>(g_variant_ref_sink(g_variant_new_parsed("{'icon': <('themed', <['indicator-messages-offline', 'indicator-messages', 'indicator']>)>, 'title': <'Notifications'>, 'accessible-desc': <'Messages'>, 'visible': <true>}")), [](GVariant *var) {if (var != nullptr) g_variant_unref(var); });
	auto blueicon = std::shared_ptr<GVariant>(g_variant_ref_sink(g_variant_new_parsed("{'icon': <('themed', <['indicator-messages-new-offline', 'indicator-messages-new', 'indicator-messages', 'indicator']>)>, 'title': <'Notifications'>, 'accessible-desc': <'New Messages'>, 'visible': <true>}")), [](GVariant *var) {if (var != nullptr) g_variant_unref(var); });

	setActions("/com/canonical/indicator/messages");

	auto app = std::shared_ptr<MessagingMenuApp>(messaging_menu_app_new("test.desktop"), [](MessagingMenuApp * app) { g_clear_object(&app); });
	ASSERT_NE(nullptr, app);
	messaging_menu_app_register(app.get());

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.launch");

	EXPECT_ACTION_STATE("messages", normalicon);

	auto app2 = std::shared_ptr<MessagingMenuApp>(messaging_menu_app_new("test2.desktop"), [](MessagingMenuApp * app) { g_clear_object(&app); });
	ASSERT_NE(nullptr, app2);
	messaging_menu_app_register(app2.get());

	EXPECT_EVENTUALLY_ACTION_EXISTS("test2.launch");

	messaging_menu_app_append_source_with_count(app2.get(),
		"countsource",
		nullptr,
		"Count Source",
		500);
	messaging_menu_app_draw_attention(app2.get(), "countsource");

	EXPECT_EVENTUALLY_ACTION_STATE("messages", blueicon);

	auto msg = std::shared_ptr<MessagingMenuMessage>(messaging_menu_message_new(
		"messageid",
		nullptr, /* no icon */
		"Message",
		"A secret message",
		"asdfa;lkweraoweprijas;dvlknasvdoiewur;aslkd",
		0), [](MessagingMenuMessage * msg) { g_clear_object(&msg); });
	messaging_menu_message_set_draws_attention(msg.get(), true);
	messaging_menu_app_append_message(app.get(), msg.get(), nullptr, FALSE);

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.msg.messageid");
	EXPECT_ACTION_STATE("messages", blueicon);

	messaging_menu_app_unregister(app2.get());
	app2.reset();

	EXPECT_EVENTUALLY_ACTION_DOES_NOT_EXIST("test2.msg.countsource");
	EXPECT_ACTION_STATE("messages", blueicon);

	messaging_menu_app_remove_message(app.get(), msg.get());

	EXPECT_EVENTUALLY_ACTION_STATE("messages", normalicon);
	EXPECT_ACTION_ENABLED("remove-all", false);

	messaging_menu_app_append_message(app.get(), msg.get(), nullptr, FALSE);

	EXPECT_EVENTUALLY_ACTION_STATE("messages", blueicon);
	EXPECT_ACTION_ENABLED("remove-all", true);

	activateAction("remove-all");

	EXPECT_EVENTUALLY_ACTION_STATE("messages", normalicon);
}
