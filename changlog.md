October 27 2018:
> tinoryj
* mv compile.sh to ../ & edit compile.sh for moving
* mv changlog.txt to changlog.md
* add typename for messageQueue define in _sender.hpp & _storage.hpp
* delete redefine construct function


October 25 2018:
> wsq
* add Configure::readConf(string path)
* add Configure::Configure()
* change Configure::Configure(ifstream &chunkFile) to Configure::Configure(string path)

* add _Chunker::loadChunkFile(string path)
* add _Chunker::_Chunker(string path)

* merge Chunk::editLogicData(string newLogicData) and Chunk::editLogicDataSize(uint64_t newSize) to Chunk::editLogicData(string newLogicDataContent,uint64_t newSize)

* implement SimpleChunker:public _chunker
* implement RabinChunker:public _chunker
* implement Configure::Configure(string path)
