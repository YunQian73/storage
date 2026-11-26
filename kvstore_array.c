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
    if(arr == NULL) return -1;

    if(arr->array_table != NULL){
        for(int i = 0; i < arr->array_idx; i++){
            kvstore_free(arr->array_table[i].key);
            kvstore_free(arr->array_table[i].value);
        }

        kvstore_free(arr->array_table);
    }

    arr->array_table = NULL;
    arr->array_idx = 0;

    return 0;
}


int _array_set(array_t *arr, char *key, char *value){

    if(arr == NULL || arr->array_table == NULL ||
       key == NULL || value == NULL){
        return -1;
    }

    // 1. 判断 key 是否已经存在
    for(int i = 0; i < arr->array_idx; i++){
        if(strcmp(arr->array_table[i].key, key) == 0){
            return 1;
        }
    }

    // 2. 判断数组是否已满
    if(arr->array_idx >= KVS_ARRAY_SIZE){
        return -1;
    }

    // 3. 分配 key 的内存
    char *kcopy = kvstore_malloc(strlen(key) + 1);

    if(kcopy == NULL) return -1;

    strcpy(kcopy, key);

    // 4. 分配 value 的内存
    char *vcopy = kvstore_malloc(strlen(value) + 1);

    if(vcopy == NULL){
        kvstore_free(kcopy);
        return -1;
    }

    strcpy(vcopy, value);

    // 5. 插入到数组末尾
    arr->array_table[arr->array_idx].key = kcopy;
    arr->array_table[arr->array_idx].value = vcopy;

    arr->array_idx++;

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
    return _array_set(&Array, key, value);
}

char* kvstore_array_get(char *key){
    return _array_get(&Array, key);
}

int kvstore_array_delete(char *key){
    return _array_delete(&Array, key);
}

int kvstore_array_modify(char *key, char *value){
    return _array_modify(&Array, key, value);
}

int kvstore_array_count(void){
    return _array_count(&Array);
}