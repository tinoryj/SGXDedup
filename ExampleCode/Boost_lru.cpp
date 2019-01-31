//
// Created by a on 1/12/19.
//
#include "boost/compute/detail/lru_cache.hpp"
#include "boost/thread.hpp"
#include <bits/stdc++.h>

using namespace boost::compute::detail;
using namespace std;

int main(){
    lru_cache<string,string> *cac=new lru_cache<string,string>(5);
    string a,b;
    for(int i=0;i<100000;i++){
        a=a+'a';
        b=b+'b';
        cac->insert(a,b);
    }
    return 0;
}