#include <stdio.h>

struct list_head_st {
    struct list_head_st *next, *prev;
};

struct task_data_st {
    // list element is head of member.
    struct list_head_st list;
    void      *data;
    int (*task_func)(void* ctx, void* data);

};

struct cb_ctx_st {
    // list element is head of member.
    struct list_head_st list;
    void *ctx;
    struct task_data_st *task;
};

typedef struct list_head_st list_head;
typedef list_head list_t;

static inline int list_is_empty(struct list_head_st *list) {
    return (list == list->next);
}

static inline void list_init(struct list_head_st *list) {
    list->prev = list;
    list->next = list;
}

static inline void list_add_tail(struct list_head_st *head, struct list_head_st *list) {
    list->prev = head->prev;
    head->prev->next = list;
    head->prev = list;
    list->next = head;
}

static inline void list_add_head(struct list_head_st *head, struct list_head_st *list) {
    list->next = head->next;
    head->next->prev = list;
    head->next = list;
    list->prev = head;
}

static inline void list_del_init(struct list_head_st *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
    list->next = list;
    list->prev = list;
}

static inline struct list_head_st* list_entry( struct list_head_st *head ) {
    struct list_head_st *list;

    list = head->next;
    list_del_init(list);
    return list;
}

