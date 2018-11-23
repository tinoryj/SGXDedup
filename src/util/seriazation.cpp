//
// Created by a on 11/17/18.
//

#include "seriazation.hpp"

using namespace std;
using namespace boost::archive;


string serialize(Chunk data){
    string ans;
    stringstream buffer;
    text_oarchive out(buffer);
    out<<data;
    ans=buffer.str();
    return ans;
}

Chunk deserialize(string data){
    Chunk ans;
    stringstream buffer;
    buffer.str(data);
    text_iarchive in(buffer);
    in>>ans;
    return ans;
}