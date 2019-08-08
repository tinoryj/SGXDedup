#ifndef GENERALDEDUPSYSTEM_TIMER_HPP
#define GENERALDEDUPSYSTEM_TIMER_HPP

#include "cache.hpp"
#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>
#include <boost/thread.hpp>

using namespace std;

typedef struct {
    bool done = false;
    string data;
} TimerMapNode;

class Timer {
private:
    struct cmp {
        bool operator()(signedHashList_t x, signedHashList_t y)
        {
            return x.startTime.time_since_epoch().count() > y.startTime.time_since_epoch().count();
        }
    };
    priority_queue<signedHashList_t, vector<signedHashList_t>, cmp> jobQueue_;
    std::mutex timerMutex_;

    messageQueue<StorageCoreData_t>* outPutMQ_;

public:
    unordered_map<string, TimerMapNode> chunkTable;
    std::mutex mapMutex_;
    Timer()
    {
        outPutMQ_ = new messageQueue<StorageCoreData_t>(3000);
    }
    ~Timer()
    {
        delete outPutMQ_;
    }
    void registerHashList(signedHashList_t& job)
    {
        mapMutex_.lock();
        for (auto it : job.hashList) {
            TimerMapNode tempNode;
            tempNode.done = false;
            chunkTable.insert(make_pair(it, tempNode));
        }
        mapMutex_.unlock();
        timerMutex_.lock();
        jobQueue_.push(job);
        timerMutex_.unlock();
    }
    void run()
    {
        using namespace std::chrono;

        system_clock::time_point now;
        duration<int, std::milli> dtn;
        bool emptyFlag;
        while (true) {
            signedHashList_t nowJob;
            timerMutex_.lock();
            emptyFlag = jobQueue_.empty();
            if (!emptyFlag) {
                nowJob = jobQueue_.top();
                jobQueue_.pop();
            }
            timerMutex_.unlock();

            if (!emptyFlag) {
                cerr << "Timer : current job queue size = " << jobQueue_.size() << endl;
                now = std::chrono::high_resolution_clock::now();
                dtn = duration_cast<milliseconds>(now - nowJob.startTime);
                if (dtn.count() - nowJob.outDataTime < 0) {
                    this_thread::sleep_for(milliseconds(nowJob.outDataTime - dtn.count()));
                }
                cerr << "Timer : check time for " << setbase(10) << nowJob.hashList.size() << " chunks" << endl;
                if (!checkDone(nowJob)) {
                    cerr << "Timer : server wait chunks timeout" << endl;
                }
            }
        }
    }
    void startTimer()
    {
        boost::thread th(boost::bind(&Timer::run, this));
        th.detach();
    }

    void PRINT_BYTE_ARRAY_TIMER(
        FILE* file, void* mem, uint32_t len)
    {
        if (!mem || !len) {
            fprintf(file, "\n( null )\n");
            return;
        }
        uint8_t* array = (uint8_t*)mem;
        fprintf(file, "%u bytes:\n{\n", len);
        uint32_t i = 0;
        for (i = 0; i < len - 1; i++) {
            fprintf(file, "0x%x, ", array[i]);
            if (i % 8 == 7)
                fprintf(file, "\n");
        }
        fprintf(file, "0x%x ", array[i]);
        fprintf(file, "\n}\n");
    }

    bool checkDone(signedHashList_t& nowJob)
    {
        string chunkLogicData;
        mapMutex_.lock();
        // cerr << "Timer : current chunk table size = " << chunkTable.size() << endl;
        // for (auto it = chunkTable.begin(); it != chunkTable.end(); it++) {
        //     cerr << "chunk hash in map = " << endl;
        //     PRINT_BYTE_ARRAY_TIMER(stderr, (char*)it->first.c_str(), CHUNK_HASH_SIZE);
        // }
        for (auto it : nowJob.hashList) {
            cerr << "chunk hash in job list = " << endl;
            PRINT_BYTE_ARRAY_TIMER(stderr, (char*)it.c_str(), CHUNK_HASH_SIZE);
            auto temp = chunkTable.find(it);
            if (temp == chunkTable.end()) {
                cerr << "Timer : can not find chunk" << endl;
                // return false;
            } else {
                StorageCoreData_t newData;
                newData.logicDataSize = temp->second.data.length();
                memcpy(newData.chunkHash, &it[0], CHUNK_HASH_SIZE);
                memcpy(newData.logicData, &temp->second.data[0], newData.logicDataSize);
                insertMQToStorageCore(newData);
                chunkTable.erase(it);
            }
        }
        cerr << "Timer : check done" << endl;
        mapMutex_.unlock();
        return true;
    }

    bool insertMQToStorageCore(StorageCoreData_t& newData)
    {
        return outPutMQ_->push(newData);
    }

    bool extractMQToStorageCore(StorageCoreData_t& newData)
    {
        return outPutMQ_->pop(newData);
    }
};

#endif //GENERALDEDUPSYSTEM_TIMER_HPP
