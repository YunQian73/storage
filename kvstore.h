#ifndef _KVSOTRE_H_
#define _KVSOTRE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

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

#define ENABLE_KVSTORE_LOG  0

enum{
    KVS_CMD_START = 0,
    KVS_CMD_SET = KVS_CMD_START,
    KVS_CMD_GET,
    KVS_CMD_DEL,
    KVS_CMD_MOD,
    KVS_CMD_ACOUNT,

    KVS_CMD_RSET,
    KVS_CMD_RGET,
    KVS_CMD_RDEL,
    KVS_CMD_RMOD,
    KVS_CMD_RCOUNT,

    KVS_CMD_HSET,
    KVS_CMD_HGET,
    KVS_CMD_HDEL,
    KVS_CMD_HMOD,
    KVS_CMD_HCOUNT,

    KVS_CMD_SSET, 
    KVS_CMD_SGET, 
    KVS_CMD_SDEL,
    KVS_CMD_SMOD,
    KVS_CMD_SCOUNT,

    KVS_CMD_COUNT
};

// kvstore_persist.c的函数
typedef struct KvRecoverResult{
    int cmd_count;      // 命令的个数
    char **cmds;        // 存放具体的命令
} KvRecoverResult_t;

// 在重启服务端的时候将文件中的数据读取出来，返回一个KvRecoverResult_t
KvRecoverResult_t *kvstore_recover_data(const char* file);

// 把读出来的指令重新执行
int kvstore_execute_recovered_cmds(const char* file);

// 释放分配KvRecoverResult的内存
void kvstore_free_recover_result(KvRecoverResult_t *recover_result);

// 当执行DEL时删除日志中的SET信息
int delete_log_cmd(const char *log_file, const char *cmd_prefix, char *key);

// 执行MOD时修改日志中的SET
int modify_log_cmd(const char *log_file, const char *cmd_prefix, char *key, char *value);

// 增量持久化的实现
void kvstore_aof_append(const char *filename, const char *content);

#define KVSTORE_MAX_TOKENS 128  // 单条指令最大可分tokens数量
#define KVSTORE_MAX_CMDS 100    // 结构体最大存放命令个数
#define KVSTORE_BUF_SIZE 256    // 相应缓冲区大小

// kvstore.c的函数
/**
 * @brief aof过滤出写命令,如SET、MOD、DEL...
 * 
 * @param cmd 该命令的枚举值
 * @return 1--写命令，需要持久化  0--读命令，不需要持久化
 */
int kvstore_is_write_cmd(int cmd);

/**
 * @brief aof命令序列化
 * 
 * @param tokens 解析完成的命令参数
 * @param count 参数数量
 * @param buf 存放序列化结果的缓冲区
 * @param capacity 缓冲区容量
 * @return int 大于0--实际序列化得到的长度  小于0--序列化失败
 */
int kvstore_serialize_cmd(char **tokens, int count, char *buf, size_t capacity);

int kvstore_split_token(char* msg, char** tokens);
int kvstore_parser_protocol(char **tokens, int count, char *response);
int kvstore_request(char *msg, int count, char *response);
void init_kvengine(void);
void exit_kvengine(void);

typedef int (*msg_handler)(char *msg, int length, char *response);

int epoll_entry(msg_handler handler);
int ntyco_entry(msg_handler handler);

int init_ctx(void);

void *kvstore_malloc(size_t size);
void kvstore_free(void *ptr);

#define NETWORK_EPOLL   0
#define NETWORK_NTYCO   1
#define NETWORK_IOURING 2

#define ENABLE_ARRAY_KVENGINE 1
#define ENABLE_RBTREE_KVENGINE 1
#define ENABLE_HASH_KVENGINE 1
#define ENABLE_SKIPLIST_KVENGINE 1
#define ENABLE_NETWORK_SELECT NETWORK_EPOLL

#define ENABLE_MEM_POOL 0
#define ENABLE_PERSIST 1

#if ENABLE_PERSIST

int kvstore_persist_init(const char *file_name);
int kvstore_persist_append(const char *buf, size_t len);
int kvstore_persist_sync(void);
void kvstore_persist_destroy(void);

#endif


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

typedef struct array_s{
    struct kvs_array_item *array_table;
    int array_idx;
}array_t;

extern array_t Array;

int kvstore_array_create(array_t *arr);
int kvstore_array_destory(array_t *arr);
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

#if ENABLE_HASH_KVENGINE

#define ENABLE_KEY_POINTER	1


typedef struct hashnode_s {
#if ENABLE_KEY_POINTER
	char *key;
	char *value;
#else
	char key[MAX_KEY_LEN];
	char value[MAX_VALUE_LEN];
#endif
	struct hashnode_s *next;
	
} hashnode_t;


typedef struct hashtable_s {

	hashnode_t **nodes; //* change **, 

	int max_slots;
	int count;

} hashtable_t;

typedef struct hashtable_s kvs_hash_t;

extern kvs_hash_t Hash;

int kvstore_hash_create(kvs_hash_t *hash);
void kvstore_hash_destory(kvs_hash_t *hash);
int kvstore_hash_set(char *key, char *value);
char * kvstore_hash_get(char *key);
int kvstore_hash_modify(char *key, char *value);
int kvstore_hash_delete(char *key);
int kvstore_hash_count(void);

#endif

#if ENABLE_SKIPLIST_KVENGINE

typedef struct Node{
    char *key;
    char *value;
    struct Node ** forward;
}Node;

typedef struct SkipList{
    int level;
    int count;
    Node *hander;
}SkipList;

typedef struct SkipList skiptable_t;

extern skiptable_t Skiptable;

int kvstore_skiplist_create(skiptable_t *table);
void kvstore_skiplist_destory(skiptable_t *table);
int kvstore_skiplist_set(char *key, char *value);
char * kvstore_skiplist_get(char *key);
int kvstore_skiplist_modify(char *key, char *value);
int kvstore_skiplist_delete(char *key);
int kvstore_skiplist_count(void);


#endif

#endif