//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"


extern Configure config;
extern util::keyCache kCache;

keyClient::keyClient(){
    _keySecurityChannel=new ssl(config.getKeyServerIP(),config.getKeyServerPort(),CLIENTSIDE);
    _inputMQ.createQueue("chunker to keyClient",READ_MESSAGE);
    _outputMQ.createQueue("keyClient to encoder",WRITE_MESSAGE);
    _keyBatchSizeMin=(int)config.getKeyBatchSizeMin();
    _keyBatchSizeMax=(int)config.getKeyBatchSizeMax();

    _bnCTX=BN_CTX_new();
    _rsa=RSA_new();
    _key=BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE,"r");
    PEM_read_bio_RSAPublicKey(_key,&_rsa,NULL,NULL);
}

keyClient::~keyClient(){
    delete _keySecurityChannel;

    BIO_free_all(_key);
    RSA_free(_rsa);
}

void keyClient::run(){
    connection con=_keySecurityChannel->sslConnect();

    while(1){
        vector<Chunk>chunkList(100);
        string segmentKey;
        char minHash[32];
        char mask[32];
        int segmentSize;
        int minHashIndex,it;

        memset(mask,'0',sizeof(mask));
        memset(minHash,255,sizeof(minHash));

        for(it=segmentSize=minHashIndex=0;segmentSize<_keyBatchSizeMax;it++){
            //may jam here
            _inputMQ.pop(chunkList[it]);

            segmentSize+=chunkList[it].getLogicDataSize();
            if(memcmp((void*)chunkList[it].getChunkHash().c_str(),minHash,sizeof(minHash))<0){
                memcpy(minHash,chunkList[it].getChunkHash().c_str(),sizeof(minHash));
                minHashIndex=it;
            }

            if(memcmp(chunkList[it].getChunkHash().c_str()+(32-9),mask,9)==0&&segmentSize>_keyBatchSizeMax){
                break;
            }
            /*
            if is end chunk break;
            */
        }

        if(kCache.existsKeyinCache(minHash)){
            segmentKey=kCache.getKeyFromCache(minHash);
            for(auto it:chunkList){
                it.editEncryptKey(segmentKey);
            }
            continue;
        }

        segmentKey=keyExchange(con.ssl,chunkList[minHashIndex]);

        //write to hash cache
        kCache.insertKeyToCache(minHash,segmentKey);

        for(auto it:chunkList){
            it.editEncryptKey(segmentKey);
            _outputMQ.push(it);
        }
    }
    //close ssl connection
}

string keyClient::keyExchange(SSL* connection,Chunk champion){
    string key,buffer;
    key=decoration(champion.getChunkHash());
    _keySecurityChannel->sslWrite(connection,key);
    _keySecurityChannel->sslRead(connection,buffer);
    key=elimination(buffer);
    return key;
}

string keyClient::decoration(string hash){
    BIGNUM *tmp;
    char result[128];
    memset(result,0,sizeof(result));

    BN_pseudo_rand(_r,256,-1,0);
    BN_bin2bn((const unsigned char*)hash.c_str(),32,_h);

    //tmp=hash*r^e mod n
    BN_mod_exp(tmp,_r,_rsa->e,_rsa->n,_bnCTX);
    BN_mod_mul(tmp,_h,tmp,_rsa->n,_bnCTX);

    BN_bn2bin(tmp,(unsigned char*)result+(128-BN_num_bytes(tmp)));

    return result;
}


string keyClient::elimination(string key){
    BIGNUM *tmp;
    char result[128];
    memset(result,0,sizeof(result));

    BN_bin2bn((const unsigned char*)key.c_str(),128,_h);
    BN_mod_inverse(&invr,_r,_rsa->n,_bnCTX);

    //tmp=key*r^-1 mod n
    BN_mod_mul(tmp,_h,&invr,_rsa->n,_bnCTX);

    BN_bn2bin(tmp,(unsigned char*)result+(128-BN_num_bytes(tmp)));

    return result;
}
