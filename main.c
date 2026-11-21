#include "kvstore.h"

int main(){

    init_kvengine();

#if(ENABLE_NETWORK_SELECT == NETWORK_EPOLL)
    epoll_entry(kvstore_request);
#elif(ENABLE_NETWORK_SELECT == NETWORK_NTYCO)
    ntyco_entry(kvstore_requset);
#elif(ENABLE_NETWORK_SELECT == NETWORK_IOURING)
#endif

    exit_kvengine();
}