//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_SERIAZATION_HPP
#define GENERALDEDUPSYSTEM_SERIAZATION_HPP

#include "chunk.hpp"
#include "message.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <string>
#include <bits/stdc++.h>

using namespace std;
using namespace boost::archive;

template <class T>
string serialize(T data){
    string ans;
    stringstream buffer;
    text_oarchive out(buffer);
    out<<data;
    ans=buffer.str();
    return ans;
}

template <class T>
T deserialize(string data){
    T ans;
    stringstream buffer;
    buffer.str(data);
    text_iarchive in(buffer);
    in>>ans;
    return ans;
}


#endif //GENERALDEDUPSYSTEM_SERIAZATION_HPP
