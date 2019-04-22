#include <boost/interprocess/ipc/message_queue.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread.hpp>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
using namespace boost::interprocess;
using namespace std;


void timereceive(){
    timeval st,ed;
    message_queue mqin(open_or_create,"mq",2,1260);
    char buffer[4096];
    unsigned long recvd_size;
    unsigned int priority;
    boost::posix_time::ptime a=microsec_clock::universal_time()+boost::posix_time::milliseconds(1000);
    bool status;
    for(int i=0;i<10;i++) {
        gettimeofday(&st,NULL);
        status=mqin.timed_receive(buffer, 1260, recvd_size, priority, a);
        std::cerr<<"recv "<<status<<endl;
        if(!status){
            i--;
            gettimeofday(&ed,NULL);
            cerr<<"recv fail time : "<<ed.tv_usec-st.tv_usec<<endl;
            continue;
        }
        gettimeofday(&ed,NULL);
        cerr<<"recv success time : "<<ed.tv_usec-st.tv_usec<<endl;
        cerr<<buffer[0]<<endl;
    }
    cout<<"done\n";
}

void timesend() {
    timeval st,ed;
    message_queue mqout(open_or_create, "mq", 2,1260);
    char buffer[1260];
    message_queue::size_type recvd_size;
    boost::posix_time::ptime a=microsec_clock::universal_time()+boost::posix_time::milliseconds(100);
    bool status;
    unsigned int priority;
    for (int i = 0; i < 10; i++) {
        gettimeofday(&st,NULL);
        buffer[0]='0'+i;
        status=mqout.timed_send(buffer,12602,0,a);
        std::cerr<<"send "<<status<<endl;
        if(!status){
            i--;
            cerr<<"recv fail time : "<<ed.tv_usec-st.tv_usec<<endl;
        }
        gettimeofday(&ed,NULL);
        cerr<<"send success time : "<<ed.tv_usec-st.tv_usec<<endl;
    }
}

int main(){
    message_queue::remove("mq");
    boost::thread th(timesend);
    sleep(1);
    boost::thread th1(timereceive);
    sleep(5);
    th.join();
    th1.join();
    return 0;

    /*
  //Erase previous message queue   
  message_queue::remove("message_queue");  
  
  string s;
  cin>>s;

  //Create a message_queue.   
  message_queue mq1  (create_only,"message_queue",100,2000);
  mq1.send(s.c_str(), sizeof(char)*s.length(), 0);

  //Open a message queue.   
  message_queue mq2(open_only,"message_queue");
  unsigned int priority;
  message_queue::size_type recvd_size; 
  char buffer[2000];
  mq2.receive(&buffer, sizeof(buffer), recvd_size, priority);  
  
  cout<<recvd_size<<endl;
  cout<<buffer<<endl;

  return 0;
   */
}