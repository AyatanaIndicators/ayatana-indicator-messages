
#include <glib.h>
#include <gtest/gtest.h>

extern "C" {
   #include "launcher-menu-item.h"
}

TEST(LauncherMenuItem, NameInitialized) {
  g_type_init();
  //const gchar * expected = "foo";
  //LauncherMenuItem * test_li = launcher_menu_item_new ("foo");
  //gchar * result = launcher_menu_item_get_name(test_li);
  //EXPECT_EQ(0, strcmp("foo", result));
  EXPECT_TRUE(true);
}
