#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


//include what you want to serialize(string.vector etc.see more in include flode)
#include <boost/serialization/string.hpp>


#include <bits/stdc++.h>
using namespace std;

class Chunk {
public:
	uint64_t _ID;
    void* x;



	template<class Archive>
	void serialize(Archive &ar, const unsigned int version) {
		ar & _ID;
        //ar & x;
	}
}x;

template <class T>
bool serialize(T data,string &ans){

    using namespace boost::archive;
	stringstream buffer;
	text_oarchive out(buffer);
	out<<data;
	ans=buffer.str();
}


int main(){
	x._ID=12;
	string tmp;
	serialize(x,tmp);
    cout<<tmp<<endl<<tmp.length()<<endl;

	return 0;

}
