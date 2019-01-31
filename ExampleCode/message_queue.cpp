#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
using namespace boost::interprocess;
using namespace std;

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