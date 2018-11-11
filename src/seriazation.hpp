#ifndef SERIAZATION
#define SERIAZATION

#include <chunker.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <string>

using namespace std;
using namespace boost::archive;

string serialize(Chunk data);

Chunk deserialize(string data);

#endif