#ifndef HEADER_CB_H
# define HEADER_CB_H

# ifdef  __cplusplus
extern "C" {
# endif

typedef int (*TASK_FUNC)(void* ctx, void* data);

typedef struct task_data_st CB_TASK;
typedef struct cb_ctx_st CB_CTX;

int register_task(struct cb_ctx_st *ctx, void *data, TASK_FUNC task_func, int add_head);
int task_init(void);

# ifdef  __cplusplus
}
# endif
#endif
