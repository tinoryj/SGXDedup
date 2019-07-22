#ifndef GENERALDEDUPSYSTEM_SERIAZATION_HPP
#define GENERALDEDUPSYSTEM_SERIAZATION_HPP

#include <bits/stdc++.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;
using namespace boost::archive;

template <class T>
bool serialize(T data, string& ans)
{
    stringstream buffer;
    text_oarchive out(buffer);
    out << data;
    ans = buffer.str();
}

template <class T>
bool deserialize(string data, T& ans)
{
    stringstream buffer;
    buffer.str(data);
    text_iarchive in(buffer);
    in >> ans;
}

#endif //GENERALDEDUPSYSTEM_SERIAZATION_HPP
