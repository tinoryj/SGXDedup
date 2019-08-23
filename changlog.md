## Aug 23 2019:
> Tinoryj

* Add key server session key life time control.
* key server do remote attestation to storage server to reduce client remote attestation time & make sure key server only run one enclave for all connections.

## Aug 19 2019:

> Tinoryj

Branch : SGX_debug_No_epoll
Commit : Final version, bugs fixed down (c3c862db7550c22bf99d69a159d937bdfd921a51)

### Full test for 20G(20480MB, 21474836480Byte) file upload & download:
* two machines.
* -O3 optimization with clang compiler.
* first round :
    * upload time = 197.782273s, speed = 103.55M/s. 
        - Sender time = 197.776763s
        - PowClient time = 189.691515s
        - KeyClient time = 188.999533s
        - Chunker time = 188.761145s
* second round : 
    >double send same file, not clean server side, upload 0 chunk

    * upload time = 147.361298, speed = 138.98M/s. 
        - Sender time = 147.357486s
        - PowClient time = 146.010096s
        - KeyClient time = 145.930319s
        - Chunker time = 145.930319s
    * download time = 195.584832, speed = 104.71M/s. 

## Aug 16 2019:

> Tinoryj

branch : SGX_debug_No_epoll

* full test for 20G(20480MB, 21474836480B) file upload & download:
    * two machines.
    * -O3 optimization with clang compiler.
    * max speed :
        * upload time = 190.760621, speed = 107.36M/s. 
        * download time = 188.962076, speed = 108.38M/s. 

## Aug 12 2019:

> Tinoryj

branch : SGX_final_debug

* basic test for 1G file upload & download:
    * single machine, 6C6T i5 9600K.
    * -O3 optimization with clang compiler.
    * upload avg 10s, download avg 8s. 

## Aug 11 2019:

> Tinoryj

* basic test for 1G file upload & download:
    * single machine, 6C6T i5 9600K.
    * -O3 optimization with clang compiler, with gdb / AddressSanitizer flag.
    * upload avg 15s, download avg 11s (3 times run). 

## Aug 9 2019:

> Tinoryj

* basic test for 1G file upload & download passed (Aug 8 version):
    * double machines.
    * -O3 optimization with clang compiler, without gdb flag.
    * upload avg 30s, download avg 17s (3 times run). 

* merge keyClient & encoder to decrease thread number & message queue overhead

* basic test for 1G file upload & download passed :
    * single machine, 6C6T i5 9600K.
    * -O3 optimization with clang compiler, with gdb flag.
    * upload avg 17s, download avg 11s (3 times run). 



## Aug 8 2019:

> Tinoryj

* system rewrite done (two enclave & lockfree queue version).
* basic test for 1G file upload & download passed :
    * single machine, 6C6T i5 9600K.
    * no optimization with clang compiler (-O0), with gdb / AddressSanitizer flag.
    * upload avg 27s, download avg 17s (3 times run). 

## Jul 21 2019:

> Tinoryj

* Replace message queue to lock free queue.
* Rewrite system structure (remove general header).

## may 14 2019:

> Tionryj

### Bugs

* Need to generate new spid for km_enclave (waiting for intel)

## May 6 2019:

> Tinoryj

### New Bugs

* key manager enclave remote attestation aborts by server ("msg0 Extended Epid Group ID is not zero.").

### Fixed Bugs

[*] Compile error by openssl & sgxssl link.
[*] Configure compact with two enclave.
[*] Compile error because _keymanager valture function not implemented (remove _keymanager | no use). 
[*] Key manager enclave create failed.

## May 5 2019:

> Tinoryj

### SGX-Version-KM_ENCLAVE Branch

* Contains km_enclave version for key server using sgx enclave.

### SGX_SSL Guide

In order to avoid pthread errors in compile, sgxssl file need to edit after install.

#### Install sgx-ssl

```shell
cp openssl-1.1.1b.tar.gz intel-sgx-ssl-master/openssl_source
cd Linux
source /opt/intel/sgxsdk/environment
make all test
sudo make install
```

#### Modify 

After install sgx-ssl, the include header & libs will copy to `opt/intel/sgxssl/`.

* Step 1: edit file name

```shell
cd /opt/intel/sgxssl/include
sudo mv pthread.h sgxpthread.h
```

* Step 2: edit `crypto.h`

```shell
sudo vim /opt/intel/sgxssl/include/openssl/crypto.h
```

In line 415, change `#    include "pthread.h"` to `#    include "../sgxpthread.h"`.

### Fixed Problems

[*] Fixed pow_enclave_u.h and pow_enclave_t.h include error.


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
