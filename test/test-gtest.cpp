
#include <glib.h>
#include <gtest/gtest.h>

extern "C" {
   #include "launcher-menu-item.h"
}

TEST(LauncherMenuItem, NameInitialized) {
  g_type_init();
  EXPECT_TRUE(true);
}
