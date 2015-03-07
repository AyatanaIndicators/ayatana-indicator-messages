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

	auto msg = std::shared_ptr<MessagingMenuMessage>(messaging_menu_message_new(
		"test-id",
		nullptr, /* no icon */
		"Test Title",
		"A subtitle too",
		"You only like me for my body",
		0), [](MessagingMenuMessage * msg) { g_clear_object(&msg); });
	messaging_menu_app_append_message(app.get(), msg.get(), nullptr, FALSE);

	EXPECT_EVENTUALLY_ACTION_EXISTS("test.launch");

}
