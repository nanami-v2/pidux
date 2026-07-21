
#pragma once
#include "../SyncGate.h"
#include "./ExecutionLineSharedData.h"

namespace pidux::detail {

class ExecutionLineSyncGateCallback final: public SyncGate::LockDependencyCallback {
public:
    explicit ExecutionLineSyncGateCallback(
        std::size_t syncGateIndex,
        ExecutionLineSharedData& sharedData,
        std::condition_variable& sharedDataCv,
        std::mutex& sharedDataMutex
    ):
        syncGateIndex{syncGateIndex},
        sharedData{sharedData},
        sharedDataCv{sharedDataCv},
        sharedDataMutex{sharedDataMutex}
    {}
    void onUnlocked() override {
        std::unique_lock<std::mutex> const lock{
            sharedDataMutex
        };
        sharedData.syncGateUnlockedFlags[syncGateIndex] = true;
        sharedDataCv.notify_one();
    }
private:
    std::size_t syncGateIndex;
    ExecutionLineSharedData& sharedData;
    std::condition_variable& sharedDataCv;
    std::mutex& sharedDataMutex;
};

} /* namespace pidux::detail */