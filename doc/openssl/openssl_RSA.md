 ```c
#include <openssl/rsa.h>

 int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d);
 int RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q);
 int RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp);
 void RSA_get0_key(const RSA *r,
                   const BIGNUM **n, const BIGNUM **e, const BIGNUM **d);
 void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q);
 void RSA_get0_crt_params(const RSA *r,
                          const BIGNUM **dmp1, const BIGNUM **dmq1,
                          const BIGNUM **iqmp);
 void RSA_clear_flags(RSA *r, int flags);
 int RSA_test_flags(const RSA *r, int flags);
 void RSA_set_flags(RSA *r, int flags);
 ENGINE *RSA_get0_engine(RSA *r);
```

RSA_get0_key 返回RSA公钥. 若公钥已被设置, 将会返回n/e/d的 RSA 内部地址, 不可以被外部delete. 如如果RSA 公钥未被设置, 这指针为 NULL
