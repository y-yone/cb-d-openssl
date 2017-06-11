
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "cb_locl.h"
#include <openssl/cb.h>

static pthread_t pt;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static list_head task_head;

static void* cb_task_func(void* dummy) {
    CB_CTX    *cb;
    CB_TASK   *task;
    CB_TASK   *next_task;
    CB_CTX    *ctx;
    TASK_FUNC task_func;
    void      *data;
    int       ret = 0;

    for(;;) {

    	pthread_mutex_lock(&lock);
        if( list_is_empty( &task_head ) ) {
        	pthread_mutex_unlock(&lock);
            sleep(1);
            continue;
        }
        cb = (CB_CTX*)(list_entry(&task_head));
        ctx = cb->ctx;
        task = cb->task;

        data = task->data;
        task_func = task->task_func;

        if( !list_is_empty(&task->list) ) {
            next_task = (CB_TASK*)task->list.next;
            cb->task = next_task;
            list_add_tail(&task_head, &cb->list);
        }
        pthread_mutex_unlock(&lock);

        free(task);
        ret = task_func(ctx, data);

    }

    return NULL;
}

int register_task(CB_CTX *ctx, void *data, TASK_FUNC task_func, int add_head) {
    CB_TASK *task;

    task = (CB_TASK *)malloc(sizeof(CB_TASK));
    if( task == NULL ) {
        return -ENOMEM;
    }
    task->data = data;
    task->task_func = task_func;
    list_init(&task->list);

    pthread_mutex_lock(&lock);
    if( ctx->task == NULL ) {
        ctx->task = task;
    }
    else {
        list_add_tail( &ctx->task->list, &task->list );
    }

    ctx->task->data = data;
    if( list_is_empty(&ctx->list) ) {
        list_add_tail(&task_head, &ctx->list);
    }
    else if(add_head){
        list_add_head(&ctx->task->list, &task->list);
    }
    else {
        list_add_tail(&ctx->task->list, &task->list);
    }
    pthread_mutex_unlock(&lock);

    return 0;
}

int task_init(void) {
    int ret;
    list_init(&task_head);
    ret = pthread_create(&pt, NULL, cb_task_func, NULL);

    return ret;
}
