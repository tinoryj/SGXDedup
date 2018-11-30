# SGX

[官方文档-v2.2](https://download.01.org/intel-sgx/linux-2.2/docs/)

#### 安装

> 完全按照官方文档......有点问题，有以下几点注意：

- 硬件支持：

  芯片：可以通过intel 官网搜索对应芯片，参数栏都会指出是否支持

  BIOS：即使芯片支持SGX，但OEM厂商可能不会在bios开放SGX选项（如果存在该选项，设为除了 disable 之外皆可）

  如果芯片不支持或无法找到对应BIOS选项，在安装PSW时会出现 icls.init() failed

- 操作系统支持：

  在官方文档中有列出支持的OS，除了列举出的OS外，其它慎用，比如ubuntu 18.04 在安装PSW时会出现 aesmd 服务无法启动。Debian 同样会出现该问题

- 软件、链接库

  在文档的安装预备中提及需要安装一些工具和链接库，但并不完整，有些库可能在日常使用中已经装上，所以其中并未提及。但新装ubuntu 16.04 肯定还会缺少 libsystemd-dev。否则在编译 dynamic-app 时会出错

- 插件

  安装文档提供eclipse 插件用于项目构建，我不用eclipse。

> 编译安装脚本：buildsgx.sh
>
> virtualbox 虚拟机磁盘：百度云/collection/study/sgx.zip
>
> 模拟器：[sslab,gts3.org](https://github.com/sslab-gatech/opensgx)

#### 概述

* 简单描述：

  SGX的主要功能是保证在Enclave中的程序和程序中的数据不被修改，并能向第三方证明其运行在SGX平台上

* 编程模型：

  开发者将代码和数据划分为可信与不可信两部分，可信部分代码将加载进入Enclave执行，不可信部分通过ECall调用Enclave提供的功能，可信部分通过OCall进行系统调用。两部分的通信由Edge runtime 进行

  ![Ecall-Ocall](media/sgx/Ecall-Ocall.png)

* SGX提供的安全性和完整性保障

  - Encalve measurement:enclave 哈希值，在CPU载入Enclave将会检查Enclave的哈希值
  - Enclave Author's public key:作者的RSA公钥，用于签名Enclave。在封包和远程认证时标识Enclave
  - Security Version Number of the Enclave:Enclave的安全版本号，不同版本号的Enclave安全等级不同，不能进行数据交换和认证
  - Product ID of the Enclave:标识不同app的Enclave，同一作者（作者公钥相同）开发的不同产品（产品ID）同样不能进行解包的操作

* SGX封包与解包

  * 描述：

    > 在Enclave销毁后，通过封包将Enclave状态和数据保存在Enclave外部不可信的媒介中（磁盘，内存...），在重新恢复Enclave时进行解包继续由上次状态开始运行

  * 用例：

    * 保存建立的可信信道状态
    * 异常退出Enclave的恢复

  * 封装策略

    * 使用Enclave的hash作为封装“密钥”，在解封装是需要验证Enclave的hash。

      缺点：在Enclave hash发生改变（升级...）后，数据将无法找回

    * 使用作者密钥封装

* SGX本地证明

  * 描述：

    > 在同一SGX平台上，一个Enclave向另一个Encalve证明它同样运行在SGX平台上（其实是互相证明），两者通过生成DH交换密钥交换机密信息

    ![LocalAttestation](media/sgx/LocalAttestation.png)

  * 过程：

    1、Enclave1向硬件请求生成Report(硬件拥有一个内嵌密钥，密钥不可知且生成过程对用户透明)，向Enclave2发送该Report

    2、Enclave2收到Report后向硬件申请验证（同一个内嵌密钥），验证通过后同样生成自己的Report发往Enclave1

    3、Enclave1验证收到的Report后二者建立可信信道可以交换机密信息

  * 样例代码通信流程：

    * APP**`(test_create_session)`**Enclave_1**`create_session`**Enclave_message_exchange**`session_request_ocall`**untrusted_enclave_message_exchange**`session_request`**Enclave_2样例代码通信过程

    * App创建多个Enclave，在其中一个Enclave(Enclave_1)调用DH库的create_session，得到一个session_info，存入全局`g_src_session_info_map`中，接着调用另一个Enclave(Enclave_2)，另一个Enclave全局`g_src_session_info_map`中取出session_info，继续DH密钥交换过程

      在生成交换密钥后，两个Enclave用DH密钥构建的可信信道进行通信（Session_info中存有密钥）

      ![DH_Flow](media/sgx/DH_Flow.png)

    

    

* SGX远程证明

  * 远程证明模型

    远程认证采用组密钥，所有Intel SGX 平台定义为组0

    本地Enclave生成Recipe发往服务器，服务器向IAS请求验证Recipe的合法性，从而得到客户端程序确实运行在可信SGX平台的保证，从而可以相信客户端提供的Enclave完整性消息未被篡改

    ![RemoteAttestationHighLevel](media/sgx/RemoteAttestationHighLevel.png)

  * 样例代码通信流程

    ![RemoteAttestationFlow](media/sgx/RemoteAttestationFlow.png)

    通信消息：

    * msg0：challenge，client to server

       1、客户端请求Enclave并在下面的第二步产生DH Context

       1、sgx_create_pse_session()	//platform service enclave
       2、sgx_ra_init() 	//内嵌的开发者提供密钥
       3、sgx_close_pse_session()
       	
       2、客户端使用sgx_get_extended_epid_group_id()获得组密钥(EPID)，目前只有一个组标识gid=0

    

    * msg1：client to server

      1、客户端使用sgx_ra_get_msg1获得包含DH密钥的msg1(DH密钥中的ga)

      2、将之前获取的组密钥加入msg1中，发往server

      ​       

    * msg2：server to client

    		1、在接收到msg1后，服务端检查其中的值，将其发往IAS验证，同时生成自己的DH密钥（gb）

    

    * msg3：client to server

      接收到msg2后，客户端调用sgx_ra_proc_msg2生成msg3，同时生成交换密钥，客户端与服务端的认证与可信信道的建立完成

    [更加详细的代码流程](media/sgx/Intel_SGX_Developer_Guide.pdf) p17

    [资源1，更多认证相关（自定义证书）](https://software.intel.com/en-us/articles/code-sample-intel-software-guard-extensions-remote-attestation-end-to-end-example)

#### 编程实践

* 工具

  * `Edger8r Tool`

    在EDL文件中声明可信部分与不可信部分，该工具将据此生成Edge Function的声明与实现

  * `Enclave_Signing Tool`

    使用开发者提供的作者公钥对Enclave进行签名，同时填入Enclave hash等元数据

  * `CPUSVN Configuration Tool` 

    设置Enclave->"Security Version Number of the Enclave"

* 编译模式：

  SIM：模拟模式，不受硬件局限，程序和数据不受保护，可以调试

  HW：硬件模式，与SIM相反

* SGX命名规则（函数名，链接库）

  * *t* / *u* 代表 Trusted / Untrusted
  * sgx打头 SGX提供
  * sim结尾 相同功能的 Sim 模式下版本

* 常用的链接库：

  `libsgx_tcrypto`->`libcrypto`

  `libsgxtstdcxx`，`libsgxstdc`：封装的可信c++/c库

  `libsgx_ukey_exchange`，`libsgx_tkey_exchange`：密钥交换库（在认证过程中使用的DH交换）

  `libsgx_trts`，`libsgx_urts`：Trust runtime/Untrust runtime

* 过程描述
  * 划分可信部分与不可信部分
  * 在edl文件中定义可信部分与不可信部分，使用sgx_edger8r生成边界代码
  * 将不可信部分编译为`app`，可信部分编译为`enclave.so`
  * 将`enclave.so`通过sgx_sign使用作者密钥签名成`enclave_signed.so`
  * app执行时将`enclave_sign.so`装载入SGX enclave
  * app与enclave之间的数据交换由sgx_edger8r生成的边界代码完成

* EDL（Enclave define language）语法

  * from .. import ..

    一个Enclave可以由多个EDL文件描述，使用 from import 引用

  * []

    用于描述函数参数行为及大小，[in]代表传入参数，[out]代表传出参数，格式为：[in/out，buffer_size]

  * 其他部分与c/c++声明一致，Ecall需要声明为public
