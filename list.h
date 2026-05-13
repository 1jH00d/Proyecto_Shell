// -----------------------------------------------------------------------------
// UNIX Shell Project
// Naive list implementation
// -----------------------------------------------------------------------------

#ifndef _NAIVE_LIST_H
#define _NAIVE_LIST_H


// ---------- TYPES FOR LIST ---------------------------------------------------
typedef struct item_t {
    void *data;
    struct item_t *next;
} list_item_t;

typedef struct {
    int count;
    char *name;
    list_item_t* first;
} list_head_t;

//------------------------------------------------------------------------------
// list_head_t                             [Job or other]
// +-------+   +---> "List of jobs"  +---->+------------------------------+
// | count |   |                     |     | content of the first element |
// +-------+   |       list_item_t   |     +------------------------------+
// | name  |---+  +--->+------+      |   +--->+------+
// +-------+      |    | data |------+   |    | data |-------->+--------------+
// | first |------+    +------+          |    +------+         |content of 2nd|
// +-------+           | next |----------+    | next |--->NULL +--------------+
//                     +------+               +------+
//                     first item             second item
//                    (last inserted)        (first inserted)
//------------------------------------------------------------------------------

// Functions for list management:
list_head_t *new_list(char *);
int insert_item(list_head_t *, void *);
int remove_item(list_head_t *, void *);
unsigned int find_item(list_head_t *, void *);
void traverse_list(list_head_t *, void (*)(void *, unsigned int));
void * get_item_bypos(list_head_t *, unsigned int);
void * get_item_byfunc(list_head_t *, int(*check)(void*));

#define list_size(list)    (list->count)    // number of items in the list
#define empty_list(list)   (!(list->count)) // returns 1 (true) on empty list
#define list_name(list)    (list->name)

// -----------------------------------------------------------------------
#endif

