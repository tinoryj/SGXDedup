//
// Created by a on 11/25/18.
//

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <bits/stdc++.h>

using namespace std;
int main(){
    using namespace boost;
    using namespace boost::property_tree;
    ptree root;
    read_json<ptree>("config.json", root);
    cout<<root.get<uint64_t>("ChunkerConfig._runningType")<<"\n------\n";
    std::vector<std::string> _keyServerIP;
    for (ptree::value_type &it:root.get_child("KeyServerConfig._keyServerIP")) {
        _keyServerIP.push_back(it.second.data());
    }
    for(auto it:_keyServerIP){
        cout<<it<<endl;
    }
}
