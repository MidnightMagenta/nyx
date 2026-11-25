#include <sys/kernel.h>
#include <sys/list.h>
#include <sys/testing.h>

struct item {
    int              key;
    int              seq;
    struct list_head node;
};

static size_t check_list(struct test_state *state, struct list_head *head) {
    struct list_head *pos;
    size_t            fwd = 0, back = 0;

    for (pos = head->next; pos != head; pos = pos->next) {
        if (pos->next->prev != pos) {
            ADD_FAILURE("broken prev link after entry %ld", (long) fwd);
            return fwd;
        }
        if (++fwd > 100000) {
            ADD_FAILURE("list does not terminate");
            return fwd;
        }
    }
    for (pos = head->prev; pos != head; pos = pos->prev) {
        if (++back > fwd) { break; }
    }
    if (fwd != back) { ADD_FAILURE("forward length %ld, backward length %ld", (long) fwd, (long) back); }

    return fwd;
}

KERNEL_TEST(list_add_del) {
    LIST_HEAD(head);
    struct item  a = {.key = 1}, b = {.key = 2}, c = {.key = 3};
    struct item *it;
    int          keys[3], n = 0;

    EXPECT_TRUE(list_is_empty(&head));

    list_add(&b.node, &head);
    list_add(&a.node, &head);
    list_add_tail(&c.node, &head);
    EXPECT_FALSE(list_is_empty(&head));
    ASSERT_EQ(check_list(state, &head), 3);

    list_for_each_entry(it, &head, node) {
        keys[n++] = it->key;
    }
    EXPECT_EQ(keys[0], 1);
    EXPECT_EQ(keys[1], 2);
    EXPECT_EQ(keys[2], 3);
    EXPECT_EQ(list_first_entry(&head, struct item, node), &a);
    EXPECT_EQ(list_next_entry(&a, node), &b);

    list_del(&b.node);
    EXPECT_EQ(b.node.next, &b.node);
    EXPECT_EQ(b.node.prev, &b.node);
    ASSERT_EQ(check_list(state, &head), 2);
    EXPECT_EQ(a.node.next, &c.node);

    list_del(&a.node);
    list_del(&c.node);
    EXPECT_TRUE(list_is_empty(&head));
    EXPECT_EQ(check_list(state, &head), 0);
}

KERNEL_TEST(list_move_replace) {
    LIST_HEAD(h1);
    LIST_HEAD(h2);
    struct item a = {.key = 1}, b = {.key = 2}, c = {.key = 3}, r = {.key = 9};

    list_add_tail(&a.node, &h1);
    list_add_tail(&b.node, &h1);
    list_add_tail(&c.node, &h1);

    list_move(&c.node, &h2);
    list_move_tail(&a.node, &h2);
    ASSERT_EQ(check_list(state, &h1), 1);
    ASSERT_EQ(check_list(state, &h2), 2);
    EXPECT_EQ(list_first_entry(&h2, struct item, node), &c);
    EXPECT_EQ(h2.prev, &a.node);

    list_move(&b.node, &h1);
    EXPECT_EQ(check_list(state, &h1), 1);

    list_replace(&b.node, &r.node);
    ASSERT_EQ(check_list(state, &h1), 1);
    EXPECT_EQ(list_first_entry(&h1, struct item, node), &r);

    list_move_tail(&r.node, &h2);
    EXPECT_TRUE(list_is_empty(&h1));
    EXPECT_EQ(check_list(state, &h2), 3);
}

KERNEL_TEST(list_for_each_safe_delete) {
    LIST_HEAD(head);
    struct item  items[8];
    struct item *it, *tmp;
    int          remaining = 0;

    for (int i = 0; i < 8; i++) {
        items[i].key = i;
        list_add_tail(&items[i].node, &head);
    }

    list_for_each_entry_safe(it, tmp, &head, node) {
        if (it->key % 2) { list_del(&it->node); }
    }

    ASSERT_EQ(check_list(state, &head), 4);
    list_for_each_entry(it, &head, node) {
        EXPECT_EQ(it->key % 2, 0);
        remaining++;
    }
    EXPECT_EQ(remaining, 4);
}

KERNEL_TEST(list_container_of) {
    struct item it;

    EXPECT_EQ(container_of(&it.node, struct item, node), &it);
    EXPECT_EQ(list_entry(&it.node, struct item, node), &it);
    EXPECT_EQ(container_of(&it.key, struct item, key), &it);
}

static int cmp_items(void *priv, const struct list_head *a, const struct list_head *b) {
    int *calls = priv;

    (*calls)++;
    return list_entry(a, struct item, node)->key - list_entry(b, struct item, node)->key;
}

static void sort_and_check(struct test_state *state, struct item *items, int n, unsigned int seed, int nkeys) {
    LIST_HEAD(head);
    struct item *it, *prev = NULL;
    int          calls = 0;

    for (int i = 0; i < n; i++) {
        seed         = seed * 1103515245u + 12345u;
        items[i].key = (int) ((seed >> 16) % (unsigned int) nkeys);
        items[i].seq = i;
        list_add_tail(&items[i].node, &head);
    }

    list_sort(&calls, &head, cmp_items);

    if (check_list(state, &head) != (size_t) n) {
        ADD_FAILURE("sorted list of %d has the wrong length", n);
        return;
    }

    list_for_each_entry(it, &head, node) {
        if (prev && prev->key > it->key) {
            ADD_FAILURE("n=%d: out of order: %d before %d", n, prev->key, it->key);
            return;
        }
        if (prev && prev->key == it->key && prev->seq > it->seq) {
            ADD_FAILURE("n=%d: not stable: seq %d before %d for key %d", n, prev->seq, it->seq, it->key);
            return;
        }
        prev = it;
    }

    if (n < 2) { EXPECT_EQ(calls, 0); }
}

KERNEL_TEST(list_sort) {
    static struct item items[300];
    LIST_HEAD(empty);
    int calls = 0;

    list_sort(&calls, &empty, cmp_items);
    EXPECT_TRUE(list_is_empty(&empty));

    for (int n = 1; n <= 40; n++) { ASSERT_NO_FAILURE(sort_and_check(state, items, n, (unsigned int) n, 1000)); }

    /* lots of duplicates to exercise stability */
    ASSERT_NO_FAILURE(sort_and_check(state, items, 300, 7, 5));
    ASSERT_NO_FAILURE(sort_and_check(state, items, 257, 99, 1000000));
    ASSERT_NO_FAILURE(sort_and_check(state, items, 128, 3, 1));
}

KERNEL_TEST(list_sort_presorted) {
    static struct item items[64];
    LIST_HEAD(head);
    struct item *it;
    int          calls = 0, expect;

    for (int i = 0; i < 64; i++) {
        items[i].key = 63 - i;
        list_add_tail(&items[i].node, &head);
    }

    list_sort(&calls, &head, cmp_items);
    ASSERT_EQ(check_list(state, &head), 64);
    expect = 0;
    list_for_each_entry(it, &head, node) {
        EXPECT_EQ(it->key, expect++);
    }

    list_sort(&calls, &head, cmp_items);
    ASSERT_EQ(check_list(state, &head), 64);
    expect = 0;
    list_for_each_entry(it, &head, node) {
        EXPECT_EQ(it->key, expect++);
    }
}
