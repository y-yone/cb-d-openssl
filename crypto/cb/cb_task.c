
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "cb_locl.h"
#include <openssl/cb.h>

static pthread_t task_pt;
static pthread_mutex_t task_lock = PTHREAD_MUTEX_INITIALIZER;
static list_head task_head;

static pthread_t work_pt;
static pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;
static list_head work_head;

static void* cb_task_func(void* dummy)
{
    CB_TASK_CTX       *cb;
    CB_TASK      *task;
    CB_TASK      *next_task;
    CB_TASK_CTX       *ctx;
    CB_TASK_FUNC task_func;
    void         *data;
    int          ret = 0;

    for(;;) {

    	pthread_mutex_lock(&task_lock);
    	cb = (CB_TASK_CTX*)(list_get_next(&task_head));
        if( cb == NULL ) {
        	pthread_mutex_unlock(&task_lock);
            sleep(1);
            continue;
        }

        ctx = cb->ctx;
        task = cb->task;
        data = task->data;
        task_func = task->task_func;

        if( list_is_empty(&task->list) ) {
            cb->task = NULL;
        }
        else {
            next_task = (CB_TASK*)task->list.next;
            list_del_init(&task->list);
            cb->task = next_task;
            list_add_tail(&task_head, &cb->list);
        }

        pthread_mutex_unlock(&task_lock);

        free(task);
        ret = task_func(ctx, data);

    }

    return NULL;
}

static void* cb_work_func(void* dummy)
{
    CB_WORK_CTX  *cb;
    CB_WORK      *work;
    CB_WORK_FUNC work_func;
    void         *ctx;
    void         *data;
    int          ret = 0;

    for(;;) {

        pthread_mutex_lock(&work_lock);
        cb = (CB_WORK_CTX*)(list_get_next(&work_head));
        pthread_mutex_unlock(&work_lock);

        if( cb == NULL ) {
            sleep(1);
            continue;
        }

        pthread_spin_lock(&cb->work_lock);
        work = (CB_WORK*)list_get_next(&cb->work_head);

        if( work == NULL ) {
            cb->work_state = CB_WORK_STATE_NONE;
            pthread_spin_unlock(&cb->work_lock);
            continue;
        }

        ctx = work->ctx;
        data = work->data;
        work_func = work->work_func;

        pthread_spin_unlock(&cb->work_lock);

        free(work);
        ret = work_func(ctx, data);

    }

    return NULL;
}

static CB_TASK_CTX* CB_TASK_new(void *ctx)
{
    CB_TASK_CTX *task_ctx;

    task_ctx = (CB_TASK_CTX*)malloc(sizeof(CB_TASK_CTX));
    if(task_ctx == NULL)
        return NULL;

    task_ctx->ctx = ctx;
    task_ctx->task = NULL;
    list_init(&task_ctx->list);

    return task_ctx;
}

static void CB_TASK_free(CB_TASK_CTX *task_ctx)
{
    CB_TASK             *task, *next_task;
    CB_FORCE_FREE_FUNC  free_func;
    void                *data;

    pthread_mutex_lock(&task_lock);
    list_del_init(&task_ctx->list);
    pthread_mutex_unlock(&task_lock);

    task = task_ctx->task;

    while( task != NULL ) {
        free_func = task->force_free_func;
        next_task = (CB_TASK*)task->list.next;
        data = task->data;

        if( next_task == task ) {
            next_task = NULL;
        }
        else {
            list_del_init( &task->list );
        }

        if( free_func!=NULL )
            free_func(data);
        free(task);
        task = next_task;
    }
    free(task_ctx);
    return;
}

static CB_WORK_CTX* CB_WORK_new(void)
{
    CB_WORK_CTX *ctx;

    ctx = (CB_WORK_CTX*)malloc(sizeof(CB_WORK_CTX));
    if(ctx == NULL)
        return NULL;

    ctx->work_state = CB_WORK_STATE_NONE;
    pthread_spin_init(&ctx->work_lock, PTHREAD_PROCESS_SHARED);
    list_init(&ctx->list);
    list_init(&ctx->work_head);

    return ctx;
}

static void CB_WORK_free(CB_WORK_CTX *work_ctx)
{
    CB_WORK             *work, *next_work;
    CB_FORCE_FREE_FUNC  free_func;
    void                *data;

    pthread_mutex_lock(&work_lock);
    list_del_init(&work_ctx->list);
    pthread_mutex_unlock(&work_lock);

    work = (CB_WORK*)list_get_next(&work_ctx->work_head);

    while( work!=NULL ) {
        free_func = work->force_free_func;
        next_work = (CB_WORK*)list_get_next(&work_ctx->work_head);
        data = work->data;

        if( free_func!=NULL )
            free_func(data);

        free(work);
        work = next_work;
    }

    free(work_ctx);
    return;
}

int CB_init(void)
{
    int ret;
    list_init(&task_head);
    ret = pthread_create(&task_pt, NULL, cb_task_func, NULL);
    if( ret ) {
        return ret;
    }

    list_init(&work_head);
    ret = pthread_create(&work_pt, NULL, cb_work_func, NULL);
    return ret;
}

CB_CTX* CB_new(void *ctx)
{
    CB_CTX *cb;
    CB_TASK_CTX *task;
    CB_WORK_CTX *work;

    cb = (CB_CTX*)malloc(sizeof(CB_CTX));
    if(cb == NULL)
        return NULL;

    task = CB_TASK_new(ctx);
    if(task == NULL) {
        free(cb);
        return NULL;
    }

    work = CB_WORK_new();
    if(work == NULL) {
        free(cb);
        free(task);
        return NULL;
    }

    cb->parent_ctx = ctx;
    cb->task = task;
    cb->work = work;

    return cb;
}

void CB_free(CB_CTX *cb_ctx)
{
    CB_TASK_free(cb_ctx->task);
    CB_WORK_free(cb_ctx->work);
    free(cb_ctx);
}


int CB_add_task(CB_CTX *_ctx, void *data, CB_TASK_FUNC task_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head)
{
    CB_TASK *task;
    CB_TASK_CTX *ctx = _ctx->task;

    task = (CB_TASK *)malloc(sizeof(CB_TASK));
    if( task == NULL ) {
        return -ENOMEM;
    }
    task->data = data;
    task->task_func = task_func;
    list_init(&task->list);

    pthread_mutex_lock(&task_lock);
    if( list_is_empty(&ctx->list) ) {
        list_add_tail(&task_head, &ctx->list);
    }

    if( ctx->task == NULL ) {
        ctx->task = task;
    }
    else {
        if(add_head){
            list_add_head(&ctx->task->list, &task->list);
        }
        else {
            list_add_tail(&ctx->task->list, &task->list);
        }
    }
    pthread_mutex_unlock(&task_lock);

    return 0;
}

int CB_add_work(CB_CTX *_ctx, void *data, CB_WORK_FUNC work_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head)
{
    CB_WORK *work;
    CB_WORK_CTX *ctx = _ctx->work;

    work = (CB_WORK *)malloc(sizeof(CB_WORK));
    if( work == NULL ) {
        return -ENOMEM;
    }

    work->data = data;
    work->work_func = work_func;
    list_init(&work->list);

    pthread_spin_lock(&ctx->work_lock);
    if( ctx->work_state == CB_WORK_STATE_NONE )
        ctx->work_state = CB_WORK_STATE_WAIT;

    if( list_is_empty(&ctx->list) ) {
        list_add_tail(&work_head, &ctx->list);
    }
    if(add_head){
        list_add_head( &ctx->work_head, &work->list );
    }
    else {
        list_add_tail( &ctx->work_head, &work->list );
    }
    pthread_spin_unlock(&ctx->work_lock);

    pthread_mutex_lock(&work_lock);
    list_add_tail( &work_head, &ctx->list );
    pthread_mutex_unlock(&work_lock);

    return 0;
}


