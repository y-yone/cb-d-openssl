
#include <stdio.h>

typedef int (*TASK_FUNC)(void* ctx, void* data);

typedef struct cb_ctx_st cb_ctx;

typedef struct list_head_st {
    struct list_head_st *next, *prev;
} list_head;

typedef list_head list_t;

typedef struct task_data_st {
    // list element is head of member.
    list_t list;
    void      *data;
    TASK_FUNC task_func;

} task_data;

typedef struct cb_ctx_st {
    // list element is head of member.
    list_t list;
    void *ctx;
    task_data *task;
} cb_ctx;

static inline int list_is_empty(list_t *list) {
    return (list == list->next);
}

static inline void list_init(list_t *list) {
    list->prev = list;
    list->next = list;
}

static inline void list_add_tail(list_t *head, list_t *list) {
    list->prev = head->prev;
    head->prev->next = list;
    head->prev = list;
    list->next = head;
}

static inline void list_add_head(list_t *head, list_t *list) {
    list->next = head->next;
    head->next->prev = list;
    head->next = list;
    list->prev = head;
}

static inline void list_del_init(list_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
    list->next = list;
    list->prev = list;
}

static inline list_t* list_entry( list_t *head ) {
    list_t *list;

    list = head->next;
    list_del_init(list);
    return list;
}

int register_task(cb_ctx *ctx, void *data, TASK_FUNC task_func, int add_head);
int task_init(void);

