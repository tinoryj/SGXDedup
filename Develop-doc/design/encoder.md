* input: message queue (KeyClient to encoder)
* output: message queue (encoder to sender)

*coder logic(mutithread->set config.mutlThreadSettings._encoderThreadLimit)
    * collect chunk from KeyClient
    * encoder chunk
    * push chunk to sender