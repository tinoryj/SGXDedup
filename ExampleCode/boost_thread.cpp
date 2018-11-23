//
// Created by a on 11/21/18.
//

#include <bits/stdc++.h>
#include <time.h>
#include <unistd.h>
#include <boost/thread.hpp>
using namespace std;

void fun(int x){
    for(int i=0;i<10;i++){
        cout<<x<<endl;
        sleep(1);
    }
}

int main(){
    boost::thread th1(boost::bind(fun,1));
    //boost::thread th2(boost::bind(fun,2));
    //th1.join();
    cout<<"------------\n";
    //th2.join();
    th1.join();

    //th2.detach();
}