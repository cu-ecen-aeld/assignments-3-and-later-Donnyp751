#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ll_node {
    void *data;
    struct ll_node *next;
} ll_node_t;

typedef struct linked_list {
    ll_node_t *head;
    ll_node_t *tail;
    size_t size;
} linked_list_t;

/* Create/destroy */
linked_list_t *ll_create(void);
void ll_destroy(linked_list_t *list, void (*free_fn)(void *));
void ll_clear(linked_list_t *list, void (*free_fn)(void *));

/* Properties */
size_t ll_size(const linked_list_t *list);
bool ll_is_empty(const linked_list_t *list);

/* Insert/remove */
int ll_push_front(linked_list_t *list, void *data);
int ll_push_back(linked_list_t *list, void *data);
void *ll_pop_front(linked_list_t *list);
void *ll_pop_back(linked_list_t *list);

/* Find/remove by comparator: cmp(a, b) -> 0 when equal */
void *ll_find(const linked_list_t *list, int (*cmp)(const void *a, const void *b), const void *key);
int ll_remove_if(linked_list_t *list, int (*cmp)(const void *a, const void *b), const void *key, void (*free_fn)(void *));

/* Iterate: callback receives (data, arg) */
void ll_foreach(const linked_list_t *list, void (*fn)(void *data, void *arg), void *arg);

/* Convert to array (returns malloc'd array of void*; out_count set to size) */
void **ll_to_array(const linked_list_t *list, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* LINKEDLIST_H */