#ifndef _NYX_LIST_H
#define _NYX_LIST_H

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

struct list_head {
    struct list_head *next, *prev;
};

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    next->prev = new;
    prev->next = new;
    new->next  = next;
    new->prev  = prev;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

#endif
