#ifndef __KERNEL_LIB__LIST_H__
#define __KERNEL_LIB__LIST_H__

#include "header/stdint.h"
#include "header/globoal.h"

#define offset(struct_type, member) (int)(&((struct_type *)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type *)((int)elem_ptr - offset(struct_type, struct_member_name))

typedef struct list_node
{
    struct list_node *prev;
    struct list_node *next;
} list_node;

typedef struct list
{
    list_node head;
    list_node tail;
} list;

typedef uint8_t(function)(list_node *, int arg);

void list_init(list *plist);
void list_insert_before(list_node *before, list_node *elm);
void list_push(list *plist, list_node *elm);
void list_append(list *plist, list_node *elm);
void list_remove(list_node *pelm);
list_node *list_pop(list *plist);
uint8_t list_empty(list *plist);
uint32_t list_len(list *plist);
struct list_node *list_traversal(list *plist, function func, int arg);
uint8_t elem_find(list *plist, list_node *obj_elem);

#endif
