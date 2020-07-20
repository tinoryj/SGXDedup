* KeyClient::decoration&&KeyClient::elimination

  由keyClient::run调用，decoration->network->elimination，run为多线程函数，因为RSA盲签名中存在状态信息（随机数 - r）,所以存在一致性问题

  * 实现方式1：每个对象对应一个线程，抽象的理解为开启多个keyClient，使用对象属性保存状态信息

  * 实现方式2：一个对象对应多个线程，抽象理解为开启一个keyClient黑盒，keyClient本身支持多线程，即需要多少个线程即调用多少次run

    * 实现方式2.1：使用Context保存状态

      缺点：多出Context类型，为了保持代码整体一致，需要多出许多Context类型，增加代码复杂度

    * 实现方式2.2：在run中申请r，将r作为参数传递给decoration和elimination（当前选择）

      缺点：感觉难看，r不在称为类属性，RSA盲签名步骤部分暴露于上层函数，产生”公共耦合“

    * 实现方式2.3：在decoration中申请r，将r作为返回参数传递给elimination

      缺点：产生“数据耦合”

    * 实现方式2.4：预设线程数，预先申请多个r属性，每次打开一个线程使用一个

      缺点：查询可用r树形增加开销和代码复杂性

* RSA公钥格式

  * 格式1：

    -----BEGIN PUBLIC KEY-----

    -----END PUBLIC KEY-----

    生成方式： openssl rsa -in key -pubout -out pubkey

  * 格式2：

    -----BEGIN RSA PUBLIC KEY-----

    -----END RSA PUBLIC KEY-----

    生成方式：ssh-keygen -f key -e -m pem

  * 区别

    [stackoverflow](https://stackoverflow.com/questions/18039401/how-can-i-transform-between-the-two-styles-of-public-key-format-one-begin-rsa)

    简单来说，格式2仅用于RSA私钥体系，格式一可以用于各种私钥体系，为了识别所以加入OID(对象识别码)，PEM_read_bio_RSAPublicKey函数貌似不能使用格式1

* 密钥缓存大小

  具体大小应根据应用情况进行实验设置，配置项位于config.json中

* config.json内容



* 凡是使用到configure的地方，必须初始化、定义configure在前，否则将会读取错误配置。
  * 方式1：configure定义为全局，在三个实体的主文件最开头进行定义初始化（客户端、密钥管理端、存储服务器端），由构造函数传入配置文件读入配置
  * 方式2：所有用到configure内容的模块都不能定义为全局，通过在main开头读入参数对configure初始化
  * 目前存在问题：可以为每一模块实现一个配置重载函数，但某些模块涉及到内存申请，如果为了保证不会出错而在每次使用时都重新载入配置，对效率会有较大影响

* 在socket和sslwrite,sslread中注意字节序转换

* [远程认证代码流程](https://software.intel.com/en-us/articles/code-sample-intel-software-guard-extensions-remote-attestation-end-to-end-example)

  * 开启PSE(时间、单调计数器等可信功能用于防止重放攻击)（可选）

    调用sgx_create_pse_session()，打开SGX可信平台服务

  * 服务器将公钥传递给客户端（硬编码到客户端enclave中，由enclave签名确保不被修改和分发的enclave只能同特定服务器交互），用户将其作为参数调用sgx_ra_init()生成不透明上下文（挑战性消息），公钥使用椭圆曲线加密体系，_sgx_ec256_public_t定义在sgx_tcrypto.h(ga,gb使用小端序)

  * 关闭PSE(如第一步开启PSE，此处需关闭)

    sgx_close_pse_session()

  * 获取拓展组密钥GID，作为msg0被送往服务器(msg0+ga=msg1)，服务器用于判断是否支持客户端硬件设备（当前GID只有0）

  * 客户端向服务端传送msg1

    ```
    typedef struct _sgx_ec256_public_t
    {
        uint8_t gx[SGX_ECP256_KEY_SIZE];
        uint8_t gy[SGX_ECP256_KEY_SIZE];
    } sgx_ec256_public_t;
    
    
    typedef struct _ra_msg1_t
    {
        sgx_ec256_public_t   g_a;		//64 bytes
        sgx_epid_group_id_t  gid;		//4  bytes  (msg0)
    } sgx_ra_msg1_t;
    ```

  * 服务端接受msg1，向IAS验证GID是否支持，接着产生msg2

    ```c++
    typedef struct _ra_msg2_t
    {
        sgx_ec256_public_t       g_b;			//64 bytes
        sgx_spid_t               spid;			//16 bytes
        uint16_t                 quote_type;	//2  bytes
        uint16_t                 kdf_id;		//2  bytes
        sgx_ec256_signature_t    sign_gb_ga;	//64 bytes
        sgx_mac_t                mac;			//16 bytes
        uint32_t                 sig_rl_size;	//4  bytes
        uint8_t                  sig_rl[];		//sig_rl_size bytes
    } sgx_ra_msg2_t;
    ```

    

  * `Please use the correct uRTS library from PSW package.`

    默认使用了安装目录中的libsgx_urts.so, 将LD_LIBRARY_PATH 改为 /usr/lib 或把 /usr/lib 中的libsgx_urts.so 复制到安装目录 

  * Remote Attestation 中的密钥及获取方式