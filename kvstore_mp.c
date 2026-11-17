#include "kvstore.h"

// 内存池大小，为了跟内存页大小一致，可以变化
#define MEM_PAGE_SIZE 4096


// 初始化
int mp_init(mempool_t *m, int size){
    if(!m) return -1;
    if(size < 16) size = 16;

    m->block_size = size;

    m->mem = (char*)malloc(MEM_PAGE_SIZE);
    if(!m->mem) return -1;
    m->free_ptr = m->mem;
    m->free_count = MEM_PAGE_SIZE / size;

    int i = 0;
    char* ptr = m->free_ptr;
    for(i = 0; i < m->free_count; i++){
        // 把ptr + size的地址，写入ptr指向的内存中
        *(char**)ptr = ptr + size;
        ptr += size;
    }
    *(char**)ptr = NULL;

    return 0;
}

// 销毁
void mp_dest(mempool_t* m){
    if(!m || !m->mem) return;

    free(m->mem);
}

// 分配内存块
void *mp_alloc(mempool_t* m){
    if(!m || m->free_count == 0) return NULL;

    void* ptr = m->free_ptr;

    // 由于已经构建了链表，*(char**)ptr存放的是下一个内存块的起始地址，将其赋值给free_ptr，就是让其指向下一个内存块
    m->free_ptr = *(char**)ptr;
    m->free_count--;

    return ptr;
}

// 释放内存块，ptr为要释放内存块的起始地址
void mp_free(mempool_t* m, void* ptr){
    *(char**)ptr = m->free_ptr;
    m->free_ptr = (char*)ptr;
    m->free_count++;
}

mempool_t m;

#if 0
int main(){
    mempool_t m;
    
    mp_init(&m, 32);

    void* p1 = mp_alloc(&m);
    printf("mp_alloc: %p\n", p1);

    void* p2 = mp_alloc(&m);
    printf("mp_alloc: %p\n", p2);

    void* p3 = mp_alloc(&m);
    printf("mp_alloc: %p\n", p3);

    void* p4 = mp_alloc(&m);
    printf("mp_alloc: %p\n", p4);

    mp_free(&m, p2);

    void* p5 = mp_alloc(&m);
    printf("mp_alloc: %p\n", p5);

    mp_dest(&m);
}
#endif