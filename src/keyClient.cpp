#include "keyClient.hpp"

extern Configure config;


keyClient::keyClient(){
	_keySecurityChannel=new ssl(config.getkeyServerIP()[0],config.getkeyServerPort[0],CLIENTSIZE);
	_inputMQ.createQueue("chunker to keyClient",READ_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
	_outputMQ.createQueue("keyClient to encoder",WRITE_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
	_minhashBatchSize=100;

	_bnCTX=BN_CTX_new();
	_rsa=RSA_new();
	_key=BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE,"r");
	PEM_read_bio_RSAPublicKey(_key,&_rsa,NULL,NULL);
}

keyClient::~keyClient(){
	delete _securityChannel;
	delete _outputMQ;
	delete _inputMQ;

	BIO_free_all(_key);
	RSA_free(_rsa);
}

void keyClient::run(){
	SSL* connection=_securityChannel.sslConnect();
	while(1){
		vector<Chunk>chunkList(_minhashBatchSize);
		Chunk tmpChunk;
		string segmentKey;
		string minHash="";
		int minHashIndex=0,it=0;
		for(it<_minhashBatchSize){
			//may jam here
			_inputMQ.pop(tmpChunk);
			if(tmpChunk.getChunkHash()<minHash){
				minHash=tmpChunk.getChunkHash();
				minHashIndex=it;
			}
		}

		//search key cache
		/*
		if(cache find){
			for(it:chunkList){
				it.editEncryptKey(catch);
			}
			continue;
		}
		*/

		segmentKey=keyExchange(connection,tmpChunk);

		//write to hash cache

		for(auto it:chunkList){
			it.editEncryptKey(segmentKey);
			_outputMQ.push(it);
		}
	}
	//close ssl connection
}

string keyClient::keyExchange(SSL* connection,Chunk champion){
	string key;
	char buffer[1024];
	key=decoration(champion.getChunkHash());
	_keySecurityChannel.sslWrite(connection,key);
	_keySecurityChannel.sslRead(connection,buffer);
	key=elimination(buffer);
	return key;
}

string keyClient::decoration(string hash){
	BIGNUM *tmp;
	char result[128];
	memset(result,0,sizeof(result));

	BN_prseude_rand(_r,256,-1,0);
	BN_bin2bn(hash.c_str(),32,_h);

	//tmp=hash*r^e mod n
	BN_mod_exp(tmp,_r,_rsa->e,_rsa->n,_bnCTX);
	BN_mod_mul(tmp,_h,tmp,_rsa->n,_bnCTX);
	
	BN_bn2bin(tmp,result+(128-BN_num_bytes(tmp)));

	return result;
}


string keyClient::elimination(string key){
	BIGNUM *tmp;
	char result[128];
	memset(result,0,sizeof(result));

	BN_bin2bn(key.c_str(),128,_h);
	BN_mod_inverse(_invr,_r,_rsa->n,_bnCTX);

	//tmp=key*r^-1 mod n
	BN_mod_mul(tmp,_h,_invr,_rsa->n,_bnCTX);

	BN_bn2bin(tmp,result+(128-BN_num_bytes(tmp)));

	return result;
}
