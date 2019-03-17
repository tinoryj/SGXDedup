//
// Created by a on 2/27/19.
//

#include <bits/stdc++.h>
#include "chrono"
#include "unistd.h"

using namespace std::chrono;

int main(){
    duration<int,std::micro> dtn;
    system_clock::time_point st,en;
    st=high_resolution_clock::now();
    sleep(2);
    en=high_resolution_clock::now();
    dtn=duration_cast<microseconds>(st-en);
    std::cout<<dtn.count();
}