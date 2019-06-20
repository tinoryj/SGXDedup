//
// Created by a on 6/20/19.
//

#include <boost/pool/pool.hpp>
#include <cstdlib>
#include <cstring>
using namespace boost;

int main(){
    pool<>* alloc;
    alloc=new pool<>(1000000);
    void *b;
    for(int i=0;i<100;i++){
        b=alloc->malloc();
        memset(b,-1,10);
        printf("%s\n",(char*)b);
        alloc->free(b);
    }
}