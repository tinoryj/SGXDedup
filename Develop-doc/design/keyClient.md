* input: message queue (chunker to KeyClient)
* output: message queue (KeyClient to encoder)

* code logic(muti thread->set config.mutithreadsettings.keyClientThreadLimit)
    * collect chunk from chunker and do segment
    * search key receipt in kCache,if find goto 4
    * request key receipt from keyServer
    * set segment key for every key in segment and push to encoder
    