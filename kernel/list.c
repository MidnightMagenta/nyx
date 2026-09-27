#include <nyx/list.h>

static struct list_head *merge(void *priv, list_cmp_t cmp, struct list_head *a, struct list_head *b) {
    struct list_head head, *tail = &head;

    while (a && b) {
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a          = a->next;
        } else {
            tail->next = b;
            b          = b->next;
        }
        tail = tail->next;
    }
    tail->next = a ? a : b;
    return head.next;
}

static void merge_final(void *priv, list_cmp_t cmp, struct list_head *head, struct list_head *a, struct list_head *b) {
    struct list_head *tail = head;

    while (a && b) {
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev    = tail;
            a          = a->next;
        } else {
            tail->next = b;
            b->prev    = tail;
            b          = b->next;
        }
        tail = tail->next;
    }

    struct list_head *rest = a ? a : b;
    while (rest) {
        tail->next = rest;
        rest->prev = tail;
        tail       = rest;
        rest       = rest->next;
    }

    tail->next = head;
    head->prev = tail;
}

void list_sort(void *priv, struct list_head *head, list_cmp_t cmp) {
    struct list_head *cur;
    struct list_head *carry[32] = {0};
    struct list_head *result    = NULL;
    int               num_carries, i, j;

    if (head->next == head || head->next->next == head) { return; }

    cur              = head->next;
    num_carries      = 0;
    head->prev->next = NULL;


    while (cur != NULL) {
        struct list_head *node = cur;
        cur                    = cur->next;
        node->next             = NULL;

        for (i = 0; i < num_carries && carry[i] != NULL; i++) {
            node     = merge(priv, cmp, carry[i], node);
            carry[i] = NULL;
        }

        if (i == num_carries) { num_carries++; }
        carry[i] = node;
    }

    for (i = 0; i < num_carries; i++) {
        if (carry[i] == NULL) { continue; }
        if (result == NULL) {
            result = carry[i];
        } else {
            for (j = i + 1; j < num_carries && carry[j] == NULL; j++);
            if (j == num_carries) {
                merge_final(priv, cmp, head, carry[i], result);
                return;
            }
        }
    }

    merge_final(priv, cmp, head, result, NULL);
}
