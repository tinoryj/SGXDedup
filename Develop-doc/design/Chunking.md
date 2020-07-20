* input: chunking file path
* output: message queue (chunker to KeyClient) 

* SimpleChunking

  hash function: winFp = win[0] xor win[1] xor .... xor win[slidingWinSize-1]

* RabinChunking

  hash function: winFp = $$win[0]\times polyBase^{slidingWinSize-1}+win[1]\times polyBase^{slidingWinSize-2}+....+win[slidingWinSize-1]\times polyBase^0$$

* run test

  ```bash
  dd if=/dev/urandom of=[testFile] count=2048000 bs=1024
  ## set config.json/ChunkerConfig/_chunkerType=0 to test SimpleChunker
  ./main [testfile] config.json
  
  ## set config.json/ChunkerConfig/_chunkerType=1 to test RabinChunker
  ./main [testfile] config.json
  ```

* test result

  2GB/4GB/8GB

  130~140 MB/s

* code logic(Single Thread):

  1、read chunking file to `waitingChunkingFile`

  2、process every bytes in `waitingChunkingFile` and copy to `chunkBuffer`

  3、if the condition satisfying,add a chunk

  4、break if file EOF,else goto1
  