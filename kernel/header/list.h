#ifndef __KERNEL_LIB__LIST_H__
#define __KERNEL_LIB__LIST_H__
#include "glob.h"
#define offset(struct_type, member) (int)(&((struct_type *)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type *)((int)elem_ptr - offset(struct_type, struct_member_name))

struct list_node{

};

struct list{

};
#endif