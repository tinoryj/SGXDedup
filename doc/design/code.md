* keyClient::decoration&&keyClient::elimination

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