#include "header/list.h"
#include "header/interrupt.h"

void list_init(list *plist)
{
    plist->head.prev = NULL;
    plist->tail.next = NULL;
    plist->head.next = &plist->tail;
    plist->tail.prev = &plist->head;
}

void list_insert_before(list_node *before, list_node *elm)
{
    enum intr_status old_status = intr_disable();
    before->prev->next = elm;
    elm->prev = before->prev;
    elm->next = before;
    before->prev = elm;
    intr_set_status(old_status);
}

void list_push(list *plist, list_node *elm)
{
    list_insert_before(plist->head.next, elm);
}

void list_append(list *plist, list_node *elm)
{
    list_insert_before(&plist->tail, elm);
}

void list_remove(list_node *pelm)
{
    enum intr_status old_status = intr_disable();
    pelm->prev->next = pelm->next;
    pelm->next->prev = pelm->prev;
    intr_set_status(old_status);
}

list_node *list_pop(list *plist)
{
    list_node *elm = plist->head.next;
    list_remove(elm);
    return elm;
}

uint8_t list_empty(list *plist)
{
    return (plist->head.next == &plist->tail);
}

uint8_t elem_find(list *plist, list_node *obj_elem)
{
    list_node *elm = plist->head.next;
    while (elm != &plist->tail)
    {
        if (elm == obj_elem)
        {
            return TRUE;
        }
        elm = elm->next;
    }
    return FALSE;
}

list_node *list_traversal(list *plist, function func, int arg)
{
    if (list_empty(plist))
    {
        return NULL;
    }
    list_node *elm = plist->head.next;
    while (elm != &plist->tail)
    {
        if (func(elm, arg))
        {
            return elm;
        }
        elm = elm->next;
    }
    return NULL;
}

uint32_t list_len(list *plist)
{
    uint32_t length = 0;
    list_node *elm = plist->head.next;
    while (elm != &plist->tail)
    {
        length++;
        elm = elm->next;
    }
    return length;
}
