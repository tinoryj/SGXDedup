###一 、文档说明

### 二、总体工作流程描述

#### 2.1 备份文件

* chunker 使用 Rabin hash/fix size chunking 进行分块，填写chunk 中的以下内容：

  * Logic data && data size：数据块内容以及长度
  * chunk ID：数据块在文件中的序号
  * chunk hash：数据块哈希
  * Recipe：所属的（file recipe && key recipe）地址

  同时，将数据块信息（chunk ID --- chunk size --- chunk hash）记录到所属file recipe中，并在 key recipe 中为数据块分配空间。

  最后，将数据块通过 MQ 发往 下一模块 key client 获取加密密钥

* key client 使用 min hash 算法，得到代表块，对代表块哈希进行变换后发往 key server 请求密钥，填写 chunk 中的 encrypt key，并通过chunk id 和 Recipe，找到key recipe 中的相应位置，填入数据块的 encrypt key。

  最后，将数据块通过 MQ 发往 下一模块 encoder 对数据块进行加密

* encoder 提取chunk 密钥，进行加密后覆盖chunk Logic data

  最后，将数据块通过 MQ 发往 下一模块 pow client 进行客户端去重

* pow client 打包多个块的数据交给 enclave，enclave 使用远程证明中获得的 SK 对称密钥计算 数据块 “哈希”的 cmac

  pow 将数据块哈希列表及 cmac 通过 sender 送往服务器（此处不通过 MQ ）验证后获得服务器缺少的块，将缺少的数据块通过 MQ 发往 下一模块 sender 发往服务器

* sender 从 MQ 中获取数据块，在套接字空闲时（未发送pow 请求信息 和 recipe）,打包多个数据块一起发往服务器

####2.2 恢复文件

* 方案一（当前采用）

  服务器在得知客户端需要的文件后，提取key recipe 发送会客户端，而不发送file recipe，在服务端根据file recipe 取得chunk并发送回客户端。

  该方式保证了模块的独立性，但模块之间的消息传递困难（MQ 为单向），采用的方式是在模块初始化时通过main 将所需文件传给 receiver，所以无法将thread pool 中的部分实现成驻留在内存中的服务，而且将组装文件的任务交给了服务器，同时重复的chunk 也会在网络上多次传输，

* 方案二

  服务器在得知客户端需要的文件后，提取key recipe 和 file recipe 同时发送给客户端，客户端根据需要向服务器请求chunk，同时在 decoder 对 chunk 重复块进行缓存。通过在receiver 中暴露chunk 请求接口由 retriver 调用，可以将thread pool 中的部分实现成驻留在内存中的服务，但破坏了模块的独立性。

* 初始化 receiver，向receiver 传递需要恢复的文件名，receiver 从服务器中接收的第一条消息是key recipe，之后的所有消息都是chunk，都通过 MQ 传递给decoder 处理

* decoder 接收到 key recipe并解密处理后，开启多个线程接收chunk并解密，最后通过MQ 传递到 retriver

* retriver 初始化时得到与 receiver 初始化相同的文件名，从 MQ 中获得chunk后，根据chunk ID 从新组装成文件

####2.3 密钥管理服务器流程

密钥管理服务器关键部分在于数据的接受与发送使用的epoll部分（此处我认为使用epoll 存在问题，也许是我的实现方式不对，但我认为应改为接受队列和发送队列的实现方式，将在本节最后说明）

密钥管理服务器将客户端的每一条密钥请求视为一条message(定义在message.hpp)，由keyServer::runRecv处理message的接受与转发，由keyServer::runKeyGen处理密钥的生成

* 服务器接受到客户端连接请求，创建msg1，缺省message中的hash和key，将套接字状态修改为监听读入

* 服务器监测到套接字读入事件，从epoll中获取msg1，从套接字读取数据填入msg1中的hash，并通过MQ传递给keyServer::runKeyGen。通过MQ传递msg1有两种做法：

  * 方式一：传递对象拷贝（当前使用）

    由于传递的是整个对象，所以msg1已经无用，所以delete msg1

  * 方式二：传递对象地址

    本身epoll 的实现已经引入内容耦合(下一步)，此处传递对象地址再次引入一处内容耦合

* keyServer::runKeyGen从MQ中接受一个message，设为msg2，生成密钥后将套接字状态修改为写状态，由于上一步通过MQ传递的方式有两种，所以此处也有两种方式与上一步一一对应

  * 方式一

    从MQ 中取得message生成密钥后，使用msg2向 epoll 注册套接字写事件，此时，msg2不等于msg1

  * 方式二

    从MQ中取得message 地址并生成密钥，使用msg2 向 epoll 注册套接字写事件，此时，msg2等于msg1

* keyServer::runRecv监测到套接字写事件，从epoll中取出mesage，设为msg3,传给客户端，此时取得的是message 的地址，所以无论方式一还是方式二，msg3等于msg2

当前实现方式存在以下问题：

* message 在以上三个步骤两个函数中存在内容耦合

所以，应放弃该种实现方式，**改一条MQ为两条MQ**，一条为接受队列，一条为发送队列，执行步骤应为如下三步：

* keyServer::runRecv 使用 epoll，当监听到套接字读事件，从套接字读取message并加入接受队列
* keyServer::runKeyGen从接受队列中获取message，生成密钥后将message加入发送队列
* keyServer::runSend从接受队列中获取message，直接发送后将套接字状态改为读监听，用于接受下一条来自客户端的message，返回第一步

#### 2.4 存储服务器流程

##### 上传文件

#####下载文件

### 三、主要模块

* chunker
* keyClient
* encoder
* powClient
* sender
* retriver
* decoder
* receiver
* dataSR
* powServer
* dedupCore
* storageCore

### 四、支持模块以及数据结构

* fileRecipe_t

  fileRecipe 在内存中的存在形式，可在序列化后转存于外存中、

* keyRecipe_t

  keyRecipe 在内存中的存在形式，可在序列化后转存于外存中

* Recipe_t

  fileRecipe 和 keyRecipe 的组合体，用于网络传输（其实我认为可以将其作为存储在外存中的结构，而不用分别存储 fileRecipe 和 keyRecipe）

* epoll_message

  用于存储服务器epoll实现的数据搭载，搭载内容包括 chunk/sgx messagge/recipe/required Chunk

* networkStruct

  网络传输结构体，用于搭载所有在网络上传输的内容

* powSignedHash(for sgx)

* signedHash(for dedupCore)

* container

* ssl

* Socket

  > Socket 模块维护一个网络 fd , 提供init, finish, Listen, Send, Recv, setNonBlock 六个方法

  * init

    **用于初始化套接字，与构造函数作用相同，故不再介绍构造函数**

    输入参数：

    > int type: 定义于 Socket.hpp, 共有3种, SERVERTCP \ CLIENTTCP \ UDP, 其中 UDP　尚未实现

    > string ip: 不解释

    > int port: 不解释

    出错指示：

    > Error at bind fd to Socket
    >
    > ​	在上次运行中，由服务端先行关闭了TCP连接，端口处于 TIME_WAIT状态，根据系统环境不同需要等待30s\1 min端口将会恢复正常

  * finish

    **用于关闭fd**

    **注意：**不要在析构函数中调用，否则当临时变量析构时会错误关闭网络

  * Listen

    **用于服务器监听客户端连接**

    返回参数：

    Socket

  * Send

    **用于发送数据**

    输入参数：

    > string buffer: 用于发送的内容，建议以string 形式传入，否则注意 char* 向 string 的转换

    返回参数：

    > bool status: 发送成功即为真，对端连接关闭则为假

    为了兼容epoll 需要的非阻塞套接字，将在每条消息发送之前发送消息长度，只有接收端接收到足够内容时才会退出，保证读完接受缓存中的所有内容

  * Recv

    输入参数：

    > string &buffer: 接受的内容，特别注意此处为引用

    返回参数：

    > bool status: 接收成功即为真，对端连接关闭则为假

    参照Send 方法，接收每条消息之前先接收消息长度

* CryptoPrimitive

* serialization

  > serialization 定义与实现于 serialization.hpp 中，只包含serialize　和　deserialize 两个模板函数，分别用于将各种数据序列化以及反序列化，以在网络中传输或在外存中存储

  **注意：**该模块在序列化时会进行对象跟踪，直到基本数据类型(int, char....)。比如以下两种情况：

  * 自定义对象 a 的属性有自定义对象 b, 在对a 序列化时也会对b 序列化，这意味着：**所有可序列化的对象其所有属性也是可序列化的**

  * 自定义对象 a 的属性有自定义对象 b 的指针，在对a 序列化时也会对b 序列化，这意味着：**序列化会追踪指针**。此处可以查看 Chunk 中 对 Recipe_t * 的处理

    当然，也可以只序列化对象的一部分属性，具体使用待补充

* chunkCache_t
* Timer
* database
* configure
* keyCache
* _messageQueue

### 五、涉及技术

* rabin hash

* min hash

* RSA 盲签名

  RSA盲签名用于客户端向密钥管理服务器申请密钥过程，本质就是RSA加解密过程，利用了RSA算法的乘同态性（同态加密算法 gentry-homomorphic-encryption.pdf）

* 多路复用IO epoll

* openssl
  * aes
  * rsa
  * ecc
  * DH
  * cmac
  * x509
  * 自签名证书

* leveldb

* intel sgx

  * 基本

  * 远程证明

  * 证书申请

    证明服务器向 IAS 请求验证服务构建的自签名证书并向 intel 注册的流程

    * 创建x509证书拓展项配置文件 `client.cnf`，配置项含义[点此处](https://software.intel.com/en-us/articles/certificate-requirements-for-intel-attestation-services)

      ```bash
      [ ssl_client ]
      keyUsage = digitalSignature, keyEncipherment, keyCertSign
      subjectKeyIdentifier=hash
      authorityKeyIdentifier=keyid,issuer
      extendedKeyUsage = clientAuth, serverAuth
      ```

    * 生成私钥

      ```bash
      openssl genrsa -out client.key 2048
      ```

    * 生成认证请求文件

      ```bash
      openssl req -key client.key -new -out client.req
      ```

    * 使用 client.cnf 中的拓展项生成证书

      ```bash
      openssl x509 -req -days 365 -in client.req -signkey client.key -out client.crt -extfile client.cnf -extensions ssl_client
      ```

    * 导出pfx文件，用于携带证书及私钥，并设置号导出密码

      ```bash
      openssl pkcs12 -export -out client.pfx -inkey client.key -in client.crt
      ```

    * 生成 txt 格式用于通过邮件发往intel 进行注册

      ```bash
      openssl x509 -in client.crt -text | cat > client.txt
      ```

    * 在[此网站](https://software.intel.com/en-us/form/sgx-onboarding)填写相关信息后将会收到intel 发来的邮件，将 client.txt 以附件的形式回复该邮件

* 定时器
  * 时间轮算法
  * 基于优先队列的实现

* boost

  * serialization

    boost serialization是该实现中最基础的部分，在各个模块中都有涉及，可以与下面提到的message queue构成了编程实现部分的骨架，它的优缺点如下：

    * 优点：
      * 可以方便的将几乎所有数据结构序列化，用于 MQ 和 网络 上的数据传输
      * 实现及使用简单，仅有两个函数即可包含所有数据的序列化和反序列化
      * 增删方便，对自实现结构体，可以采用侵入式的方式，可选的对结构体属性进行序列化
    * 缺点
      * 属于高层次的实现，效率不高
      * 序列化结果的大小无法预测，甚至会超出原始数据的3倍

  * message queue

    boost message queue是该实现中各模块之间连接的重要部分，它的优缺点如下：

    * 优点：
      * 无需实现共享内存
      * 提供进程安全，对于多进程(多线程)无需考虑锁的问题
      * 无需考虑数据包边界问题
      * 包含定时机制time_receive和time_send
    * 缺点
      * 效率不是很理想

  * lru_cache

    

  * muti thread

  * json

* socket