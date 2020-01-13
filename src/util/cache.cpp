#include <boost/thread/mutex.hpp>
#include <cache.hpp>

using namespace std;
using namespace boost::compute::detail;

Cache::Cache()
{
    this->Cache_ = new lru_cache<string, string>(100);
}

void Cache::insertToCache(string& name, string& data)
{
    {
        boost::unique_lock<boost::shared_mutex> t(this->mtx);
        this->Cache_->insert(name, data);
    }
}

bool Cache::existsInCache(string& name)
{
    bool flag = false;
    {
        boost::shared_lock<boost::shared_mutex> t(this->mtx);
        flag = this->Cache_->contains(name);
    }
    return flag;
}

string Cache::getFromCache(string& name)
{
    string ans = "";
    if (!existsInCache(name))
        return ans;
    {
        boost::shared_lock<boost::shared_mutex> t(this->mtx);
        ans = this->Cache_->get(name).get();
    }
    return ans;
}