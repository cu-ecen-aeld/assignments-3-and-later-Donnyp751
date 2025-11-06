/*
 * linkedlist.c
 *
 * Simple generic singly linked list implementation.
 *
 */

#include <stdlib.h>
#include <stddef.h>

typedef struct ll_node {
    void *data;
    struct ll_node *next;
} ll_node;

typedef struct linked_list {
    ll_node *head;
    size_t size;
    void (*free_fn)(void *data);
} linked_list;

/* Create a new linked list. free_fn is used to free stored data on removal/destruction (may be NULL). */
linked_list *ll_create(void (*free_fn)(void *))
{
    linked_list *list = malloc(sizeof(*list));
    if (!list) return NULL;
    list->head = NULL;
    list->size = 0;
    list->free_fn = free_fn;
    return list;
}

/* Destroy list and free all nodes. If free_fn was provided it will be called for each data. */
void ll_destroy(linked_list *list)
{
    if (!list) return;
    ll_node *cur = list->head;
    while (cur) {
        ll_node *next = cur->next;
        if (list->free_fn && cur->data) list->free_fn(cur->data);
        free(cur);
        cur = next;
    }
    free(list);
}

/* Return number of elements in list. */
size_t ll_size(const linked_list *list)
{
    return list ? list->size : 0;
}

/* Push data at front. Returns 0 on success, -1 on allocation failure. */
int ll_push_front(linked_list *list, void *data)
{
    if (!list) return -1;
    ll_node *n = malloc(sizeof(*n));
    if (!n) return -1;
    n->data = data;
    n->next = list->head;
    list->head = n;
    list->size++;
    return 0;
}

/* Push data at back. Returns 0 on success, -1 on allocation failure. */
int ll_push_back(linked_list *list, void *data)
{
    if (!list) return -1;
    ll_node *n = malloc(sizeof(*n));
    if (!n) return -1;
    n->data = data;
    n->next = NULL;
    if (!list->head) {
        list->head = n;
    } else {
        ll_node *cur = list->head;
        while (cur->next) cur = cur->next;
        cur->next = n;
    }
    list->size++;
    return 0;
}

/* Pop and return data from front. Returns NULL if list empty. Caller owns returned data. */
void *ll_pop_front(linked_list *list)
{
    if (!list || !list->head) return NULL;
    ll_node *n = list->head;
    void *data = n->data;
    list->head = n->next;
    free(n);
    list->size--;
    return data;
}

/* Find first element matching comparator.
 * cmp should return 0 if element matches (like strcmp convention), non-zero otherwise.
 * ctx is passed through to cmp.
 * Returns pointer to data or NULL if not found.
 */
void *ll_find(linked_list *list, int (*cmp)(const void *data, const void *ctx), const void *ctx)
{
    if (!list || !cmp) return NULL;
    for (ll_node *cur = list->head; cur; cur = cur->next) {
        if (cmp(cur->data, ctx) == 0) return cur->data;
    }
    return NULL;
}

/* Remove first element matching comparator. Returns 1 if removed, 0 if not found, -1 on error.
 * If the list was created with a free_fn, the stored data will be freed.
 */
int ll_remove_if(linked_list *list, int (*cmp)(const void *data, const void *ctx), const void *ctx)
{
    if (!list || !cmp) return -1;
    ll_node *prev = NULL;
    ll_node *cur = list->head;
    while (cur) {
        if (cmp(cur->data, ctx) == 0) {
            if (prev) prev->next = cur->next;
            else list->head = cur->next;
            if (list->free_fn && cur->data) list->free_fn(cur->data);
            free(cur);
            list->size--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

/* Clear all elements but keep the list structure. */
void ll_clear(linked_list *list)
{
    if (!list) return;
    ll_node *cur = list->head;
    while (cur) {
        ll_node *next = cur->next;
        if (list->free_fn && cur->data) list->free_fn(cur->data);
        free(cur);
        cur = next;
    }
    list->head = NULL;
    list->size = 0;
}

/* Apply fn to each element's data. ctx is user-supplied context passed to fn. */
void ll_foreach(linked_list *list, void (*fn)(void *data, void *ctx), void *ctx)
{
    if (!list || !fn) return;
    for (ll_node *cur = list->head; cur; cur = cur->next) {
        fn(cur->data, ctx);
    }
}