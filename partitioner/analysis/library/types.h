// types.h -- Provide a few universal types and associated functions.
//
// This file is distributed under the MIT license, see LICENSE.txt.

#ifndef _TYPES_H
#define _TYPES_H

#include <cstdint>
#include <cstring>

typedef const char *    node_t;
typedef uint64_t        edge_t; // Edge weights are mutable ...
typedef const uint64_t  page_t;

static bool inline NODE_T_EQ(const node_t a, const node_t b)
{
  // Why 255?  Why not, it seems long enough for a (sensible) function name.
  return strncmp(a, b, 255) == 0;
}

static bool inline NODE_T_LT(const node_t a, const node_t b)
{
  // Why 255?  Why not, it seems long enough for a (sensible) function name.
  return strncmp(a, b, 255) < 0;
}

// Useful for STL data structures that require ordering (e.g. std::map).
class NODE_T_COMPARATOR {
public:
  bool operator()(const node_t a, const node_t b) const
  {
    return NODE_T_LT(a,b);
  }
};

#endif // _TYPES_H
