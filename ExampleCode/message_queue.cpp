#include <boost/interprocess/ipc/message_queue.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>
using namespace boost::interprocess;
using namespace std;

void timereceive(){
    message_queue::remove("mq");
    message_queue mqout(create_only,"mq",100,2000);
    message_queue mqin(open_only,"mq");
    char buffer[1024];
    message_queue::size_type recvd_size;
    unsigned int priority;
    boost::posix_time::ptime a=microsec_clock::universal_time()+boost::posix_time::milliseconds(1000);
    mqin.timed_receive(buffer,2000,recvd_size,priority,a);
}

int main(){
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
}