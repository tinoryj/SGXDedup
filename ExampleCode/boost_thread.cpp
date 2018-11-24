//
// Created by a on 11/21/18.
//

#include <bits/stdc++.h>
#include <time.h>
#include <unistd.h>
#include <boost/thread.hpp>
using namespace std;
using namespace boost;

shared_mutex rwmutex;

void readthread(int x,int y){
    for(int i=0;i<10;i++){
        {
            shared_lock<shared_mutex> tm(rwmutex);
            cout<<"read x in: "<<i<<" x=";
            cout<<x<<endl;
            cout<<"read x out\n";
        }
        boost::this_thread::sleep(posix_time::seconds(y));
    }
}

void writethread(int x,int y){
    for(int i=0;i<10;i++){
        {
            boost::unique_lock<shared_mutex> tm(rwmutex);
            cout<<"write x in: "<<i<<" x=";
            cout<<x<<endl;
            cout<<"write x out\n";
        }
        boost::this_thread::sleep(posix_time::seconds(y));
    }
}

int main(){
    boost::thread th1(bind(readthread,1,1));
    boost::thread th2(bind(writethread,2,2));
    boost::thread th3(bind(readthread,3,1));
    boost::thread th4(bind(writethread,4,1));
    boost::thread th5(bind(writethread,5,2));
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
}