#include "kvstore.h"

struct kvs_array_item array_table[KVS_ARRAY_SIZE] = {0};
int array_idx = 0;

// create - 分配空间，让idx = 0

// destory - 释放空间自行

int kvstore_array_set(char *key, char *value){
    if(key == NULL || value == NULL || array_idx == KVS_ARRAY_SIZE) return -1;

    char *kcopy = kvstore_malloc(strlen(key) + 1); // 不要直接用malloc不利于拓展
    if(kcopy == NULL) return -1;
    strncpy(kcopy, key, strlen(key) + 1);

    char *vcopy = kvstore_malloc(strlen(value) + 1); // 不要直接用malloc不利于拓展
    if(kcopy == NULL) {
        kvstore_free(kcopy);
        return -1;
    }
    strncpy(vcopy, value, strlen(value) + 1);

    int i = 0;
    // 这里不能判断等于的情况，当array_table满的
    // 时候(array_table[0] ~ array_table[1023])，
    // 但这时array_idx = 1024，直接array_table[1024]会溢出
    for(i = 0; i < array_idx; i++){
        if(array_table[i].key == NULL){
            array_table[i].key = kcopy;
            array_table[i].value = vcopy;
            array_idx++;
            return 0;
        }
    }

    if(i < KVS_ARRAY_SIZE && i == array_idx){
        array_table[array_idx].key = kcopy;
        array_table[array_idx].value = vcopy;
        array_idx++;
    }

    return 0;
}

char* kvstore_array_get(char *key){
    int i = 0;
    for(i = 0; i < array_idx; i++){
        if(array_table[i].key == NULL) return NULL;

        if(strcmp(array_table[i].key, key) == 0){
            return array_table[i].value;
        }
    }

    return NULL;
}

int kvstore_array_delete(char *key){
    int i = 0;
    if(key == NULL) return -1;    

    for(i = 0; i < array_idx; i++){
        if(strcmp(array_table[i].key, key) == 0){
            kvstore_free(array_table[i].value);
            array_table[i].value = NULL;

            kvstore_free(array_table[i].key);
            array_table[i].key = NULL;

            array_idx--;

            return 0;
        }
    }
    return i;
}

int kvstore_array_modify(char *key, char *value){
    int i = 0;
    if(key == NULL || value == NULL) return -1;

    for(i = 0; i < array_idx; i++){
        if(strcmp(array_table[i].key, key) == 0){
            kvstore_free(array_table[i].value);
            array_table[i].value = NULL;

            char *vcopy = kvstore_malloc(strlen(value) + 1);
            strncpy(vcopy, value, strlen(value) + 1);

            array_table[i].value = vcopy;

            return 0;
        }
    }
    return i;
}

int kvstore_array_count(void){
    return array_idx;
}