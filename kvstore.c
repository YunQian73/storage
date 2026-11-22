#include "kvstore.h"


const char* commands[] = {
    "SET", "GET", "DEL", "MOD", "ACOUNT",
    "RSET", "RGET", "RDEL", "RMOD", "RCOUNT"
};






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


int kvstore_split_token(char* msg, char** tokens){
    // 初始化后tokens[i]为空，但是tokens不应为空
    if(msg == NULL || tokens == NULL) return -1;

    int idx = 0;

    char* token = strtok(msg, " \r\n");

    while(token != NULL){
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
    char cmd_buf[1024] = {0};
    int length = 0;
    char* key = tokens[1];
    char* value = tokens[2];

    switch(cmd){
        case KVS_CMD_SET:{
            // LOG("set\n");
            int res = kvstore_array_set(key, value);
            if(!res){
                length = sprintf(response, "SUCCESS\r\n");
#if ENABLE_KVSTORE_LOG
                log_file_fd = open("kvstore_rdb_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
                if(log_file_fd != -1){
                    snprintf(cmd_buf, sizeof(cmd_buf), "%s %s %s\r\n", tokens[0], tokens[1], tokens[2]);
                    write(log_file_fd, cmd_buf, strlen(cmd_buf));
                    close(log_file_fd);
                }
                else{
                    perror("open file failed (SET)");
                }
#endif
            }
            else{
                length = sprintf(response, "FATLED\r\n");
            }
            printf("set: %d\n", res);
            break;
        }
        case KVS_CMD_GET:{
            // LOG("get\n");
            char *val = kvstore_array_get(key);
            if(val){
                length = sprintf(response, "%s\r\n", val);
            }
            else{
                length = sprintf(response, "NO EXIST\r\n");
            }
            printf("get: %s\n", val);
            break;
        }
        case KVS_CMD_DEL:{
            // LOG("del\n");
            int res = kvstore_array_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR");
            }
            else if(res == 0){
                length = sprintf(response, "%s", "SUCCESS");
#if ENABLE_KVSTORE_LOG
                int ret_log = delete_log_cmd("kvstore_rdb_log.txt", "SET", tokens[1]);
                if(ret_log != 0){
                    fprintf(stderr, "清理key = %s的日志失败\n", tokens[1]);
                }
#endif
            }
            else{
                length = sprintf(response, "NO EXITS");
            }
            break;
        }
        case KVS_CMD_MOD:{
            // LOG("mod\n");
            int res = kvstore_array_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR");
            }
            else if(res == 0){
                length = sprintf(response, "%s", "SUCCESS");
#if ENABLE_KVSTORE_LOG
                int ret_log = modify_log_cmd("kvstore_rdb_log.txt", "SET", tokens[1], tokens[2]);
                if(ret_log != 0){
                    fprintf(stderr, "修改key = %s的日志失败\n", tokens[1]);
                }
#endif
            }
            else{
                length = sprintf(response, "NO EXITS");
            }          
            break;
        }
        case KVS_CMD_ACOUNT:{
            int count = kvstore_array_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR");
            }
            else{
                length = sprintf(response, "%d", count);               
            }  
            break;
        }
        case KVS_CMD_RSET:{
            // printf("rset:%s\n", key);
            int res = kvstore_rbtree_set(key, value);
            if(!res){
                length = sprintf(response, "SUCCESS");
#if ENABLE_KVSTORE_LOG
                log_file_fd = open("kvstore_rdb_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
                if(log_file_fd != -1){
                    length = snprintf(cmd_buf, sizeof(cmd_buf), "%s %s %s\r\n", tokens[0], tokens[1], tokens[2]);
                    write(log_file_fd, cmd_buf, strlen(cmd_buf));
                    close(log_file_fd);
                }
                else{
                    perror("open file failed (RSET)");
                }
#endif
            }
            else{
                length = sprintf(response, "FATLED");
            }
            break;
        }
        case KVS_CMD_RGET:{
            char *val = kvstore_rbtree_get(key);
            if(val){
                length = sprintf(response, "%s", val);
            }
            else{
                length = sprintf(response, "NO EXIST");
            }
            printf("rget: %s\n", val);
            break;
        }
        case KVS_CMD_RDEL:{
            // printf("rdel:%s\n", key);
            int res = kvstore_rbtree_delete(key);
            if(res < 0){ // server
                length = sprintf(response, "%s", "ERROR");
            }
            else if(res == 0){
                length = sprintf(response, "%s", "SUCCESS");
#if ENABLE_KVSTORE_LOG
                int ret_log = delete_log_cmd("kvstore_rdb_log.txt", "SET", tokens[1]);
                if(ret_log != 0){
                    fprintf(stderr, "清理key = %s的日志失败\n", tokens[1]);
                }
#endif
            }
            else{
                length = sprintf(response, "NO EXITS");
            }
            break;
        }
        case KVS_CMD_RMOD:{
            // printf("rmod:%s\n", key);
            int res = kvstore_rbtree_modify(key, value);  
            if(res < 0){
                length = sprintf(response, "%s", "ERROR");
            }
            else if(res == 0){
                length = sprintf(response, "%s", "SUCCESS");               
            }
            else{
                length = sprintf(response, "NO EXITS");
            } 
            break;
        }  
        case KVS_CMD_RCOUNT:{
            int count = kvstore_rbtree_count();  
            if(count < 0){
                length = sprintf(response, "%s", "ERROR");
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
    return length;
}

int kvstore_request(char *msg, int count, char *response){
    // printf("recv:%s\n", item->rbuffer);
    if(msg == NULL || count < 0 || response == NULL){
        return -1;
    }

    char *tokens[KVSTORE_MAX_TOKENS] = {};

    count = kvstore_split_token(msg, tokens);
    if(count == -1){
        return -1;
    }

    // int idx = 0;
    // for(idx = 0; idx < count; idx++){
    //     printf("idx: %s\n", tokens[idx]);
    // }
    kvstore_aof_append("kvstore_aof_log.txt", msg);
    return kvstore_parser_protocol(tokens, count, response);

    return 0;
}

void init_kvengine(void){

#if ENABLE_ARRAY_KVENGINE

#endif

#if ENABLE_RBTREE_KVENGINE
    kvstore_rbtree_create(&Tree);
#endif
}

void exit_kvengine(void){

#if ENABLE_ARRAY_KVENGINE

#endif

#if ENABLE_RBTREE_KVENGINE
    kvstore_rbtree_destory(&Tree);
#endif
}

int init_ctx(void){

#if ENABLE_MEM_POOL
    mp_init(&m, 4096);
#endif
}


#if 0
int main(){

    init_kvengine();

#if(ENABLE_NETWORK_SELECT == NETWORK_EPOLL)
    epoll_entry();
#elif(ENABLE_NETWORK_SELECT == NETWORK_NTYCO)
    ntyco_entry();
#elif(ENABLE_NETWORK_SELECT == NETWORK_IOURING)
#endif

    exit_kvengine();
}
#endif
// 1.40