## May 3 2019:

> Tinoryj

### SGX-Version-dev Branch

* Contains basic version of dedup-sgx based on blind signature method keyGen at commit "basic version / not optmization version". 


### Fixed Problems

[*] Fixed pow_enclave_u.h and pow_enclave_t.h include error.


## Apr 26 2019:

> Tinoryj

### New Bugs

* All multithread setting leads to errors because boost::serialization (will be removed by using ringbuffer -> don't need serialization).
* pow_enclave & km_enclave build error by using "pow_enclave_u.h" include file (not compact to User Guide of SGX).

### Fixed Problems

[*] Keymanger BlindRSA signature error lead to generate wrong key (not functional).
[*] Client POW enclave set too large enclave size lead to enclave error with error code `0003-memory overflow`.
[*] KeyClient batch size not fit for all different chunk size.
[*] Server storage folders `fr`, `kr`, `cr` not auto crate.

## Apr 22 2019:

> Tinoryj

### Known Bugs

* Keymanger enclave need to rewrite (not functional).
* Keymanger BlindRSA signature error lead to generate wrong key (not functional).
* Client POW enclave set too large enclave size lead to enclave error with error code `0003-memory overflow`.
* KeyClient batch size not fit for all different chunk size.
* Server storage folders `fr`, `kr`, `cr` not auto crate.
* Server peer close early with big `"client":"_sendChunkBatchSize":1000` and  `"MessageQueue":"_messageQueueCnt":1000` (have most important influence of system throughput).

### Throught Problems

> All Vtune result in ./Vtune-dedup-sgx
> Best result now is 10M/s with messageQueue size = 100.

* Message queue used more than 50% CPU time in client.
* In `CryptoPrimitive::message_digest`, too slow hash function cost more than 16% CPU time in client.
* `boost::interprocess` thread_cond_timedwait cost more than 10% CPU time in client.


## Old Update Logs

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
