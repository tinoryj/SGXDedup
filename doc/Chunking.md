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

  2 GB:

  ​	S: 4 s

  ​	R: 6 s

  4 GB:

  ​	S: 8 s

  ​	R: 11.5 s