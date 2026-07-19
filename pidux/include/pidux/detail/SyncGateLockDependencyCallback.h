
#pragma once
#include <memory>
#include "./ExecutionLineSharedData.h"

namespace pidux::detail {

class SyncGateLockDependencyCallback {
public:
    explicit SyncGateLockDependencyCallback(
        std::size_t syncGateIndex,
        std::weak_ptr<ExecutionLineSharedData> executionLineSharedData
    ):
        syncGateIndex_{syncGateIndex},
        executionLineSharedData_{executionLineSharedData}
    {}
    void onUnlocked() {
        if (auto p = this->executionLineSharedData_.lock()) {
            std::unique_lock<std::mutex> lock{p->mutex};

            p->syncGateUnlockedFlags[this->syncGateIndex_] = true;
            p->cv.notify_one();
        }
    }
private:
    std::size_t syncGateIndex_;
    std::weak_ptr<ExecutionLineSharedData> executionLineSharedData_;
};

} /* namespace pidux::detail */