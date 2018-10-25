#ifndef RABINCHUNKER_HPP
#define RABINCHUNKER_HPP

#include "chunk.hpp"
#include "_chunker.hpp"
#include "configure.hpp"

#define FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker


extern Configure config;
class RabinChunker:public _Chunker {

private:
    //chunker type setting (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
    bool _chunkerType;
    //chunk size setting
    int _avgChunkSize;
    int _minChunkSize;
    int _maxChunkSize;
    //sliding window size
    int _slidingWinSize;
    /*the base for calculating the value of the polynomial in rolling hash*/
    uint32_t _polyBase;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t _polyMOD;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t *_powerLUT;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t *_removeLUT;
    /*the mask for determining an anchor*/
    uint32_t _anchorMask;
    /*the value for determining an anchor*/
    uint32_t _anchorValue;

    /*
        function : divide a buffer into a number of fixed-size chunks
        input : data buffer(unsigned char *) buffer size(int *)
        output : chunk index list(int *) number of chunks(int)

         @param buffer - a buffer to be chunked
        @param bufferSize - the size of the buffer
         @param chunkEndIndexList - a list for returning the end index of each chunk <return>
         @param numOfChunks - the number of chunks <return>
    */
    //void fixSizeChunking(unsigned char *buffer, int bufferSize, int *chunkEndIndexList, int *numOfChunks);

    /*
        function : divide a buffer into a number of variable-size chunks
        input : data buffer(unsigned char *) buffer size(int *)
        output : chunk index list(int *) number of chunks(int)

        note: to improve performance, we use the optimization in open-vcdiff: "http://code.google.com/p/open-vcdiff/"

         @param buffer - a buffer to be chunked
        @param bufferSize - the size of the buffer
         @param chunkEndIndexList - a list for returning the end index of each chunk <return>
         @param numOfChunks - the number of chunks <return>
    */
    void varSizeChunking();

public:

    /*
        function : constructor of Chunker
        input : chunker type(int [macro]) avgChunkSize(int) minChunkSize(int) maxChunkSize(int) slidingWinSize(int)
        output : NULL

         @param chunkerType - chunker type (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
         @param avgChunkSize - average chunk size
         @param minChunkSize - minimum chunk sizea
         @param maxChunkSize - maximum chunk size
         @param slidingWinSize - sliding window size
    */
    RabinChunker();

    RabinChunker(std::string path);

    /*
        function: destructor of Chunker
    */
    ~RabinChunker();

    /*
         function : accroding chunking type setting to call the select function
        input : data buffer(unsigned char *) buffer size(int *)
        output : chunk index list(int *) number of chunks(int)

         @param buffer - a buffer to be chunked
         @param bufferSize - the size of the buffer
         @param chunkEndIndexList - a list for returning the end index of each chunk <return>
         @param numOfChunks - the number of chunks <return>
    */
    bool chunking();
};

/*
	function : constructor of Chunker
	input : chunker type(int [macro]) avgChunkSize(int) minChunkSize(int) maxChunkSize(int) slidingWinSize(int)
	output : NULL

 	@param chunkerType - chunker type (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
 	@param avgChunkSize - average chunk size
 	@param minChunkSize - minimum chunk sizea
 	@param maxChunkSize - maximum chunk size
 	@param slidingWinSize - sliding window size
*/
RabinChunker::RabinChunker(std::string path) {
    using namespace std;
    loadChunkFile(path);

    /*fixed-size chunker*/
    if (config.getChunkingType() == FIX_SIZE_TYPE) {

        _avgChunkSize = config.getAverageChunkSize();
        cerr << std::endl << "A fixed-size chunker has been constructed!" << endl;
        cerr << "Parameters:" << std::endl;
        cerr << setw(6) << "_avgChunkSize: " << _avgChunkSize << endl;
        cerr << endl;
    }
    /*variable-size chunker*/
    if (config.getChunkingType() == VAR_SIZE_TYPE) {
        int numOfMaskBits;
/*
		if (minChunkSize >= avgChunkSize)  {
			cerr<<"Error: minChunkSize should be smaller than avgChunkSize!"<<endl;
			exit(1);
		}
		if (maxChunkSize <= avgChunkSize)  {
			cerr<<"Error: maxChunkSize should be larger than avgChunkSize!"<<endl;
			exit(1);
		}
*/
        _avgChunkSize = config.getAverageChunkSize();
        _minChunkSize = config.getMinChunkSize();
        _maxChunkSize = config.getMaxChunkSize();
        _slidingWinSize = config.getSlidingWinSize();
        /*initialize the base and modulus for calculating the fingerprint of a window*/
        /*these two values were employed in open-vcdiff: "http://code.google.com/p/open-vcdiff/"*/
        _polyBase = 257; /*a prime larger than 255, the max value of "unsigned char"*/


//        _polyMOD = (1 << 23) - 1; /*_polyMOD - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
        _polyMOD = (1 << 23) - 1;



        /*initialize the lookup table for accelerating the power calculation in rolling hash*/
        _powerLUT = (uint32_t *) malloc(sizeof(uint32_t) * _slidingWinSize);
        /*_powerLUT[i] = power(_polyBase, i) mod _polyMOD*/
        _powerLUT[0] = 1;
        for (int i = 1; i < _slidingWinSize; i++) {
            /*_powerLUT[i] = (_powerLUT[i-1] * _polyBase) mod _polyMOD*/


            //_powerLUT[i] = (_powerLUT[i - 1] * _polyBase) & (_polyMOD - 1);
            _powerLUT[i] = (_powerLUT[i - 1] * _polyBase) % (_polyMOD - 1);


        }
        /*initialize the lookup table for accelerating the byte remove in rolling hash*/


        _removeLUT = (uint32_t *) malloc(sizeof(uint32_t) * 256); /*256 for unsigned char*/


        for (int i = 0; i < 256; i++) {
            /*_removeLUT[i] = (- i * _powerLUT[_slidingWinSize-1]) mod _polyMOD*/


            //_removeLUT[i] = (i * _powerLUT[_slidingWinSize - 1]) & (_polyMOD - 1);
            _removeLUT[i] = (i * _powerLUT[_slidingWinSize - 1]) % (_polyMOD - 1);


            if (_removeLUT[i] != 0) {

                _removeLUT[i] = _polyMOD - _removeLUT[i];
            }
            /*note: % is a remainder (rather than modulus) operator*/
            /*      if a < 0,  -_polyMOD < a % _polyMOD <= 0       */
        }

        /*initialize the mask for depolytermining an anchor*/
        /*note: power(2, numOfMaskBits) = _avgChunkSize*/
        numOfMaskBits = 1;
        while ((_avgChunkSize >> numOfMaskBits) != 1) {

            numOfMaskBits++;
        }
        _anchorMask = (1 << numOfMaskBits) - 1;
        /*initialize the value for depolytermining an anchor*/
        _anchorValue = 0;
        cerr << endl << "A variable-size chunker has been constructed!" << endl;
        cerr << "Parameters: " << endl;
        cerr << setw(6) << "_avgChunkSize: " << _avgChunkSize << endl;
        cerr << setw(6) << "_minChunkSize: " << _minChunkSize << endl;
        cerr << setw(6) << "_maxChunkSize: " << _maxChunkSize << endl;
        cerr << setw(6) << "_slidingWinSize: " << _slidingWinSize << endl;
        cerr << setw(6) << "_polyBase: 0x" << hex << _polyBase << endl;
        cerr << setw(6) << "_polyMOD: 0x" << hex << _polyMOD << endl;
        cerr << setw(6) << "_anchorMask: 0x" << hex << _anchorMask << endl;
        cerr << setw(6) << "_anchorValue: 0x" << hex << _anchorValue << endl;
        cerr << endl;
    }
}

/*
	function: destructor of Chunker
*/
RabinChunker::~RabinChunker() {
    using namespace std;

    if (_chunkerType == VAR_SIZE_TYPE) { /*variable-size chunker*/
        free(_powerLUT);
        free(_removeLUT);
        cerr << endl << "The variable-size chunker has been destructed!" << endl;
        cerr << endl;
    }
    if (_chunkerType == FIX_SIZE_TYPE) { /*variable-size chunker*/
        cerr << endl << "The fixed-size chunker has been destructed!" << endl;
        cerr << endl;
    }
}

/*
	function : divide a buffer into a number of variable-size chunks
	input : data buffer(unsigned char *) buffer size(int *)
	output : chunk index list(int *) number of chunks(int)

	note: to improve performance, we use the optimization in open-vcdiff: "http://code.google.com/p/open-vcdiff/"

 	@param buffer - a buffer to be chunked
	@param bufferSize - the size of the buffer
 	@param chunkEndIndexList - a list for returning the end index of each chunk <return>
 	@param numOfChunks - the number of chunks <return>
*/
void RabinChunker::varSizeChunking() {
    using namespace std;

#ifdef DEBUG
    double tot = 0, cnt = 0;
#endif

    unsigned short int mask = 0x1ff, magic = 0x2B, x = 40320;
    unsigned short int winFp;
    unsigned char readBuffer[512], *chunkBuffer;
    unsigned long long chunkBufferCnt, chunkIDCnt = 0;
    unsigned long long maxChunkSize = config.getMaxChunkSize();
    unsigned long long minChunkSize = config.getMinChunkSize();
    unsigned long long slidingWinSize = config.getSlidingWinSize();

    chunkBuffer = new unsigned char(maxChunkSize);
    Chunk *tmpchunk;

    ifstream &fin = getChunkingFile();

    while (1) {
        fin.read((char *) readBuffer, sizeof(readBuffer));
        int i, len = fin.gcount();
        for (i = 0; i < len; i++) {
            chunkBuffer[chunkBufferCnt] = readBuffer[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize) {
                winFp = winFp + (chunkBuffer[chunkBufferCnt] * _powerLUT[slidingWinSize - chunkBufferCnt]) & _polyMOD;
                chunkBufferCnt++;
                continue;
            }
            winFp &= (_polyMOD);

            /*slide window*/
            unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize];//queue
            winFp = ((winFp + _removeLUT[v]) * _polyBase +chunkBuffer[chunkBufferCnt]) & _polyMOD;//remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minchunksize*/
            if (chunkBufferCnt < minChunkSize)continue;

            /*find chunk pattern*/
            if ((winFp & _anchorMask) == _anchorValue) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//                cout<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif


                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }

            /*chunk's size exceed maxchunksize*/
            if (chunkBufferCnt >= maxChunkSize) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//                cout<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif


                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }
        }
        if (fin.eof())break;
    }

    /*add final chunk*/
    if (!chunkBufferCnt) {
        tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//        cout<<chunkBufferCnt<<endl;
        tot += chunkBufferCnt;
        cnt += 1;
#endif


        chunkBufferCnt = winFp = 0;
        //mq1.push(*tmpchunk);
        delete tmpchunk;
    }

#ifdef DEBUG
    cout << tot / cnt << endl;
#endif
}

/*
 	function : accroding chunking type setting to call the select function
	input : data buffer(unsigned char *) buffer size(int *)
	output : chunk index list(int *) number of chunks(int)

 	@param buffer - a buffer to be chunked
 	@param bufferSize - the size of the buffer
 	@param chunkEndIndexList - a list for returning the end index of each chunk <return>
 	@param numOfChunks - the number of chunks <return>
*/
bool RabinChunker::chunking() {
    /*fixed-size chunker*/
    if (config.getChunkingType() == FIX_SIZE_TYPE) {

        //fixSizeChunking();
    }
    /*variable-size chunker*/
    if (config.getChunkingType() == VAR_SIZE_TYPE) {

        varSizeChunking();
    }
}






/*
	function : divide a buffer into a number of fixed-size chunks
	input : data buffer(unsigned char *) buffer size(int *)
	output : chunk index list(int *) number of chunks(int)

 	@param buffer - a buffer to be chunked
	@param bufferSize - the size of the buffer
 	@param chunkEndIndexList - a list for returning the end index of each chunk <return>
 	@param numOfChunks - the number of chunks <return>
*/

/*
void RabinChunker::fixSizeChunking(unsigned char *buffer, int bufferSize, int *chunkEndIndexList, int *numOfChunks){

	int chunkEndIndex;
	(*numOfChunks) = 0;
	chunkEndIndex = -1 + _avgChunkSize;
	//divide the buffer into chunks
	while (chunkEndIndex < bufferSize) {
		//record the end index of a chunk
		chunkEndIndexList[(*numOfChunks)] = chunkEndIndex;
		//go on for the next chunk
		chunkEndIndex = chunkEndIndexList[(*numOfChunks)] + _avgChunkSize;
		(*numOfChunks)++;
	}
	//deal with the tail of the buffer
	if (((*numOfChunks) == 0) || (((*numOfChunks) > 0) && (chunkEndIndexList[(*numOfChunks)-1] != bufferSize -1))) {
		//note: such a tail chunk has a size < _avgChunkSize
		chunkEndIndexList[(*numOfChunks)] = bufferSize -1;
		(*numOfChunks)++;
	}
}
*/



#endif