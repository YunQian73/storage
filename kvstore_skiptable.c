#include <limits.h>
#include <stdbool.h>
#include "kvstore.h"

#define MAX_LEVEL 6


static Node* createNode(int level, char *key, char *value){
    if(level < 0 || level > MAX_LEVEL) return NULL;

    Node *newNode = (Node*)kvstore_malloc(sizeof(Node));
    if(newNode == NULL) return NULL;

    newNode->key = NULL;
    newNode->value = NULL;
    newNode->forward = NULL;

    // 1. 为 key 分配内存
    if(key != NULL){
        newNode->key = kvstore_malloc(strlen(key) + 1);
        if(newNode->key == NULL) goto fail;

        strcpy(newNode->key, key);
    }

    // 2. 为 value 分配内存
    if(value != NULL){
        newNode->value = kvstore_malloc(strlen(value) + 1);
        if(newNode->value == NULL) goto fail;

        strcpy(newNode->value, value);
    }

    // 3. 为 forward 指针数组分配内存
    newNode->forward = (Node**)kvstore_malloc(
        sizeof(Node*) * (level + 1)
    );

    if(newNode->forward == NULL) goto fail;

    // 4. 初始化各层指针
    for(int i = 0; i <= level; i++){
        newNode->forward[i] = NULL;
    }

    return newNode;

fail:
    if(newNode->key != NULL)
        kvstore_free(newNode->key);

    if(newNode->value != NULL)
        kvstore_free(newNode->value);

    if(newNode->forward != NULL)
        kvstore_free(newNode->forward);

    kvstore_free(newNode);

    return NULL;
}

static int createSkipList(skiptable_t *skipList){
    if(skipList == NULL) return -1;

    skipList->level = 0;
    skipList->count = 0;

    skipList->hander = createNode(MAX_LEVEL, NULL, NULL);

    if(skipList->hander == NULL) return -1;

    return 0;
}

int randomLevel(){
    int level = 0;

    while(rand() < RAND_MAX / 2 && level < MAX_LEVEL){
        level++;
    }
    return level;
}

static Node* findNode(skiptable_t *table, char *key){

    if(table == NULL || table->hander == NULL || key == NULL){
        return NULL;
    }

    Node *current = table->hander;

    for(int i = table->level; i >= 0; i--){

        while(current->forward[i] != NULL && strcmp(current->forward[i]->key, key) < 0){

            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if(current != NULL && strcmp(current->key, key) == 0){

        return current;
    }

    return NULL;
}

static void destroyNode(Node *node){
    if(node == NULL) return;

    if(node->key != NULL)
        kvstore_free(node->key);

    if(node->value != NULL)
        kvstore_free(node->value);

    if(node->forward != NULL)
        kvstore_free(node->forward);

    kvstore_free(node);
}

// 5 + 2
skiptable_t Skiptable;

int kvstore_skiplist_create(skiptable_t *table){
    if(table == NULL) return -1;
    return createSkipList(table);
}

void kvstore_skiplist_destory(skiptable_t *table){

    if(table == NULL) return;

    Node *current = table->hander;

    while(current != NULL){

        Node *next = current->forward[0];

        destroyNode(current);

        current = next;
    }

    table->hander = NULL;
    table->level = 0;
    table->count = 0;
}

int _skiplist_set(skiptable_t *table, char *key, char *value){
    if(table == NULL || table->hander == NULL ||
       key == NULL || value == NULL){
        return -1;
    }

    Node *update[MAX_LEVEL + 1];
    Node *current = table->hander;

    // 1. 查找各层的前驱节点
    for(int i = table->level; i >= 0; i--){

        while(current->forward[i] != NULL &&
              strcmp(current->forward[i]->key, key) < 0){

            current = current->forward[i];
        }

        update[i] = current;
    }

    current = current->forward[0];

    // 2. 判断 key 是否已经存在
    if(current != NULL &&
       strcmp(current->key, key) == 0){
        return 1;
    }

    // 3. 创建新节点
    int level = randomLevel();

    Node *newNode = createNode(level, key, value);
    if(newNode == NULL) return -1;

    // 4. 更新跳表最高有效层
    if(level > table->level){

        for(int i = table->level + 1; i <= level; i++){
            update[i] = table->hander;
        }

        table->level = level;
    }

    // 5. 将新节点插入各层链表
    for(int i = 0; i <= level; i++){

        newNode->forward[i] = update[i]->forward[i];

        update[i]->forward[i] = newNode;
    }

    // 6. 更新节点数量
    table->count++;

    return 0;
}

char * _skiplist_get(skiptable_t *table, char *key){
    Node *node = findNode(table, key);

    if(node == NULL) return NULL;

    return node->value;
}

int _skiplist_modify(skiptable_t *table, char *key, char *value){
    if(table == NULL || table->hander == NULL || key == NULL || value == NULL){
        return -1;
    }

    // 1. 查找节点
    Node *node = findNode(table, key);

    if(node == NULL) return 1;

    // 2. 为新的 value 分配内存
    char *vcopy = kvstore_malloc(strlen(value) + 1);

    if(vcopy == NULL) return -1;

    strcpy(vcopy, value);

    // 3. 释放旧的 value
    kvstore_free(node->value);

    // 4. 更新 value
    node->value = vcopy;

    return 0;
}

int _skiplist_delete(skiptable_t *table, char *key){
    if(table == NULL || table->hander == NULL ||
       key == NULL){
        return -1;
    }

    Node *update[MAX_LEVEL + 1];
    Node *current = table->hander;

    // 1. 查找各层的前驱节点
    for(int i = table->level; i >= 0; i--){

        while(current->forward[i] != NULL &&
              strcmp(current->forward[i]->key, key) < 0){

            current = current->forward[i];
        }

        update[i] = current;
    }

    current = current->forward[0];

    // 2. 判断节点是否存在
    if(current == NULL ||
       strcmp(current->key, key) != 0){
        return 1;
    }

    // 3. 解除各层的指针连接
    for(int i = 0; i <= table->level; i++){

        if(update[i]->forward[i] != current){
            break;
        }

        update[i]->forward[i] = current->forward[i];
    }

    // 4. 释放节点
    destroyNode(current);

    // 5. 更新跳表最高有效层
    while(table->level > 0 &&
          table->hander->forward[table->level] == NULL){

        table->level--;
    }

    // 6. 更新节点数量
    table->count--;

    return 0;
}

int _skiplist_count(skiptable_t *table){
    if(table == NULL || table->hander == NULL){
        return -1;
    }

    return table->count;
}

int kvstore_skiplist_set(char *key, char *value){
    return _skiplist_set(&Skiptable, key, value);
}

char * kvstore_skiplist_get(char *key){
    return _skiplist_get(&Skiptable, key);
}

int kvstore_skiplist_modify(char *key, char *value){
    return _skiplist_modify(&Skiptable, key, value);
}

int kvstore_skiplist_delete(char *key){
    return _skiplist_delete(&Skiptable, key);
}

int kvstore_skiplist_count(void){
    return _skiplist_count(&Skiptable);
}