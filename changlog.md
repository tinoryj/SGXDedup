November 24 2018:

> quantu_zo
> 
> * delete _maxThreadLimit in configure
> * add muti thread settings in configure(include _encoderThreadLimit,_keyClientThreadLimt,_keyServerThreadLimt)
> * change _chunker::inserMQ() to virtual _chunker::insertMQ(chunk)

November 11 2018:

> quantu_zo
>
> * add configure.messageQueueCnt and configure.messageQueueUintSize
> * add configure.keyBatchSizeMin and configure._keyBatchSizeMax
> * add invasive Chunk serialize
> * change bool keyManger::keyGen() to bool keyManger::keyGen(string hash,string key)
> * move all implements in hpp to cpp



October 31 2018:

> quantu_zo
>
> * merge SimpleChunker&RabinChunker in chunker.hpp
> * rewrite fixSizeChunking
> * remove _chunkerType in configure
> * update doc/chunking.md
> * add chunkerInit in chunker

October 28 2018:

> quantu_zo

* fix bug in Simple Chunker and Rabin Chunker(-_- new char()->new char[])
* add _ReadSize in Configure

October 27 2018:

> tinoryj
* mv compile.sh to ../ & edit compile.sh for moving
* mv changlog.txt to changlog.md
* add typename for messageQueue define in _sender.hpp & _storage.hpp
* delete redefine construct function


October 25 2018:
> quantu_zo
* add Configure::readConf(string path)
* add Configure::Configure()
* change Configure::Configure(ifstream &chunkFile) to Configure::Configure(string path)

* add _Chunker::loadChunkFile(string path)
* add _Chunker::_Chunker(string path)

* merge Chunk::editLogicData(string newLogicData) and Chunk::editLogicDataSize(uint64_t newSize) to Chunk::editLogicData(string newLogicDataContent,uint64_t newSize)

* implement SimpleChunker:public _chunker
* implement RabinChunker:public _chunker
* implement Configure::Configure(string path)
