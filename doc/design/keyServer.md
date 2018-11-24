input: TlS/SSL connection
output: messge queue (receiver to workload)

* code logic(muti thread->set config.mutlThreadSettings.keyServerThreadLimit)
  * receiver(runRecv):
    * listen epoll event
  
    * if event "in": create message and push to workload
   
    * if event "out": send message(key receipt) to keyClient
        
   * worlload(runkeyGen):
    * collect message from receiver
    * call keyGen and set epoll event to "out"
    
  * keyGen：
    * search key receipt in kCache,if find return
    * genera key