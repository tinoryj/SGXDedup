//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"
#include "../../../../../../public/opt/openssl/include/openssl/rsa.h"


extern Configure config;
extern util::keyCache kCache;

keyClient::keyClient(){
    _keySecurityChannel=new ssl(config.getKeyServerIP(),config.getKeyServerPort(),CLIENTSIDE);
    _inputMQ.createQueue(CHUNKER_TO_KEYCLIENT_MQ,READ_MESSAGE);
    _outputMQ.createQueue(KEYCLIENT_TO_ENCODER_MQ,WRITE_MESSAGE);
    _keyBatchSizeMin=(int)config.getKeyBatchSizeMin();
    _keyBatchSizeMax=(int)config.getKeyBatchSizeMax();

    _bnCTX=BN_CTX_new();
    _keyfile=BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE,"r");

    if(_keyfile== nullptr){
        cerr<<"Can not open keymanger public key file "<<KEYMANGER_PUBLIC_KEY_FILE<<endl;
        exit(1);
    }

    _pubkey=PEM_read_bio_PUBKEY(_keyfile, nullptr, nullptr, nullptr);

    if(_pubkey== nullptr){
        cerr<<"keymanger public keyfile damage\n";
        exit(1);
    }
    _rsa=EVP_PKEY_get1_RSA(_pubkey);
    RSA_get0_key(_rsa,&_keyN,&_keyE, nullptr);
    BIO_free_all(_keyfile);
}

keyClient::~keyClient(){
    delete _keySecurityChannel;

    RSA_free(_rsa);
}

void keyClient::run(){
    std::pair<int,SSL*> con=_keySecurityChannel->sslConnect();

    while(1){
        vector<Chunk>chunkList;
        string segmentKey;
        string minHash;
        minHash.resize(32);
        Chunk tmpchunk;
        char mask[32];
        int segmentSize;
        int minHashIndex,it;

        memset(mask,'0',sizeof(mask));
        memset(&minHash[0],255,sizeof(char)*minHash.length());
        chunkList.clear();

        for(it=segmentSize=minHashIndex=0;segmentSize<_keyBatchSizeMax;it++){

            if(!_inputMQ.pop(tmpchunk)){
                break;
            }

            chunkList.push_back(tmpchunk);

            segmentSize+=chunkList[it].getLogicDataSize();
            if(memcmp((void*)chunkList[it].getChunkHash().c_str(),&minHash[0],sizeof(minHash))<0){
                memcpy(&minHash[0],chunkList[it].getChunkHash().c_str(),sizeof(minHash));
                minHashIndex=it;
            }

            if(memcmp(chunkList[it].getChunkHash().c_str()+(32-9),mask,9)==0&&segmentSize>_keyBatchSizeMax){
                break;
            }
        }

        if(chunkList.empty()){
            continue;
        }

        if(kCache.existsKeyinCache(minHash)){
            segmentKey=kCache.getKeyFromCache(minHash);
            for(auto it:chunkList){
                it.editEncryptKey(segmentKey);
            }
            continue;
        }

        //segmentKey=keyExchange(con.second,chunkList[minHashIndex]);
        segmentKey=keyExchange(con.second,chunkList[minHashIndex]);
#ifdef DEBUG
        std::cout<<"get key : "<<segmentKey<<endl;
#endif
        //write to hash cache
        kCache.insertKeyToCache(minHash,segmentKey);

        for(auto it:chunkList){
            it.editEncryptKey(segmentKey);
            this->insertMQ(it);
        }
    }
    //close ssl connection
}

string keyClient::keyExchange(SSL* connection,Chunk champion){
    string key,buffer;
    BIGNUM *r=BN_new(),*invr=BN_new();
    BN_pseudo_rand(r,256,-1,0);
    BN_mod_inverse(invr,r,_keyN,_bnCTX);

    key=decoration(r,champion.getChunkHash());
    _keySecurityChannel->sslWrite(connection,key);
#ifdef DEBUG
    std::cout<<"request key\n";
#endif
    if(!_keySecurityChannel->sslRead(connection,buffer)){
        std::cerr<<"key server close\n";
        exit(1);
    }
#ifdef DEBUG
    std::cout<<"receive key\n";
#endif
    key=elimination(invr,buffer);
    return key;
}

string keyClient::decoration(BIGNUM* r,string hash){
    BIGNUM *tmp=BN_new(),*h=BN_new();
    char result[128];
    memset(result,0,sizeof(result));

    BN_bin2bn((const unsigned char*)hash.c_str(),32,h);

    //tmp=hash*r^e mod n
    BN_mod_exp(tmp,r,_keyE,_keyN,_bnCTX);
    BN_mod_mul(tmp,h,tmp,_keyN,_bnCTX);

    BN_bn2bin(tmp,(unsigned char*)result+(128-BN_num_bytes(tmp)));

    return result;
}


string keyClient::elimination(BIGNUM* invr,string key){
    BIGNUM *tmp=BN_new(),*h=BN_new();
    char result[128];
    memset(result,0,sizeof(result));

    BN_bin2bn((const unsigned char*)key.c_str(),128,h);

    //tmp=key*r^-1 mod n
    BN_mod_mul(tmp,h,invr,_keyN,_bnCTX);

    BN_bn2bin(tmp,(unsigned char*)result+(128-BN_num_bytes(tmp)));

    return result;
}

bool keyClient::insertMQ(Chunk &newChunk) {
    _outputMQ.push(newChunk);
    Recipe_t* _recipe=newChunk.getRecipePointer();
    keyRecipe_t *kr=&_recipe->_k;
    kr->_body[newChunk.getID()]._chunkKey=newChunk.getEncryptKey();
}