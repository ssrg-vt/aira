/*
 * Red-black tree, adapted from "http://en.literateprograms.org/Red-black_tree_(C)"
 *
 * Modifications by Rob Lyerly <rlyerly@vt.edu>
 */

#ifndef _RBTREE_H_

#include <stdlib.h>

/* Public type definitions */
enum rbtree_node_color { RED, BLACK };

typedef struct rbtree_node_t {
    void* key;
    size_t value;
    struct rbtree_node_t* left;
    struct rbtree_node_t* right;
    struct rbtree_node_t* parent;
    enum rbtree_node_color color;
} *rbtree_node;

typedef struct rbtree_t {
    rbtree_node root;
} *rbtree;

/* Public function prototypes */
rbtree rbtree_create();
void rbtree_destroy(rbtree t);
size_t rbtree_lookup(rbtree t, void* key);
size_t rbtree_lookup_np(rbtree t, void* key);
void *rbtree_lookup_pointer(rbtree t, void* key);
void rbtree_insert(rbtree t, void* key, size_t value);
void rbtree_delete(rbtree t, void* key);

#endif /* _RBTREE_H_ */
