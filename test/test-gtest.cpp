
#include <glib.h>
#include <gtest/gtest.h>

extern "C" {
   #include "launcher-menu-item.h"
}

TEST(LauncherMenuItem, EmptyAtStart) {
  gboolean result;
  // FIXME
  //LauncherMenuItem * test_li = ???;
  gboolean test_eclipsed;
  //result = launcher_menu_item_set_eclipsed(test_li, test_eclipsed);
  EXPECT_TRUE(false);
}
