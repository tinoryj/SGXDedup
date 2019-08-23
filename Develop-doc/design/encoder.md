* input: message queue (keyClient to encoder)
* output: message queue (encoder to sender)

*coder logic(mutithread->set config.mutlThreadSettings._encoderThreadLimit)
    * collect chunk from keyClient
    * encoder chunk
    * push chunk to sender