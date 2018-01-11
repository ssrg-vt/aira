// page_tracking.h -- Record which functions access which pages.
//
// This file is distributed under the MIT license, see LICENSE.txt.
//
// The main purpose of this class is to provide information about the function
// that previously accessed a page, i.e. the function that currently 'owns' a
// page.  So when a function accesses a page, it is recorded as the new owner,
// and the previous owner is returned.  Of course, the current function may
// already be the owner of a page.
//
// The point of this is that if two functions end up on separate kernels we
// need to know how many page faults this will cause so that we can choose a
// partitioning that minimises the number of faults (balanced with other
// costs).

#ifndef _PAGE_TRACKING_H
#define _PAGE_TRACKING_H

#include <map>
#include "types.h"

class PageTracker {
public:
  static const unsigned int PageSize = 4096;

  // Convert an address to the page that that address refers to.
  static page_t memToPage(const void *);

  // Record that a function accesses a particular memory address or page.  For
  // now we don't care about the difference between a read and a write so we
  // don't provide separate tracking for them.
  node_t functionAccessesMemory(node_t fname, const void *ptr);
  node_t functionAccessesPage(node_t fname, page_t page);

private:

  // The map from page->function, represents the current 'owner' of the page.
  std::map<page_t, node_t> map;

};

#endif // _PAGE_TRACKING_H
