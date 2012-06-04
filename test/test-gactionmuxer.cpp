
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

TEST(GActionMuxerTest, AddAndRemove) {
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
	EXPECT_TRUE (g_action_group_has_action (G_ACTION_GROUP (muxer), "first.one"));
	EXPECT_FALSE (g_action_group_has_action (G_ACTION_GROUP (muxer), "one"));
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

	g_action_muxer_remove (muxer, NULL);
	EXPECT_FALSE (g_action_group_has_action (G_ACTION_GROUP (muxer), "foo"));
	EXPECT_TRUE (g_action_group_has_action (G_ACTION_GROUP (muxer), "first.one"));
	actions = g_action_group_list_actions (G_ACTION_GROUP (muxer));
	EXPECT_EQ (6, g_strv_length (actions));
	EXPECT_FALSE (strv_contains (actions, "foo"));
	EXPECT_TRUE (strv_contains (actions, "first.one"));
	g_strfreev (actions);

	g_action_muxer_remove (muxer, "first");
	EXPECT_FALSE (g_action_group_has_action (G_ACTION_GROUP (muxer), "first.two"));
	EXPECT_TRUE (g_action_group_has_action (G_ACTION_GROUP (muxer), "second.es"));
	actions = g_action_group_list_actions (G_ACTION_GROUP (muxer));
	EXPECT_EQ (3, g_strv_length (actions));
	EXPECT_FALSE (strv_contains (actions, "first.two"));
	EXPECT_TRUE (strv_contains (actions, "second.es"));
	g_strfreev (actions);

	g_object_unref (muxer);
	g_object_unref (group1);
	g_object_unref (group2);
	g_object_unref (group3);
}

static gboolean
g_variant_equal0 (gconstpointer one,
		  gconstpointer two)
{
	if (one == NULL)
		return two == NULL;
	else
		return g_variant_equal (one, two);
}

TEST(GActionMuxerTest, ActionAttributes) {
	GSimpleActionGroup *group;
	GSimpleAction *action;
	GActionMuxer *muxer;
	gboolean enabled[2];
	const GVariantType *param_type[2];
	const GVariantType *state_type[2];
	GVariant *state_hint[2];
	GVariant *state[2];

	g_type_init ();

	group = g_simple_action_group_new ();
	action = g_simple_action_new ("one", G_VARIANT_TYPE_STRING);
	g_simple_action_group_insert (group, G_ACTION (action));

	muxer = g_action_muxer_new ();
	g_action_muxer_insert (muxer, "first", G_ACTION_GROUP (group));

	/* test two of the convenience functions */
	ASSERT_TRUE (g_action_group_get_action_enabled (G_ACTION_GROUP (muxer), "first.one"));
	g_simple_action_set_enabled (action, FALSE);
	ASSERT_FALSE (g_action_group_get_action_enabled (G_ACTION_GROUP (muxer), "first.one"));

	ASSERT_STREQ ((gchar *) g_action_group_get_action_parameter_type (G_ACTION_GROUP (muxer), "first.one"),
		      (gchar *) G_VARIANT_TYPE_STRING);

	/* query_action */
	g_action_group_query_action (G_ACTION_GROUP (group), "one",
				     &enabled[0], &param_type[0], &state_type[0], &state_hint[0], &state[0]);
	g_action_group_query_action (G_ACTION_GROUP (muxer), "first.one",
				     &enabled[1], &param_type[1], &state_type[1], &state_hint[1], &state[1]);
	ASSERT_EQ (enabled[0], enabled[1]);
	ASSERT_STREQ ((gchar *) param_type[0], (gchar *) param_type[1]);
	ASSERT_STREQ ((gchar *) state_type[0], (gchar *) state_type[1]);
	ASSERT_TRUE (g_variant_equal0 ((gconstpointer) state_hint[0], (gconstpointer) state_hint[1]));
	ASSERT_TRUE (g_variant_equal0 ((gconstpointer) state[0], (gconstpointer) state[1]));

	g_object_unref (action);
	g_object_unref (group);
	g_object_unref (muxer);
}

typedef struct {
	gboolean signal_ran;
	const gchar *name;
} TestSignalClosure;

static void
action_added (GActionGroup *group,
	      gchar *action_name,
	      gpointer user_data)
{
	TestSignalClosure *c = (TestSignalClosure *)user_data;
	EXPECT_STREQ (c->name, action_name);
	c->signal_ran = TRUE;
}

static void
action_enabled_changed (GActionGroup *group,
			gchar *action_name,
			gboolean enabled,
			gpointer user_data)
{
	TestSignalClosure *c = (TestSignalClosure *)user_data;
	EXPECT_EQ (enabled, FALSE);
	c->signal_ran = TRUE;
}

static void
action_removed (GActionGroup *group,
		gchar *action_name,
		gpointer user_data)
{
	TestSignalClosure *c = (TestSignalClosure *)user_data;
	EXPECT_STREQ (c->name, action_name);
	c->signal_ran = TRUE;
}

TEST(GActionMuxerTest, Signals) {
	GSimpleActionGroup *group;
	GSimpleAction *action;
	GActionMuxer *muxer;
	TestSignalClosure closure;

	group = g_simple_action_group_new ();

	action = g_simple_action_new ("one", G_VARIANT_TYPE_STRING);
	g_simple_action_group_insert (group, G_ACTION (action));
	g_object_unref (action);

	muxer = g_action_muxer_new ();

	g_signal_connect (muxer, "action-added",
			  G_CALLBACK (action_added), (gpointer) &closure);
	g_signal_connect (muxer, "action-enabled-changed",
			  G_CALLBACK (action_enabled_changed), (gpointer) &closure);
	g_signal_connect (muxer, "action-removed",
			  G_CALLBACK (action_removed), (gpointer) &closure);

	/* add the group with "one" action and check whether the signal is emitted */
	closure.signal_ran = FALSE;
	closure.name = "first.one";
	g_action_muxer_insert (muxer, "first", G_ACTION_GROUP (group));
	ASSERT_TRUE (closure.signal_ran);

	/* add a second action after the group was added to the muxer */
	closure.signal_ran = FALSE;
	closure.name = "first.two";
	action = g_simple_action_new ("two", G_VARIANT_TYPE_STRING);
	g_simple_action_group_insert (group, G_ACTION (action));
	ASSERT_TRUE (closure.signal_ran);

	/* disable the action */
	closure.signal_ran = FALSE;
	g_simple_action_set_enabled (action, FALSE);
	ASSERT_TRUE (closure.signal_ran);
	g_object_unref (action);

	/* remove the first action */
	closure.signal_ran = FALSE;
	closure.name = "first.one";
	g_simple_action_group_remove (group, "one");
	ASSERT_TRUE (closure.signal_ran);

	/* remove the whole group, should be notified about "first.two" */
	closure.signal_ran = FALSE;
	closure.name = "first.two";
	g_action_muxer_remove (muxer, "first");
	ASSERT_TRUE (closure.signal_ran);

	g_object_unref (group);
	g_object_unref (muxer);
}
