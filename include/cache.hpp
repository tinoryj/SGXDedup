#ifndef SGXDEDUP_CACHE_HPP
#define SGXDEDUP_CACHE_HPP

#include "boost/compute/detail/lru_cache.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include <string>

extern Configure config;

using namespace boost::compute::detail;
using namespace std;

class Cache {
private:
    lru_cache<string, string>* Cache_;
    boost::shared_mutex mtx;

public:
    Cache();
    void insertToCache(string& name, string& data);
    bool existsInCache(string& name);
    string getFromCache(string& name);
};

#endif //SGXDEDUP_CACHE_HPP
