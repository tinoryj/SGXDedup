##### Missing part

* user identified

##### Network Structure

```c
struct {
    uint8_t type;
    //uint32_t length;
	unsigned char *body;
}
```

* type

| Value |    Direction     |            Meaning            | Call |Return|
| :---: | :--------------: | :---------------------------: | :--: |:--:|
| 0x00  | Client to Server |   Attestation Process Msg01   | process_msg01 |Msg2|
| 0x01  | Server to Client |   Attestation Process Msg2    | sgx_ra_proc_msg2 |Msg3|
| 0x02  | Client to Server |   Attestation Process Msg3    | process_msg3 |ra_session_t|
| 0x03  | Client to Server | Body = {signature, hashLists} | dupCore |required Chunks|
| 0x04  | Client to Server |     Body = {Chunk lists}      | dupCore |NULL|
| 0x05  | Server to Client |  Body = {Required Chunk ID}   | socket send |NULL|
| 0x06  | Server to Client |     Body = {File Recipt}      | socket send |NULL|
| 0x07  | Server to Client |     Body = {Chunk lists}      | socket send |NULL|
| 0x08  | Client to Server | Body = {download file name} | Storage System |NULL|



* body

  | part1  | part2 | part1  | part2 | ...  |
  | ------ | ----- | ------ | ----- | ---- |
  | length | data  | length | data  | ...  |

##### session structure

```c
struct session_t{
    //uint32_t CID;
    uint8_t vk[16];
    int fd;
    struct sockaddr* addr;
}
```

* VK

  verify key get from RA Process

* fd

* addr

#####Flow



