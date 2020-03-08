# General data deduplication system design

> version: 0.1.2 
> data: 2018.10.10 designer: tinoryj

## Design goals

* Optimal scalability
* Optimal performance
* Full C++, object-oriented design


## WorkFlow
![DeduplicationSystem-FrameworkDesign-2018](media/15394101863645/DeduplicationSystem-FrameworkDesign-2018.png)


## Dependent libs

* OpenSSL [https://www.openssl.org/source/](https://www.openssl.org/source/)
* Boost C++ library [http://sourceforge.net/projects/boost/files/boost/](http://sourceforge.net/projects/boost/files/boost/)
* LevelDB [https://github.com/google/leveldb/archive/master.zip](https://github.com/google/leveldb/archive/master.zip)
* SGX


## Basic Data Structure

### Chunk

```c++
class Chunk {
    private: 
        uint64_t _ID;
        uint64_t _type;
        uint64_t _logicDataSize;
        string _logicData;
        string _metaData;
        string _chunkHash;
        string _encryptKey;
        // any additional info of chunk
        
    public:
        Chunk(uint64_t ID, uint64_t type, uint64_t logicDataSize, string logicData, string metaData, string chunkHash);
        ~Chunk();
        uint64_t getID();
        uint64_t getType();
        uint64_t getLogicDataSize();
        string getLogicData();
        string getChunkHash();
        bool editType(uint64_t type);
        bool editLogicData(String newLogicData);
        bool editLogicDataSize(uint64_t newSize);
        bool editEncryptKey(string newKey);
        // any additional function of chunk

};
```
### Configure
```c++
class Configure {
    private: 
        // following settings configure by macro set 
        uint64_t _runningType; // localDedup \ serverDedup 
        uint64_t _maxThreadLimits; // threadPool auto configure baseline
        // chunking settings
        uint64_t _chunkingType; // varSize \ fixedSize
        uint64_t _maxChunkSize;
        uint64_t _minChunkSize;
        uint64_t _averageChunkSize;
        uint64_t _segmentSize // if exist segment function
        // key management settings
        uint64_t _keyServerNumber;
        vector<string> _keyServerIP;
        vector<int> _keyServerPort;
        // storage management settings
        uint64_t _storageServerNumber;
        vector<string> _storageServerIP;
        vector<int> _storageServerPort;
        uint64_t _maxContainerSize;
        // any additional settings
    public:
        Configure(); // according to setting json to init configure class
        ~Configure();
        
        uint64_t getRunningType(); 
        uint64_t getMaxThreadLimits(); 
        // chunking settings
        uint64_t getChunkingType(); 
        uint64_t getMaxChunkSize();
        uint64_t getMinChunkSize();
        uint64_t getAverageChunkSize();
        uint64_t getSegmentSize() 
        // key management settings
        uint64_t getKeyServerNumber();
        vector<string> getkeyServerIP();
        vector<int> getKeyServerPort();
        // storage management settings
        uint64_t getStorageServerNumber();
        vector<string> getStorageServerIP();
        vector<int> getStorageServerPort();
        uint64_t getMaxContainerSize();
        // any additional configure function
        // any macro settings should be set here (using "const" to replace "#define")

};
```

## Basic Module Design

### MessageQueue

This part will repackaging using "message_queue" in the Boost library.
Design a repackaging class "MessageQueue".

```c++
//Producer 
#include <boost/interprocess/ipc/message_queue.hpp>   
#include <iostream>   
#include <vector>   
  
using namespace boost::interprocess;  
  
int main () {  
   try{  
      //Erase previous message queue   
      message_queue::remove("message_queue");  
  
      //Create a message_queue.   
      message_queue mq  
         (create_only               //only create   
         ,"message_queue"           //name   
         ,100                       //max message number   
         ,sizeof(int)               //max message size   
         );  
  
      //Send 100 numbers   
      for (int i = 0; i < 100; ++i) {  
         mq.send(&i, sizeof(i), 0);  
      }  
   }  
   catch (interprocess_exception &ex) {  
      std::cout << ex.what() << std::endl;  
      return 1;  
   }  
   return 0;  
}  
```

```c++
//Consumer
#include <boost/interprocess/ipc/message_queue.hpp>   
#include <iostream>   
#include <vector>   
  
using namespace boost::interprocess;  
  
int main () {  
   try {  
      //Open a message queue.   
      message_queue mq  
         (open_only        //only create   
         ,"message_queue"  //name   
         );  
  
      unsigned int priority;  
      message_queue::size_type recvd_size;  
  
      //Receive 100 numbers   
      for (int i = 0; i < 100; ++i) {  
         int number;  
         mq.receive(&number, sizeof(number), recvd_size, priority);  
         if (number != i || recvd_size != sizeof(number))  
            return 1;  
      }  
   }  
   catch (interprocess_exception &ex) {  
      message_queue::remove("message_queue");  
      std::cout << ex.what() << std::endl;  
      return 1;  
   }  
   message_queue::remove("message_queue");  
   return 0;  
}  
```

### Chunker
```c++
class Chunker {
    friend class Configure;
    friend class Chunk;
    private:
        ifstream _chunkingFile;
        // any additional info 
    public:
        Chunker();
        ~Chunker();
        virtual bool chunking() = 0;
        bool insertMQ();
        // any additional functions
};
```
### Retriever
```c++
class Retriever {
    friend class Configure;
    friend class Chunk;
    private:
        ofstream _retrieveFile;
        // any additional info 
    public:
        Retriever();
        ~Retriever();
        virtual bool Retrieve() = 0;
        bool extractMQ();
        // any additional functions
};
```
### Encoder
```c++
class Encoder {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        ofstream encodeRecoder;
        // any additional info
    public:
        Encoder();
        ~Encoder();
        bool extractMQ();
        bool insertMQ(); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool encodeChunk(Chunk newChunk) = 0;
        virtual bool outputEncodeRecoder() = 0;
        // any additional functions
};
```
### Decoder
```c++
class Decoder {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        ofstream decodeRecoder;
        // any additional info
    public:
        Decoder();
        ~Decoder();
        bool extractMQ();
        bool insertMQ(); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool decodeChunk(Chunk newChunk) = 0;
        virtual bool outputDecodeRecoder() = 0;
        // any additional functions
};
```
### Sender
```c++
class Sender {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        // any additional info
    public:
        Sender();
        ~Sender();
        bool extractMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool sendData() = 0; 
        // any additional functions
};
```
### Receiver
```c++
class Receiver {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _outputMQ;
        // any additional info
    public:
        Receiver();
        ~Receiver();
        bool insertMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool receiveData() = 0; 
        // any additional functions
};
```

### Key Management 
```c++
class KeyManager {
    friend class Configure;
    friend class Chunk;
    private:
        deque<Chunk> receiveQue;
        deque<Chunk> sendQue;
        // any additional info
    public:
        KeyManager();
        ~KeyManager();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        virtual bool keyGen() = 0;
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertQue & extractQue threads).
        bool insertQue(); 
        bool extractQue(); 
        // any additional functions
};
```


### DataSR
Using epoll to receive / send data, and threadpool for message queue insert or extract.

#### Boost ThreadPool

When compile, add `LIBS := -lboost_thread`

```c++
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <boost/threadpool.hpp>
 
using namespace std;
using namespace boost::threadpool;
 
void task_1() {
	cout << "task_1 start" << endl;
	cout << "thead_id(task_1): " << boost::this_thread::get_id() << endl;
	for (int i = 0; i < 10; i++) {
		cout << "1111111111111111111111111" << endl;
		sleep(1);
	}
}
 
void task_2() {
	cout << "task_2 start" << endl;
	cout << "thead_id(task_2): " << boost::this_thread::get_id() << endl;
	for (int i = 0; i < 30; i++){
		cout << "222222222222222222222222" << endl;
		sleep(1);
	}
}
 
void DoGetVersionNoForUpdate(int a) {
	cout << "task_3 start" << endl;
	cout << "thead_id(task_3): " << boost::this_thread::get_id() << endl;
	for (int i = 0; i < 5; i++) {
		cout << a*a << endl;
		sleep(1);
	}
}
 
int main(int argc, char *argv[]) {
	//seeting max thread number
	pool tp(10);
	//add thread into schedule
	tp.schedule(&task_1);
	tp.schedule(&task_2);
	int i =10;
	tp.schedule(boost::bind(DoGetVersionNoForUpdate, i));
	return (0);
}
```

#### DataSR class 
```c++
class DataSR {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        // any additional info
    public:
        DataSR();
        ~DataSR();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertMQ & extractMQ threads).
        bool insertMQ(); 
        bool extractMQ(); 
        // any additional functions
};
```
### DedupCore
```c++
class DedupCore {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        vector<leveldb::DB> _dbSet;
        // any additional info
    public:
        DedupCore();
        ~DedupCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool dataDedup() = 0;
        // any additional functions
};
```
### StorageSystem
```c++
class StorageCore {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        vector<leveldb::DB> _dbSet;
        vector<ifstream> _intputContainerSet;
        vector<ofstream> _outputContainerSet; 
        // any additional info
    public:
        StorageCore();
        ~StorageCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool createContainer() = 0;
        virtual bool writeContainer() = 0;
        virtual bool readContainer() = 0;
        // any additional functions
};
```





