//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_SERIAZATION_HPP
#define GENERALDEDUPSYSTEM_SERIAZATION_HPP

#include "chunk.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <string>
#include <bits/stdc++.h>

using namespace std;
using namespace boost::archive;

string serialize(Chunk data);

Chunk deserialize(string data);

#endif //GENERALDEDUPSYSTEM_SERIAZATION_HPP
