#include "header/list.h"
#include "header/globoal.h"
void list_linit(list *plist){
    plist->head.prev=NULL;
    plist->tail.next=NULL;
    plist->head.next=&(plist->tail);
    plist->tail.prev=&(plist->head);
    return;
}