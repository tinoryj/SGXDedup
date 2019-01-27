* Structure

  * File Recipe

    ```c
    struct fileRecipe{
        string _fileName;		// =hash(origin name)
        uint32_t _fileSize;		// bytes
        //uint32_t _chunkCnt;
        string _createData;
        struct body{
            uint32_t _chunkID;
            uint32_t _chunkSize;
            char _chunkHash[128];
        };
        vector<body>_body;
    };
    ```

  * key Recipe

    ```c
    struct keyRecipe{
        string _filename;	// =hash(origin name)
        uint32_t _fileSize;	// bytes
        string _createData;
        struct body{
            uint32_t _chunkID;
            char _chunkHash[128];
            string _chunkKy;
        };
        vector<body>_body;
    };
    ```

    

  * Container

    ```c
    struct Container{
      uint32_t _used;		//bytes
      char buffer[4<<20];	//4 M
    };
    ```

    

* Database (key - value) (leveldb, Mongodb, Redis)

  * dedup database

    ```c
    key_value{
        key{
            string chunkHash;
        }
        value{
            string containerName;
            uint32_t offset;
            uint32_t length;
        }
    }
    ```

  * file location database

    ```c
    key_value{
        key{
            string filename;	//=hash(origin name)
        }
        value{
            string fileRecipeName;
            string keyRecipeName;
            uint32_t uid;
            uint32_t version;
        }
    }
    ```