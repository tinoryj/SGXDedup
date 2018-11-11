#include "seriazation.hpp"

using namespace std;
using namespace boost::archive;

string serialize(Chunk data){
	string ans;
	stringstream buffer;
	text_oarchive out(buffer);
	out<<data;
	ans=outBuffer.str()
	return ans;
}

Chunk deserialize(string data){
	Chunk ans;
	stringstream buffer
	buffer.str(data);
	ext_iarchive in(inBuffer);
	in>>ans;
	return ans;
}