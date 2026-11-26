#include "kvstore.h"

int main(){

    init_kvengine();

#if ENABLE_PERSIST
    if(kvstore_persist_init("appendonly.aof") != 0){
        fprintf(stderr, "AOF init failed\n");

        exit_kvengine();

        return EXIT_FAILURE;
    }
#endif

#if(ENABLE_NETWORK_SELECT == NETWORK_EPOLL)
    epoll_entry(kvstore_request);
#elif(ENABLE_NETWORK_SELECT == NETWORK_NTYCO)
    ntyco_entry(kvstore_request);
#elif(ENABLE_NETWORK_SELECT == NETWORK_IOURING)
#endif

#if ENABLE_PERSIST
    if (kvstore_persist_sync() != 0) {

        fprintf(stderr, "AOF sync failed\n");
    }

    kvstore_persist_destroy();
#endif

    exit_kvengine();
}