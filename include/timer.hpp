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
    ChunkCache* cache_;
    Timer()
    {
        outPutMQ_ = new messageQueue<StorageCoreData_t>(3000);
        cache_ = new ChunkCache;
    }
    ~Timer() {}
    void registerHashList(signedHashList_t& job)
    {
        for (auto it : job.hashList) {
            cache_->refer(it);
        }

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

            if (emptyFlag) {
                this_thread::sleep_for(chrono::milliseconds(500));
                continue;
            }
            now = std::chrono::high_resolution_clock::now();
            dtn = duration_cast<milliseconds>(now - nowJob.startTime);
            if (dtn.count() - nowJob.outDataTime < 0) {
                this_thread::sleep_for(milliseconds(nowJob.outDataTime - dtn.count()));
            }
            cerr << "Timer : check time for " << setbase(10) << nowJob.hashList.size() << " chunks" << endl;
            StorageChunkList_t chunkList;
            if (!checkDone(nowJob, chunkList)) {
                timeOut(nowJob);
                cerr << " server wait chunks timeout" << endl;
                return;
            }
            cerr << "success" << endl;
        }
    }
    void startTimer()
    {
        boost::thread th(boost::bind(&Timer::run, this));
        th.detach();
    }

    bool timeOut(signedHashList_t nowJob)
    {
        int size = nowJob.chunks.size();
        for (int i = 0; i < size; i++) {
            cache_->derefer(nowJob.hashList[i]);
        }
        return true;
    }
    bool checkDone(signedHashList_t nowJob, StorageChunkList_t& chunkList)
    {
        string chunkLogicData;
        bool success;
        for (auto it : nowJob.hashList) {
            success = cache_->readChunk(it, chunkLogicData);
            if (!success) {
                return false;
            } else {
                StorageCoreData_t newData;
                newData.logicDataSize = chunkLogicData.length();
                memcpy(newData.chunkHash, &it[0], CHUNK_HASH_SIZE);
                memcpy(newData.logicData, &chunkLogicData[0], newData.logicDataSize);
                insertMQToStorageCore(newData);
            }
        }
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
