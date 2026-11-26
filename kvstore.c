#include "kvstore.h"


const char* commands[] = {
    "SET", "GET", "DEL", "MOD", "ACOUNT",
    "RSET", "RGET", "RDEL", "RMOD", "RCOUNT",
    "HSET", "HGET", "HDEL", "HMOD", "HCOUNT",
    "SSET", "SGET", "SDEL", "SMOD", "SCOUNT"
};

#if ENABLE_PERSIST

static int persist_error = 0;

#endif




void *kvstore_malloc(size_t size){
#if ENABLE_MEM_POOL

#else
    return malloc(size);
#endif
}

void kvstore_free(void *ptr){
#if ENABLE_MEM_POOL

#else
    return free(ptr);
#endif
}

// 1 写命令  0 读命令
int kvstore_is_write_cmd(int cmd){
    switch(cmd){
        case KVS_CMD_SET:
        case KVS_CMD_MOD:
        case KVS_CMD_DEL:

        case KVS_CMD_RSET:
        case KVS_CMD_RMOD:
        case KVS_CMD_RDEL:

        case KVS_CMD_HSET:
        case KVS_CMD_HMOD:
        case KVS_CMD_HDEL:

        case KVS_CMD_SSET:
        case KVS_CMD_SMOD:
        case KVS_CMD_SDEL:
            return 1;
        default:
            return 0;
    }
}


int kvstore_serialize_cmd(char **tokens, int count, char *buf, size_t capacity){
    if(tokens == NULL || count < 0 || buf == NULL || capacity == 0){
        fprintf(stderr, "parameters cannot be empty");
        return -1;
    }

    size_t offset = 0;
    for(int i = 0; i < count; i++){
        if(tokens[i] == NULL){
            fprintf(stderr, "token[%d] is NULL\n", i);
            return -1;
        }
        // 在tokens间加上分隔符
        int n = snprintf(buf + offset, capacity - offset, "%s%s", i == 0 ? "" : " ", tokens[i]);
        if(n < 0 || (size_t)n >= capacity - offset){
            fprintf(stderr, "缓冲区大小不足或写入数据失败\n");
            return -1;
        }
        offset += (size_t)n;
    }

    // 这里的n为"\r\n"的长度，恒为2
    int n = snprintf(buf + offset, capacity - offset, "\r\n");
    if(n < 0 || (size_t)n >= capacity - offset){
        fprintf(stderr, "缓冲区大小不足或写入数据失败\n");
        return -1;
    }
    offset += (size_t)n;

    return (int)offset;
}

int kvstore_split_token(char* msg, char** tokens){
    // 初始化后tokens[i]为空，但是tokens不应为空
    if(msg == NULL || tokens == NULL) return -1;

    int idx = 0;

    char* token = strtok(msg, " \r\n");

    while(token != NULL){
        if(idx >= KVSTORE_MAX_TOKENS){
            return -1;
        }
        tokens[idx++] = token;
        token = strtok(NULL, " \r\n");
    }
    return idx;
}

int kvstore_parser_protocol(char **tokens, int count, char *response){
    if(tokens[0] == NULL || response == NULL || count == 0) return -1;

    int cmd = KVS_CMD_START;

    for(cmd = KVS_CMD_START; cmd < KVS_CMD_COUNT; cmd++){
        if(strcmp(commands[cmd], tokens[0]) == 0){
            break;
        }
    }

    // 存放返回的文件描述符
    int log_file_fd = -1;

    int write_success = 0;

    int length = 0;
    char* key = tokens[1];
    char* value = tokens[2];

    switch(cmd){
        case KVS_CMD_SET:{
            int res = kvstore_array_set(key, value);

            if(res == 0){
                write_success = 1;
                length = sprintf(response, "SUCCESS\r\n");
            }
            else if(res < 0){
                length = sprintf(response, "FATLED\r\n");
            }
            else{
                length = sprintf(response, "EXIST\r\n");
            }

            break;
        }
        case KVS_CMD_GET:{
            char *val = kvstore_array_get(key);

            if(val){
                length = sprintf(response, "%s\r\n", val);
            }
            else{
                length = sprintf(response, "NO EXIST\r\n");
            }

            break;
        }
        case KVS_CMD_DEL:{
            int res = kvstore_array_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }
            break;
        }
        case KVS_CMD_MOD:{
            int res = kvstore_array_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }          
            break;
        }
        case KVS_CMD_ACOUNT:{
            int count = kvstore_array_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else{
                length = sprintf(response, "%d", count);               
            }  
            break;
        }
        case KVS_CMD_RSET:{
            // printf("rset:%s\n", key);
            int res = kvstore_rbtree_set(key, value);
            if(res == 0){
                write_success = 1;
                length = sprintf(response, "SUCCESS\r\n");
            }
            else if(res < 0){
                length = sprintf(response, "FATLED\r\n");
            }
            else{
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        }
        case KVS_CMD_RGET:{
            char *val = kvstore_rbtree_get(key);
            if(val){
                length = sprintf(response, "%s", val);
            }
            else{
                length = sprintf(response, "NO EXIST\r\n");
            }
            printf("rget: %s\n", val);
            break;
        }
        case KVS_CMD_RDEL:{
            // printf("rdel:%s\n", key);
            int res = kvstore_rbtree_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }
            break;
        }
        case KVS_CMD_RMOD:{
            // printf("rmod:%s\n", key);
            int res = kvstore_rbtree_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");               
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            } 
            break;
        }  
        case KVS_CMD_RCOUNT:{
            int count = kvstore_rbtree_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else{
                length = sprintf(response, "%d", count);               
            }  
            break;
        }
        case KVS_CMD_HSET:{
            // LOG("set\n");
            int res = kvstore_hash_set(key, value);
            if(res == 0){
                write_success = 1;
                length = sprintf(response, "SUCCESS\r\n");
            }
            else if(res < 0){
                length = sprintf(response, "FATLED\r\n");
            }
            else{
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        }
        case KVS_CMD_HGET:{
            // LOG("get\n");
            char *val = kvstore_hash_get(key);
            if(val){
                length = sprintf(response, "%s\r\n", val);
            }
            else{
                length = sprintf(response, "NO EXIST\r\n");
            }
            break;
        }
        case KVS_CMD_HDEL:{
            // LOG("del\n");
            int res = kvstore_hash_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }
            break;
        }
        case KVS_CMD_HMOD:{
            // LOG("mod\n");
            int res = kvstore_hash_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }          
            break;
        }
        case KVS_CMD_HCOUNT:{
            int count = kvstore_hash_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else{
                length = sprintf(response, "%d", count);               
            }  
            break;
        }
        case KVS_CMD_SSET:{
            // LOG("set\n");
            int res = kvstore_skiplist_set(key, value);
            if(res == 0){
                write_success = 1;
                length = sprintf(response, "SUCCESS\r\n");
            }
            else if(res < 0){
                length = sprintf(response, "FATLED\r\n");
            }
            else{
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        }
        case KVS_CMD_SGET:{
            // LOG("get\n");
            char *val = kvstore_skiplist_get(key);
            if(val){
                length = sprintf(response, "%s\r\n", val);
            }
            else{
                length = sprintf(response, "NO EXIST\r\n");
            }
            break;
        }
        case KVS_CMD_SDEL:{
            // LOG("del\n");
            int res = kvstore_skiplist_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }
            break;
        }
        case KVS_CMD_SMOD:{
            // LOG("mod\n");
            int res = kvstore_skiplist_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else if(res == 0){
                write_success = 1;
                length = sprintf(response, "%s", "SUCCESS\r\n");
            }
            else{
                length = sprintf(response, "NO EXITS\r\n");
            }          
            break;
        }
        case KVS_CMD_SCOUNT:{
            int count = kvstore_skiplist_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR\r\n");
            }
            else{
                length = sprintf(response, "%d", count);               
            }  
            break;
        }    
        default:{
            LOG("cmd: %s\n", commands[cmd]);
            assert(0);
        }
            
    }
#if ENABLE_PERSIST

    char cmd_buf[1024] = {0};

    if (persist_error && kvstore_is_write_cmd(cmd)) {

        length = sprintf(response, "AOF ERROR: WRITES DISABLED\r\n");
    }

    if(kvstore_is_write_cmd(cmd) && write_success){
        int n = kvstore_serialize_cmd(tokens, count, cmd_buf, sizeof(cmd_buf));
        if(n == -1){
            fprintf(stderr, "failed to write data\n");
        }

        int m = kvstore_persist_append(cmd_buf, n);
        if(m == -1){
            persist_error = 1;
            length = sprintf(response, "AOF WRITE ERROR\r\n");
        }
    }
#endif
    return length;
}

int kvstore_request(char *msg, int length, char *response){
    // printf("recv:%s\n", item->rbuffer);
    if(msg == NULL || length < 0 || response == NULL){
        return -1;
    }

    char *tokens[KVSTORE_MAX_TOKENS] = {};

    int token_count = kvstore_split_token(msg, tokens);
    if(token_count == -1){
        return -1;
    }

    // int idx = 0;
    // for(idx = 0; idx < count; idx++){
    //     printf("idx: %s\n", tokens[idx]);
    // }
    // kvstore_aof_append("kvstore_aof_log.txt", msg);
    return kvstore_parser_protocol(tokens, token_count, response);

    return 0;
}

void init_kvengine(void){

#if ENABLE_ARRAY_KVENGINE
    kvstore_array_create(&Array);
#endif

#if ENABLE_RBTREE_KVENGINE
    kvstore_rbtree_create(&Tree);
#endif

#if ENABLE_HASH_KVENGINE
    kvstore_hash_create(&Hash);
#endif

#if ENABLE_SKIPLIST_KVENGINE
    kvstore_skiplist_create(&Skiptable);
#endif
}

void exit_kvengine(void){

#if ENABLE_ARRAY_KVENGINE
    kvstore_array_destory(&Array);
#endif

#if ENABLE_RBTREE_KVENGINE
    kvstore_rbtree_destory(&Tree);
#endif

#if ENABLE_HASH_KVENGINE
    kvstore_hash_destory(&Hash);
#endif

#if ENABLE_SKIPLIST_KVENGINE
    kvstore_skiplist_destory(&Skiptable);
#endif
}

int init_ctx(void){

#if ENABLE_MEM_POOL
    mp_init(&m, 4096);
#endif
}

// 1.40