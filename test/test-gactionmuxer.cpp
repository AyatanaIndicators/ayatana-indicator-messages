
#include <glib.h>
#include <gio/gio.h>
#include <gtest/gtest.h>

extern "C" {
#include "app-section.h"
#include "gactionmuxer.h"
}

static gboolean
strv_contains (gchar **str_array,
	       const gchar *str)
{
	gchar **it;

	for (it = str_array; *it; it++) {
		if (!g_strcmp0 (*it, str))
			return TRUE;
	}

	return FALSE;
}

TEST(GActionMuxerTest, Empty) {
	GActionMuxer *muxer;
	gchar **actions;

	g_type_init ();

	muxer = g_action_muxer_new ();

	actions = g_action_group_list_actions (G_ACTION_GROUP (muxer));
	EXPECT_EQ (0, g_strv_length (actions));

	g_strfreev (actions);
	g_object_unref (muxer);
}

TEST(GActionMuxerTest, General) {
	const GActionEntry entries1[] = { { "one" }, { "two" }, { "three" } };
	const GActionEntry entries2[] = { { "gb" }, { "es" }, { "fr" } };
	const GActionEntry entries3[] = { { "foo" }, { "bar" } };
	GSimpleActionGroup *group1;
	GSimpleActionGroup *group2;
	GSimpleActionGroup *group3;
	GActionMuxer *muxer;
	gchar **actions;

	g_type_init ();

	group1 = g_simple_action_group_new ();
	g_simple_action_group_add_entries (group1,
					   entries1,
					   G_N_ELEMENTS (entries1),
					   NULL);

	group2 = g_simple_action_group_new ();
	g_simple_action_group_add_entries (group2,
					   entries2,
					   G_N_ELEMENTS (entries2),
					   NULL);

	group3 = g_simple_action_group_new ();
	g_simple_action_group_add_entries (group3,
					   entries3,
					   G_N_ELEMENTS (entries3),
					   NULL);

	muxer = g_action_muxer_new ();
	g_action_muxer_insert (muxer, "first", G_ACTION_GROUP (group1));
	g_action_muxer_insert (muxer, "second", G_ACTION_GROUP (group2));
	g_action_muxer_insert (muxer, NULL, G_ACTION_GROUP (group3));

	actions = g_action_group_list_actions (G_ACTION_GROUP (muxer));
	EXPECT_EQ (8, g_strv_length (actions));
	EXPECT_TRUE (strv_contains (actions, "first.one"));
	EXPECT_TRUE (strv_contains (actions, "first.two"));
	EXPECT_TRUE (strv_contains (actions, "first.three"));
	EXPECT_TRUE (strv_contains (actions, "second.gb"));
	EXPECT_TRUE (strv_contains (actions, "second.es"));
	EXPECT_TRUE (strv_contains (actions, "second.fr"));
	EXPECT_TRUE (strv_contains (actions, "foo"));
	EXPECT_TRUE (strv_contains (actions, "bar"));
	g_strfreev (actions);

	g_object_unref (muxer);
	g_object_unref (group1);
	g_object_unref (group2);
	g_object_unref (group3);
}
