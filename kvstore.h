#ifndef _KVSOTRE_H_
#define _KVSOTRE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stddef.h>

#define BUFFER_LENGTH 512

// #define ENABLE_LOG 1

#ifdef ENABLE_LOG

#define LOG(_fmt, ...) fprintf(stdout, "[%s:%d]: " _fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#else

#define LOG(_fmt, ...)

#endif

typedef int(*RCALLBACK)(int fd);

struct conn_item{
    int fd;

    char rbuffer[BUFFER_LENGTH];
    int rlen;
    char wbuffer[BUFFER_LENGTH];
    int wlen;

    char resource[BUFFER_LENGTH]; // abc.html

    union{
        RCALLBACK accept_callback;
        RCALLBACK recv_callback;
    } recv_t;
    RCALLBACK send_callback;
};

int epoll_entry(void);
int ntyco_entry(void);

int kvstore_request(struct conn_item *item);

void *kvstore_malloc(size_t size);
void kvstore_free(void *ptr);

#define NETWORK_EPOLL   0
#define NETWORK_NTYCO   1
#define NETWORK_IOURING 2

#define ENABLE_ARRAY_KVENGINE 1
#define ENABLE_RBTREE_KVENGINE 1
#define ENABLE_NETWORK_SELECT NETWORK_NTYCO

#define ENABLE_MEM_POOL 1


#if ENABLE_MEM_POOL

// 针对固定大小内存块设计内存池
typedef struct mempool_s{
    int block_size; // 整块的大小
    int free_count; // 剩余可分配的块数

    char *free_ptr; // 指向下一次分配的位置
    char *mem; // 指向整块内存的指针
}mempool_t;

int mp_init(mempool_t *m, int size);
void mp_dest(mempool_t* m);
void *mp_alloc(mempool_t* m);
void mp_free(mempool_t* m, void* ptr);

extern mempool_t m;

#endif

#if ENABLE_ARRAY_KVENGINE

struct kvs_array_item{
    char* key;
    char* value;
};

#define KVS_ARRAY_SIZE  1024

int kvstore_array_set(char *key, char *value);
char* kvstore_array_get(char *key);
int kvstore_array_delete(char *key);
int kvstore_array_modify(char *key, char *value);
int kvstore_array_count(void);

#endif

#if ENABLE_RBTREE_KVENGINE

typedef struct _rbtree rbtree_t;

extern rbtree_t Tree;

int kvstore_rbtree_create(rbtree_t* tree);
int kvstore_rbtree_destory(rbtree_t* tree);
int kvstore_rbtree_set(char *key, char *value);
char* kvstore_rbtree_get(char *key);
int kvstore_rbtree_delete(char *key);
int kvstore_rbtree_modify(char *key, char *value);
int kvstore_rbtree_count(void);

#endif

#endif