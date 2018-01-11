// test_page_tracking.cpp -- Test page_tracking.[cpp,h].
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include "../page_tracking.h"
#include <gtest/gtest.h>

const unsigned int onepage = PageTracker::PageSize;
const char *fakeptr = (const char *)onepage; // Don't dereference this!

// Test that we can map memory addresses to pages.
TEST(PageTrackingTest, PageCalculations) {
  EXPECT_EQ(PageTracker::memToPage(fakeptr), 1);
  EXPECT_EQ(PageTracker::memToPage(fakeptr+1), 1);
  EXPECT_EQ(PageTracker::memToPage(fakeptr+onepage-1), 1);
  EXPECT_EQ(PageTracker::memToPage(fakeptr+onepage), 2);
  EXPECT_EQ(PageTracker::memToPage(fakeptr+onepage*100), 101);
}

// Test that we can track 'ownership' of pages.
TEST(PageTrackingTest, PageTracking) {
  PageTracker pt;

  // Function 'foo' accesses a previously unaccessed page, so the page should
  // have default ownership ('main').
  EXPECT_STREQ(pt.functionAccessesMemory("foo", fakeptr), "main");

  // Function 'foo' accesses the same address again, but it should now have
  // ownership of the page.
  EXPECT_STREQ(pt.functionAccessesMemory("foo", fakeptr), "foo");

  // Now 'bar' accesses the page (but from a different adddress), 'foo' should
  // still have ownership.
  EXPECT_STREQ(pt.functionAccessesMemory("bar", fakeptr+1), "foo");

  // 'baz' accesses the page, 'bar' should have ownership.
  EXPECT_STREQ(pt.functionAccessesMemory("baz", fakeptr+2), "bar");

  // Finally, test that the next page was not affected by the above (i.e. it
  // still has default ownership, 'main').
  EXPECT_STREQ(pt.functionAccessesMemory("baz", fakeptr+onepage), "main");
}
