// page_tracking.cpp -- See page_tracking.h.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#include "page_tracking.h"

page_t PageTracker::memToPage(const void *addr)
{
  return ((page_t)addr) / PageSize;
}

node_t PageTracker::functionAccessesMemory(node_t fname, const void *ptr)
{
  return functionAccessesPage(fname, memToPage(ptr));
}

node_t PageTracker::functionAccessesPage(node_t fname, page_t page)
{
  node_t oldOwner = "main"; // By default all pages start owned by 'main'.

  // Check to see if this page already has an owner recorded, if it does save
  // the owner (now ex-owner), and erase that entry from the record.
  auto it = map.find(page);
  if (it != map.end()) {
    oldOwner = it->second;
    map.erase(it);
  }

  // Record that 'fname' now has ownership of the page.
  map[page] = fname;

  return oldOwner;
}
