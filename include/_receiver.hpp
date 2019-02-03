#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "tmp.hpp"
#include "Sock.hpp"
#include "protocol.hpp"

class _Receiver {
//    friend class Configure;
//    friend class Chunk;
private:
    _messageQueue _outputMQ;
    // any additional info
public:
    _Receiver();
    ~_Receiver();
    bool insertMQ(Chunk &chunk);

        //add
    bool insertMQ(string &keyRecipe);

    //Implemented in a derived class and implements different types of transmissions by overloading the function
    virtual bool receiveData(string &data,int status) = 0;
    _messageQueue getOutputMQ();
    // any additional functions
};
