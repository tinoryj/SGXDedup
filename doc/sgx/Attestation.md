####Remote Attestation

> Intel SGX 远程证明技术用于向网络对端交互的程序证明自己的身份（运行在可信的Intel SGX 平台上）

#####前提条件

* 从可信机构取得证书，或是基于测试目的的自建CA签名的证书

* 注册Intel 开发者服务，用于使用 IAS（Intel Attestation Server）

  * linkable policy && unlinkable policy

    SGX 使用组签名策略，如果使用非连接的签名策略，服务器将只能判断签名来自可信的SGX平台而不能区分在不同平台上的不同用户，而使用连接的签名策略，服务器将能区分不同平台上的用户，将涉及到隐私问题

* 在注册 IAS 服务后，Intel 将会通过邮件发送SPID（Server Provider Indentity）用于与IAS 交互

#####认证过程

###### Msg0（C -> S）{GID}

 GID(Group ID)，用于服务器确认是否支持该平台的SGX服务

* 进入Enclave（ecall 接口）

* 调用 sgx_ra_init()，已硬编码到客户端的服务器ECC公钥为参数， 获取 DHKE  context

  ```c
  typedef struct _sgx_ec256_public_t{
      uint8_t gx[SGX_ECP256_KEY_SIZE];
      uint8_t gy[SGX_ECP256_KEY_SIZE];
  } sgx_ec256_public_t;
  // 小端序
  // 通过硬编码到 enclave.so 中，由SGX自带的完整性机制保证不被修改
  ```

  如需使用SGX 提供的平台服务（计数器、时戳），则遵循以下调用顺序：

  1、sgx_create_pse_session()

  2、sgx_ra_init()

  3、sgx_close_pse_session()

* 调用 sgx_get_extended_epid_group_id 获取

######Msg1（C -> S）{Ga}

常见做法是将Msg0和Msg1一同发往服务端，Msg1中包含由服务端密钥推导出的密钥Ga

* 调用 sgx_ra_get_msg1()，以DHKE context 为输入，返回 sgx_ra_msg1_t

```c
Ga	Ga 	  32 bytes
	ga_y  32 bytes

typedef struct _ra_msg1_t{
    sgx_ec256_public_t   g_a;
    sgx_epid_group_id_t  gid;
} sgx_ra_msg1_t;
// 小端序
```

###### Msg2（S -> C）

* 服务器接受Msg0/1 生成DHKE context

* 生成随机EC密钥Gb

* 由Ga Gb导出 kdk（Key Derivation Key）

  * 由Ga Gb 计算Gab_x
  * 对全零块和Gab_x执行AES-128 CMAC

* kdk导出SMK(SIGMA protocol key)

  * `0x01 || SMK || 0x00 || 0x80 || 0x00`

  

```c
typedef struct _ra_msg2_t
{
    sgx_ec256_public_t       g_b;
    sgx_spid_t               spid;			//服务器标识，前提部分提到获取方式
    uint16_t                 quote_type;	//linkable(0x1) or unlinkable(0x1)
    uint16_t                 kdf_id;	//0x1
    sgx_ec256_signature_t    sign_gb_ga;
    sgx_mac_t                mac;
    uint32_t                 sig_rl_size;
    uint8_t                  sig_rl[];	//查询IAS获得
} sgx_ra_msg2_t;
```

######Msg3（C -> S）

* 调用sgx_ra_proc_msg2()，验证SPID，检查SigRL

> 注：需要连接 libsgx_tservice.a
>
> 在EDL文件中包含以下两行：
>
> ​	include "sgx_tkey_exchange.h"
>
> ​	from "sgx_tkey_exchange.edl" import *;

##### RemoteAttestation 中的密钥

* SK：Signing Key
* MK：Master Key
* VK： Verification Key

获取方式：

```c
sgx_status_t sgx_ra_get_keys(
             sgx_ra_context_t context,
             sgx_ra_key_type_t type,
             sgx_ra_key_128_t *p_key
        );
type:
          KDK = AES-CMAC(key0, gab x-coordinate)
SGX_RA_KEY_MK = AES-CMAC(KDK, 0x01||’MK’||0x00||0x80||0x00)
SGX_RA_KEY_SK = AES-CMAC(KDK, 0x01||’SK’||0x00||0x80||0x00)
SGX_RA_KEY_VK = AES-CMAC(KDK, 0x01||’VK’||0x00||0x80||0x00)
```

##### 实现流程

1、远程证明流程

2、S：map.insert(ClientID，SK)

3、C：调用ecall_calHash(logicData，signature)      signature = encryptWithKey（hashList，SK）

4、C： TLS/SSL send（hashList，signature）

5、S：query SK in map base ClientID and verify hashList

6、S：dedup hashList

......

Local Attestation