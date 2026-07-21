// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <algorithm>
#include <mutex>
#include <boost/container/static_vector.hpp>

#include "./Config.h"

namespace pidux {

class SyncGate {
public:
    class LockDependencyCallback {
    public:
        virtual ~LockDependencyCallback() noexcept = default;
        virtual void onUnlocked() = 0;
    };
public:
    SyncGate() noexcept = default;
    SyncGate(SyncGate const&) = delete;
    SyncGate(SyncGate&&) noexcept = delete;
    ~SyncGate() noexcept = default;

    SyncGate& operator=(SyncGate const&) = delete;
    SyncGate& operator=(SyncGate&&) noexcept = delete;

    unsigned int addLockDependency(
        LockDependencyCallback& lockDependencyCallback
    );
    bool removeLockDependency(
        unsigned int lockDependencyId
    );
    void requestUnlock(
        unsigned int lockDependencyId
    ) noexcept;
    std::size_t lockDependencyCount() const noexcept;
private:
    struct SharedData {
        boost::container::static_vector<
            unsigned int, 
            SyncGateLockDependencyMaxCount
        > lockDependencyIds;
        boost::container::static_vector<
            LockDependencyCallback*,
            SyncGateLockDependencyMaxCount
        > lockDependencyCallbacks;
        boost::container::static_vector<
            bool,
            SyncGateLockDependencyMaxCount
        > lockDependencyUnlockedFlags;

        unsigned int lockDependencyCount{0};
        unsigned int lockDependencyLatestId{0};
    };
    SharedData sharedData_;
    mutable std::mutex sharedDataMutex_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/
inline unsigned int SyncGate::addLockDependency(LockDependencyCallback& lockDependencyCallback) {
    std::unique_lock<std::mutex> lock{
        this->sharedDataMutex_
    };
    this->sharedData_.lockDependencyCount++;
    this->sharedData_.lockDependencyLatestId++;
    this->sharedData_.lockDependencyIds.push_back(this->sharedData_.lockDependencyLatestId);
    this->sharedData_.lockDependencyCallbacks.push_back(&lockDependencyCallback);
    this->sharedData_.lockDependencyUnlockedFlags.push_back(false);

    return static_cast<unsigned int>(this->sharedData_.lockDependencyLatestId);
}

inline bool SyncGate::removeLockDependency(unsigned int lockDependencyId) {
    std::unique_lock<std::mutex> lock{
        this->sharedDataMutex_
    };
    auto const ite = std::find(
        this->sharedData_.lockDependencyIds.begin(),
        this->sharedData_.lockDependencyIds.end(),
        lockDependencyId
    );
    auto const found = (ite != this->sharedData_.lockDependencyIds.end());

    if (!found)
        return false;
        
    auto const lockDepdencyIndex = std::distance(this->sharedData_.lockDependencyIds.begin(), ite);

    this->sharedData_.lockDependencyCount--;
    this->sharedData_.lockDependencyIds.erase(this->sharedData_.lockDependencyIds.begin() + lockDepdencyIndex);
    this->sharedData_.lockDependencyCallbacks.erase(this->sharedData_.lockDependencyCallbacks.begin() + lockDepdencyIndex);
    this->sharedData_.lockDependencyUnlockedFlags.erase(this->sharedData_.lockDependencyUnlockedFlags.begin() + lockDepdencyIndex);

    auto const unlockedCountAfterErase = std::count(
        this->sharedData_.lockDependencyUnlockedFlags.begin(),
        this->sharedData_.lockDependencyUnlockedFlags.end(),
        true
    );
    if (unlockedCountAfterErase == this->sharedData_.lockDependencyCount) {
        /* Reset unlocked flags */
        std::fill(
            this->sharedData_.lockDependencyUnlockedFlags.begin(),
            this->sharedData_.lockDependencyUnlockedFlags.end(),
            false
        );
        for (auto* callback : this->sharedData_.lockDependencyCallbacks)
            callback->onUnlocked();
    }
    return true;
}

inline void SyncGate::requestUnlock(unsigned int lockDependencyId) noexcept {
    std::unique_lock<std::mutex> lock{
        this->sharedDataMutex_
    };
    auto const ite = std::find(
        this->sharedData_.lockDependencyIds.begin(),
        this->sharedData_.lockDependencyIds.end(),
        lockDependencyId
    );
    auto const found = (ite != this->sharedData_.lockDependencyIds.end());

    if (!found)
        return;
        
    auto const lockDepdencyIndex = std::distance(this->sharedData_.lockDependencyIds.begin(), ite);

    this->sharedData_.lockDependencyUnlockedFlags[lockDepdencyIndex] = true;

    auto const unlockedCount = std::count(
        this->sharedData_.lockDependencyUnlockedFlags.begin(),
        this->sharedData_.lockDependencyUnlockedFlags.end(),
        true
    );
    if (unlockedCount == this->sharedData_.lockDependencyCount) {
        /* Reset unlocked flags */
        std::fill(
            this->sharedData_.lockDependencyUnlockedFlags.begin(),
            this->sharedData_.lockDependencyUnlockedFlags.end(),
            false
        );
        for (auto* callback : this->sharedData_.lockDependencyCallbacks)
            callback->onUnlocked();
    }
}

inline std::size_t SyncGate::lockDependencyCount() const noexcept {
    std::unique_lock<std::mutex> lock{
        this->sharedDataMutex_
    };
    return this->sharedData_.lockDependencyCount;
}

} /* namespace pidux */