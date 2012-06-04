
#include <glib.h>
#include <gtest/gtest.h>

extern "C" {
   #include "app-section.h"
}

TEST(AppSection, NameInitialized) {
  g_type_init();
  EXPECT_TRUE(true);
}
