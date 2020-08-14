import sys
import linecache

msFingerPrint = 10


class chunk:
    def __init__(self):
        self.fP = "" # chunk fingerprint
        self.size = 0 # chunk size


def Usage():
    print("%s [input hash file]" 
        "[output hash file]" % sys.argv[0])


if __name__ == "__main__":
    if (len(sys.argv) != 3):
        Usage()
        exit(1)
    HashFile = open(str(sys.argv[1]), "r")
    OutputFile = open(str(sys.argv[2]), "w")
    assert(OutputFile)
    assert(HashFile)
    currentLineNum = 0
    fileStartLine = -1
    fileSize = 0
    fileSizeLine = -1
    # flag to find next file
    FIND_FILE_FLAG = 0
    # flag to find a non-empty file
    NON_EMPTY_FILE = 0
    # check if it is a chunk
    CHECK_FLAG = 0
    # start check flag
    START_CHECK_FLAG = 0

    # file counter
    fileCounter = 0

    # chunk counter
    chunkCounter = 0

    # expected trace size
    expTraceSize = 0

    # real trace size 
    realTraceSize = 0

    # read file size 
    realFileSize = 0

    # zero chunk amount
    zeroChunkNum = 0

    for line in HashFile:
        line = line.strip()
        currentLineNum = currentLineNum + 1
        if (not len(line)) and (FIND_FILE_FLAG == 0):
            # print("file start at line number: %d" %
            #     currentLineNum)
            # record the start line of this file
            fileStartLine = currentLineNum
            # set the flag of find a file
            FIND_FILE_FLAG = 1
            continue
        # read the file size (offset = 5)
        if (currentLineNum == (fileStartLine + 5)) and (FIND_FILE_FLAG == 1):
            fileSize = int(line)
            if fileSize == 0:
                FIND_FILE_FLAG = 0
                # print("It is an empty file, file size: %d" % fileSize)
                expTraceSize = expTraceSize + fileSize
                continue
            else:
                # print("It is an non-empty file, file size: %d" % fileSize)
                fileSizeLine = currentLineNum
                NON_EMPTY_FILE = 1
                expTraceSize = expTraceSize + fileSize
                # increment file counter
                fileCounter = fileCounter + 1 
                continue
        # start to find the chunk starting offset 
        if (NON_EMPTY_FILE == 1) and (currentLineNum == (fileSizeLine + 7)):
            CHECK_FLAG = 1
            continue
        
        if (CHECK_FLAG == 1) and (START_CHECK_FLAG == 0):
            #print(currentLineNum)
            if (len(line)):
                if (line[0] == 'S' or line[0] == 'V' or line[0] == 'A'):
                    continue
                else:
                    # find the start of the chunk hash
                    START_CHECK_FLAG = 1
            else:
                # reset all
                CHECK_FLAG = 0
                START_CHECK_FLAG = 0
                NON_EMPTY_FILE = 0
                FIND_FILE_FLAG = 0
                continue
        
        if (START_CHECK_FLAG == 1):
            if (not len(line)):
                # reset all
                CHECK_FLAG = 0
                START_CHECK_FLAG = 0
                NON_EMPTY_FILE = 0
                FIND_FILE_FLAG = 0

                # start the new file from here
                # print("file start at line number: %d" %
                # currentLineNum)
                # record the start line of this file
                fileStartLine = currentLineNum
                # set the flag of find a file
                FIND_FILE_FLAG = 1
                continue
            elif (CHECK_FLAG == 1):
                if(line[0] != 'z'):
                    # print(line)
                    sizePos = line.rfind(":")
                    size = int(line[sizePos+1:sizePos+11])
                    #print(size)
                    OutputFile.write(str(line[0:2])+":"+str(line[2:4]+":"+str(line[4:6])+":"+str(line[6:8])+":"+str(line[8:10])+'\t\t'+str(size)+"\n"))
                    # increment chunk counter
                    chunkCounter = chunkCounter + 1
                    realTraceSize = realTraceSize + size
                else:
                    sizePos = line.rfind(":")
                    size = int(line[sizePos+1:sizePos+11])
                    zeroChunkNum = zeroChunkNum + 1
                    line = "ffffffffff"
                    OutputFile.write(str(line[0:2])+":"+str(line[2:4]+":"+str(line[4:6])+":"+str(line[6:8])+":"+str(line[8:10])+'\t\t'+str(size)+"\n"))
                    realTraceSize = realTraceSize + size

    HashFile.close()
    OutputFile.close()
    print("Number of files: %d" % fileCounter)
    print("Number of chunks: %d" % chunkCounter)
    print("Zero chunk number: %d" % zeroChunkNum)
    print("Real trace size: %fGB" % (realTraceSize / (1024 * 1024 * 1024)))
