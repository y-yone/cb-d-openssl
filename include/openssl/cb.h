#ifndef HEADER_CB_H
# define HEADER_CB_H

# ifdef  __cplusplus
extern "C" {
# endif

typedef int (*CB_TASK_FUNC)(void* ctx, void* data);
typedef int (*CB_WORK_FUNC)(void* ctx, void* data);
typedef void (*CB_FORCE_FREE_FUNC)(void* data);

typedef struct task_data_st CB_TASK;
typedef struct cb_task_ctx_st CB_TASK_CTX;

typedef struct work_data_st CB_WORK;
typedef struct cb_work_ctx_st CB_WORK_CTX;

typedef struct cb_ctx_st CB_CTX;

CB_CTX* CB_new(void* ctx);
void CB_free(CB_CTX *cb);
int CB_add_task(CB_CTX *ctx, void *data, CB_TASK_FUNC task_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head);
int CB_add_work(CB_CTX *ctx, void *data, CB_WORK_FUNC work_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head);
int CB_task_init(void);
int CB_work_init(void);

# ifdef  __cplusplus
}
# endif
#endif
