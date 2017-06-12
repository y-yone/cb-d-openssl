#ifndef HEADER_CB_H
# define HEADER_CB_H

# ifdef  __cplusplus
extern "C" {
# endif

typedef int (*CB_TASK_FUNC)(void* ctx, void* data);
typedef int (*CB_FORCE_FREE_FUNC)(void* data);

typedef struct task_data_st CB_TASK;
typedef struct cb_ctx_st CB_CTX;

CB_CTX* CB_new(void* ctx);
void CB_free(CB_CTX *cb);
int CB_add_task(struct cb_ctx_st *ctx, void *data, CB_TASK_FUNC task_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head);
int CB_task_init(void);

# ifdef  __cplusplus
}
# endif
#endif
