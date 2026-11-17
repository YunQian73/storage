#include "kvstore.h"

#define RED         1
#define BLACK       2

#define ENABLE_KEY_CHAR 1

#define MAX_KEY_LEN 256
#define MAX_VALUE_LEN 1024

#if ENABLE_KEY_CHAR

typedef char* KEY_TYPE;

#else

typedef int KEY_TYPE;

#endif



#define RBTREE_ENTRY(name, type)    \
    struct name{                    \
        unsigned char color;        \
        struct type* right;         \
        struct type* left;          \
        struct type* parent;        \
    }


typedef struct _rbtree_node{
#if 0
    unsigned char color;
    struct _rbtree_node* right;
    struct _rbtree_node* left;
    struct _rbtree_node* parent;
#else
    RBTREE_ENTRY(, _rbtree_node);
#endif
    KEY_TYPE key;
    void* value;

}rbtree_node;

typedef struct _rbtree{

    struct _rbtree_node* root; // 根节点
    struct _rbtree_node* nil; // 所有叶子节点隐藏，直接定义一个尾节点，让所有节点指向它
    int count;

}rbtree;

rbtree_node* rbtree_mini(rbtree* T, rbtree_node* x){
    while(x->left != T->nil){
        x = x->left;
    }
    return x;
}

rbtree_node* rbtree_maxi(rbtree* T, rbtree_node* x){
    while(x->right != T->nil){
        x = x->right;
    }
    return x;
}

rbtree_node *rbtree_successor(rbtree* T, rbtree_node* x){
    rbtree_node* y = x->parent;

    if(x->right != T->nil){
        return rbtree_mini(T, x->right);
    }

    while((y != T->nil) && (x == y->right)){
        x = y;
        y = y->parent;
    }
    return y;
}

// 左旋
void rbtree_left_rotate(rbtree* T, rbtree_node* x){
    rbtree_node* y = x->right;
    // 应该改变三对指针

    x->right = y->left;
    if(y->left != T->nil){
        y->left->parent = x;
    }

    y->parent = x->parent;
    // 将x的父节点指向y,如果x节点是根节点
    if(x->parent == T->nil){
        T->root = y;
    }
    else if(x == x->parent->left){
        x->parent->left = y;
    }
    else{
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;

}

// 右旋
void rbtree_right_rotate(rbtree* T, rbtree_node* y){
    rbtree_node* x = y->left;
    // 应该改变三对指针

    y->left = x->right;
    if(x->right != T->nil){
        x->right->parent = y;
    }

    x->parent = y->parent;
    // 将y的父节点指向x,如果y节点是根节点
    if(y->parent == T->nil){
        T->root = x;
    }
    else if(y == y->parent->right){
        y->parent->right = x;
    }
    else{
        y->parent->left = x;
    }

    x->right = y;
    y->parent = x;
}

// 红黑树的插入调整
void rbtree_insert_fixup(rbtree* T, rbtree_node* node){
    while(node->parent->color == RED){
        if(node->parent == node->parent->parent->left){
            rbtree_node *y = node->parent->parent->right;

            if(y->color == RED){
                node->parent->color = BLACK;
                y->color = BLACK;
                node->parent->parent->color = RED;

                node = node->parent->parent; // node == RED
            }
            else{ // y == BLACK
                if(node == node->parent->right){
                    node = node->parent;
                    rbtree_left_rotate(T, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;

                rbtree_right_rotate(T, node->parent->parent);
            }
        }
        else{
            rbtree_node* y = node->parent->parent->left;
            if(y->color == RED){
                node->parent->color = BLACK;
                y->color = BLACK;
                node->parent->parent->color = RED;

                node = node->parent->parent; // z -> RED
            }
            else{
                if(node == node->parent->left){
                    node = node->parent;
                    rbtree_right_rotate(T, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rbtree_left_rotate(T, node->parent->parent);
            }
        }
    }
    T->root->color = BLACK;
}

// 插入
void rbtree_insert(rbtree* T, rbtree_node* node){
    rbtree_node* y = T->nil;
    rbtree_node* x = T->root;

    /*
    字符串的对比用strcmp()
    返回值如果 == 0，代表两者相等
    如果 < 0，代表前面的字符要小
    */

    // 循环执行完之后x == T->nil
    while(x != T->nil){
        y = x;
#if ENABLE_KEY_CHAR
        if(strcmp(node->key, x->key) < 0){
            x = x->left;
        }
        else if(strcmp(node->key, x->key) > 0){
            x = x->right;
        }
        else{
            return;
        }
#else

        if(node->key < x->key){
            x = x->left;
        }
        else if(node->key > x->key){
            x = x->right;
        }
        else{
            // 取决于业务场景
            return;
        }
#endif
    }

    node->parent = y;
    if(y == T->nil){
        T->root = node;
    }
#if ENABLE_KEY_CHAR
    else if(strcmp(y->key, node->key) > 0)
#else
    else if(y->key > node->key)
#endif
    {
        y->left = node;
    }
    else{
        y->right = node;
    }

    node->parent = y;
    node->right = T->nil;
    node->left = T->nil;
    node->color = RED;
    rbtree_insert_fixup(T, node);
}

// 红黑树的删除调整
void rbtree_delete_fixup(rbtree* T, rbtree_node* node){
    while((node != T->root) && (node->color == BLACK)){
        if(node == node->parent->left){
            rbtree_node* w = node->parent->right;
            if(w->color == RED){
                w->color = BLACK;
                node->parent->color = RED;

                rbtree_left_rotate(T, node->parent);
                w = node->parent->right;
            }
            if((w->left->color == BLACK) && (w->right->color == BLACK)){
                w->color = RED;
                node = node->parent;
            }
            else{
                if(w->right->color == BLACK){
                    w->left->color = BLACK;
                    w->color = RED;
                    rbtree_right_rotate(T, w);
                    w = node->parent->right;
                }
                w->color = node->parent->color;
                node->parent->color = BLACK;
                w->right->color = BLACK;
                rbtree_left_rotate(T, node->parent);

                node = T->root;
            }
        }
        else{
            rbtree_node* w = node->parent->left;
            if(w->color == RED){
                w->color == BLACK;
                node->parent->color = RED;
                rbtree_right_rotate(T, node->parent);
                w = node->parent->left;
            }
            if((w->left->color == BLACK) && (w->right->color == BLACK)){
                w->color = RED;
                node = node->parent;
            }
            else{
                if(w->left->color == BLACK){
                    w->right->color = BLACK;
                    w->color = RED;
                    rbtree_left_rotate(T, w);
                    w = node->parent->left;
                }
                w->color = node->parent->color;
                node->parent->color = BLACK;
                w->left->color = BLACK;
                rbtree_right_rotate(T, node->parent);

                node = T->root;
            }
        }
    }
    node->color = BLACK;
}

// 删除
rbtree_node* rbtree_delete(rbtree* T, rbtree_node* node){
    rbtree_node* y = T->nil;
    rbtree_node* x = T->nil;

    if((node->left == T->nil) || (node->right == T->nil)){
        y = node;
    }
    else{
        y = rbtree_successor(T, node);
    }

    if(y->left != T->nil){
        x = y->left;
    }
    else if(y->right != T->nil){
        x = y->right;
    }

    x->parent = y->parent;
    if(y->parent == T->nil){
        T->root = x;
    }
    else if(y == y->parent->left){
        y->parent->left = x;
    }
    else{
        y->parent->right = x;
    }

    if(y != node){
#if ENABLE_KEY_CHAR
        void *tmp = node->key;
        node->key = y->key;
        y->key = tmp;

        tmp = node->value;
        node->value = y->value;
        y->value = tmp;
        // memcpy(node->key, y->key, sizeof(y->key));
        // Q: 为什么不直接像int一样直接赋值，或者为什么不用memcpy()
#else
        node->key = y->key;
        node->value = y->value;
#endif
    }

    if(y->color == BLACK){
        rbtree_delete_fixup(T, x);
    }

    return y;
}

// 搜索
rbtree_node* rbtree_search(rbtree* T, KEY_TYPE key){
    rbtree_node* node = T->root;
    while(node != T->nil){
#if ENABLE_KEY_CHAR
        if(strcmp(key, node->key) < 0){
            node = node->left;
        }
        else if(strcmp(key, node->key) > 0){
            node = node->right;
        }
        else{
            return node;
        }
#else
        if(key < node->key){
            node = node->left;
        }
        else if(key > node->key){
            node = node->right;
        }
        else{
            return node;
        }
#endif
    }
    return T->nil;
}

void rbtree_traversal(rbtree* T, rbtree_node* node){
    if(node != T->nil){
        rbtree_traversal(T, node->left);
        printf("key:%s, color:%d\n", node->key, node->color);
        rbtree_traversal(T, node->right);
    }
}

// api
int kvstore_rbtree_create(rbtree* tree){
    if(!tree) return -1;
    memset(tree, 0, sizeof(rbtree));
    
    tree->nil = (rbtree_node*)malloc(sizeof(rbtree_node));
    tree->nil->key = malloc(1);
    *(tree->nil->key) = '\0';

    tree->nil->color = BLACK;
    tree->root = tree->nil;

    return 0;
}

int kvstore_rbtree_destory(rbtree* tree){
    if(!tree) return -1;

    if(tree->nil->key) free(tree->nil->key);

    rbtree_node* node = tree->root;
    while(node != tree->nil){
        node = rbtree_mini(tree, tree->root);
        if(node == tree->nil){
            break;
        }

        node = rbtree_delete(tree, node);

        if(!node)
        {
            kvstore_free(node->key);
            kvstore_free(node->value);
            kvstore_free(node);
        }
    }
}


int _rbtree_set(rbtree *tree, char *key, char *value){
    rbtree_node* node = (rbtree_node*)malloc(sizeof(rbtree_node));
    if(!node) return -1;

    node->key = kvstore_malloc(strlen(key) + 1);
    if(node->key == NULL) {
        kvstore_free(node);
        return -1;
    }
    memset(node->key, 0, strlen(key) + 1);
    strcpy(node->key, key);

    node->value = kvstore_malloc(strlen(value) + 1);
    if(node->value == NULL){
        kvstore_free(node->key);
        kvstore_free(node);
        return -1;
    }
    memset(node->value, 0, strlen(value) + 1);
    strcpy(node->value, value);

    rbtree_insert(tree, node);
    tree->count++;

    return 0;
}

char* _rbtree_get(rbtree* tree, char *key){
    rbtree_node* node = rbtree_search(tree, key);
    if(node == tree->nil){
        return NULL;
    }

    return node->value;
}

int _rbtree_delete(rbtree* tree, char *key){
    rbtree_node* node = rbtree_search(tree, key);
    if(node == tree->nil){
        return -1;
    }
    rbtree_node* cur = rbtree_delete(tree, node);
    if(!cur) {
        kvstore_free(cur->key);
        kvstore_free(cur->value);
        kvstore_free(cur);
    }

    tree->count--;
    
    return 0;
}

int _rbtree_modify(rbtree* tree, char *key, char *value){
    rbtree_node* node = rbtree_search(tree, key);
    if(node == tree->nil){
        return -1;
    }

    char* tmp = node->value;
    kvstore_free(tmp);

    node->value = kvstore_malloc(strlen(value) + 1);
    if(node->value == NULL){
        return -1;
    }
    strcpy(node->value, value);

    return 0;
}

int _rbtree_count(rbtree* tree){
    return tree->count;
}

int kvstore_rbtree_set(char *key, char *value){
    return _rbtree_set(&Tree, key, value);
}

char* kvstore_rbtree_get(char *key){
    return _rbtree_get(&Tree, key);
}

int kvstore_rbtree_delete(char *key){
    return _rbtree_delete(&Tree, key);
}

int kvstore_rbtree_modify(char *key, char *value){
    return _rbtree_modify(&Tree, key, value);
}

int kvstore_rbtree_count(){
    return _rbtree_count(&Tree);
}

rbtree Tree;

#if 0
int main(){
#if ENABLE_KEY_CHAR

    KEY_TYPE keyArray[10] = {
        "YunQian", "QingYun", "Lee", "Mark", "JingZhe123", 
        "ZhangSan", "LiSi", "WangWu", "Youzi", "Nick"
    };

    char* valueArray[10] = {
        "I love you.", "zdjd", "myself", "teacher", "nickname", 
        "first", "second", "third", "a women teacher", "last"
    };

    rbtree* T = (rbtree*)malloc(sizeof(rbtree));
    if(T == NULL){
        printf("malloc failed\n");
        return -1;
    }
    
    T->nil = (rbtree_node*)malloc(sizeof(rbtree_node));
    T->nil->key = malloc(1);
    *(T->nil->key) = '\0';

    T->nil->color = BLACK;
    T->root = T->nil;

    rbtree_node* node = T->nil;
    
    int i = 0;
    for(i = 0; i < 10; i++){
        node = (rbtree_node*)malloc(sizeof(rbtree_node));
        node->key = malloc(strlen(keyArray[i]) + 1);
        memset(node->key, 0, strlen(keyArray[i] + 1));
        strcpy(node->key, keyArray[i]);

        node->value = malloc(strlen(valueArray[i]) + 1);
        memset(node->value, 0, strlen(valueArray[i] + 1));
        strcpy(node->value, valueArray[i]);
        // node->key = keyArray[i];
        // node->value = valueArray[i];

        rbtree_insert(T, node);
    }

    rbtree_traversal(T, T->root);
    printf("-----------------------------------\n");

        for(i = 0; i < 10; i++){
        rbtree_node* node = rbtree_search(T, keyArray[i]);
        rbtree_node* cur = rbtree_delete(T, node);

        if(!cur){
            free(cur->key);
            free(cur->value);
            free(cur);
        }
        
        rbtree_traversal(T, T->root);
        printf("-----------------------------------\n");
    }
#else
    int keyArray[20] = {24, 25, 13, 35, 23, 26, 67, 47, 38, 98, 20, 19, 17, 49, 12, 21, 9, 18, 14, 15};

    rbtree* T = (rbtree*)malloc(sizeof(rbtree));
    if(T == NULL){
        printf("malloc failed\n");
        return -1;
    }

    T->nil = (rbtree_node*)malloc(sizeof(rbtree_node));
    T->nil->color = BLACK;
    T->root = T->nil;

    rbtree_node* node = T->nil;
    int i = 0;
    for(i = 0; i < 20; i++){
        node = (rbtree_node*)malloc(sizeof(rbtree_node));
        node->key = keyArray[i];
        node->value = NULL;

        rbtree_insert(T, node);
    }

    rbtree_traversal(T, T->root);
    printf("-----------------------------------\n");

    for(i = 0; i < 20; i++){
        rbtree_node* node = rbtree_search(T, keyArray[i]);
        rbtree_node* cur = rbtree_delete(T, node);
        free(cur);

        rbtree_traversal(T, T->root);
        printf("-----------------------------------\n");
    }
#endif
}
#endif