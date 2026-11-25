#include "kvstore.h"

array_t Array;

// create - 分配空间，让idx = 0
int kvstore_array_create(array_t *arr){
    if(!arr) return -1;

    arr->array_table = kvstore_malloc(KVS_ARRAY_SIZE * sizeof(struct kvs_array_item));
    if(!arr->array_table) return -1;
    memset(arr->array_table, 0, KVS_ARRAY_SIZE * sizeof(struct kvs_array_item));

    arr->array_idx = 0;

    return 0;
}


// destory - 释放空间自行
int kvstore_array_destory(array_t *arr){
    if(!arr) return -1;

    if(!arr->array_table) kvstore_free(arr->array_table);

    return 0;
}


int _array_set(array_t *arr, char *key, char *value){
    if(arr == NULL || key == NULL || value == NULL || arr->array_idx == KVS_ARRAY_SIZE) return -1;

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
    for(i = 0; i < arr->array_idx; i++){
        if(arr->array_table[i].key == NULL){
            arr->array_table[i].key = kcopy;
            arr->array_table[i].value = vcopy;
            arr->array_idx++;
            return 0;
        }
    }

    if(i < KVS_ARRAY_SIZE && i == arr->array_idx){
        arr->array_table[arr->array_idx].key = kcopy;
        arr->array_table[arr->array_idx].value = vcopy;
        arr->array_idx++;
    }

    return 0;
}

char* _array_get(array_t *arr, char *key){
    int i = 0;
    for(i = 0; i < arr->array_idx; i++){
        if(arr->array_table[i].key == NULL) return NULL;

        if(strcmp(arr->array_table[i].key, key) == 0){
            return arr->array_table[i].value;
        }
    }

    return NULL;
}

int _array_delete(array_t *arr, char *key)
{
    if (arr == NULL || key == NULL)
        return -1;

    for (int i = 0; i < arr->array_idx; i++) {
        if (strcmp(arr->array_table[i].key, key) == 0) {
            kvstore_free(arr->array_table[i].key);
            kvstore_free(arr->array_table[i].value);

            // 删除最后一个有效元素
            arr->array_idx--;

            // 用最后一个元素填补当前空位
            if (i != arr->array_idx) {
                arr->array_table[i] = arr->array_table[arr->array_idx];
            }

            // 清空原最后一个位置
            arr->array_table[arr->array_idx].key = NULL;
            arr->array_table[arr->array_idx].value = NULL;

            return 0;
        }
    }
    return 1; // Key不存在
}

int _array_modify(array_t *arr, char *key, char *value){
    int i = 0;
    if(arr == NULL || key == NULL || value == NULL) return -1;

    for(i = 0; i < arr->array_idx; i++){
        if(strcmp(arr->array_table[i].key, key) == 0){
            kvstore_free(arr->array_table[i].value);
            arr->array_table[i].value = NULL;

            char *vcopy = kvstore_malloc(strlen(value) + 1);
            strncpy(vcopy, value, strlen(value) + 1);

            arr->array_table[i].value = vcopy;

            return 0;
        }
    }
    return i;
}

int _array_count(array_t *arr){
    return arr->array_idx;
}


int kvstore_array_set(char *key, char *value){
    _array_set(&Array, key, value);
}

char* kvstore_array_get(char *key){
    _array_get(&Array, key);
}

int kvstore_array_delete(char *key){
    _array_delete(&Array, key);
}

int kvstore_array_modify(char *key, char *value){
    _array_modify(&Array, key, value);
}

int kvstore_array_count(void){
    _array_count(&Array);
}