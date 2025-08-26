#pragma once
#ifdef PUBLIC_SKELETON
  #define SKIP_REDACTED() GTEST_SKIP() << "Redacted in public skeleton"
  #define EXPECT_REDACTED_THROW(stmt) EXPECT_THROW((stmt), std::runtime_error)
#else
  #define SKIP_REDACTED() ((void)0)
  #define EXPECT_REDACTED_THROW(stmt) ((void)0)
#endif
