
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "cb_locl.h"
#include <openssl/cb.h>

static pthread_t pt;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static list_head work_head;

static void* cb_work_func(void* dummy)
{
    CB_WORK_CTX  *cb;
    CB_WORK      *work;
    CB_WORK_FUNC work_func;
    void         *ctx;
    void         *data;
    int          ret = 0;

    for(;;) {

    	pthread_mutex_lock(&lock);
        cb = (CB_WORK_CTX*)(list_get_next(&work_head));
        pthread_mutex_unlock(&lock);

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

int CB_add_work(CB_WORK_CTX *ctx, void *data, CB_WORK_FUNC work_func,
        CB_FORCE_FREE_FUNC force_free_func, int add_head)
{
    CB_WORK *work;

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

    pthread_mutex_lock(&lock);
    list_add_tail( &work_head, &ctx->list );
    pthread_mutex_unlock(&lock);

    return 0;
}

int CB_work_init(void)
{
    int ret;
    list_init(&work_head);
    ret = pthread_create(&pt, NULL, cb_work_func, NULL);

    return ret;
}

CB_WORK_CTX* CB_WORK_new(void *ctx)
{
    CB_WORK_CTX *cb;

    cb = (CB_WORK_CTX*)malloc(sizeof(CB_WORK_CTX));
    if( cb==NULL )
        return NULL;

    cb->work_state = CB_WORK_STATE_NONE;
    pthread_spin_init(&cb->work_lock, PTHREAD_PROCESS_SHARED);
    list_init(&cb->list);
    list_init(&cb->work_head);


    return cb;
}

void CB_WORK_free(CB_WORK_CTX *cb)
{
    CB_WORK             *work, *next_work;
    CB_FORCE_FREE_FUNC  free_func;
    void                *data;

    pthread_mutex_lock(&lock);
    list_del_init(&cb->list);
    pthread_mutex_unlock(&lock);

    work = (CB_WORK*)list_get_next(&cb->work_head);

    while( work!=NULL ) {
        free_func = work->force_free_func;
        next_work = list_get_next(&cb->work_head);
        data = work->data;

        if( free_func!=NULL )
            free_func(data);

        free(work);
        work = next_work;
    }

    free(cb);
}
