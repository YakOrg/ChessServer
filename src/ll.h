#ifndef UNTITLED19_LL_H
#define UNTITLED19_LL_H
#include <pthread.h>

/* type definitions */

// useful for casting
typedef void (*gen_fun_t)(void *);

// linked list
typedef struct ll ll_t;

// linked list node
typedef struct ll_node ll_node_t;

// linked list
struct ll {
    // running length
    int len;

    // pointer to the first node
    ll_node_t *hd;

    // mutex for thread safety
    pthread_rwlock_t m;

    // a function that is called every time a value is deleted
    // with a pointer to that value
    gen_fun_t val_teardown;

    // a function that can print the values in a linked list
    gen_fun_t val_printer;
};

/* function prototypes */

// returns a pointer to an allocated linked list.
// needs a taredown function that is called with
// a pointer to the value when it is being deleted.
ll_t *ll_new(gen_fun_t val_teardown);

// traverses the linked list, deallocated everything (including `list`)
void ll_delete(ll_t *list);

// inserts a value into the linked list at position `n`. acceptable values for n are `0`
// (puts it in first) to `list->len` (puts it in last).
// returns the new length of the linked list if successful, -1 otherwise
int ll_insert_n(ll_t *list, void *val, int n);

void *ll_get_search(ll_t *list, int cond(void *, void*), void* args);

// puts a value at the front of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int ll_insert_first(ll_t *list, void *val);

// puts a value at the end of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int ll_insert_last(ll_t *list, void *val);

// removes the value at position n of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int ll_remove_n(ll_t *list, int n);

// removes the value at the front of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int ll_remove_first(ll_t *list);

// given a function that tests the values in the linked list, the first element that
// satisfies that function is removed.
// returns the new length of the linked list if successful, -1 otherwise
int ll_remove_search(ll_t *list, int cond(void *, void*), void* args);

// returns a pointer to the `n`th value in the linked list.
// returns `NULL` if unsuccessful
void *ll_get_n(ll_t *list, int n);

// returns a pointer to the first value in a linked list.
// `NULL` if empty
void *ll_get_first(ll_t *list);

// runs f on all values of list
void ll_map(ll_t *list, gen_fun_t f);

// goes through all the values of a linked list and calls `list->val_printer` on them
void ll_print(ll_t list);

// a generic taredown function for values that don't need anything done
void ll_no_teardown(void *n);

#endif //UNTITLED19_LL_H
