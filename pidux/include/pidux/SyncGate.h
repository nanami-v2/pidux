// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <algorithm>
#include <mutex>
#include <boost/container/static_vector.hpp>

#include "./detail/SyncGateLockDependencyCallback.h"

namespace pidux {

class SyncGate {
public:
    SyncGate() noexcept = default;
    SyncGate(SyncGate const&) = delete;
    SyncGate(SyncGate&&) noexcept = delete;
    ~SyncGate() noexcept = default;

    SyncGate& operator=(SyncGate const&) = delete;
    SyncGate& operator=(SyncGate&&) noexcept = delete;

    unsigned int addLockDependency(
        detail::SyncGateLockDependencyCallback const& lockDependencyCallback
    );
    void removeLockDependency(
        unsigned int lockDependencyId
    );
    void requestUnlock(
        unsigned int lockDependencyId
    ) noexcept;
private:
    struct SharedData {
        boost::container::static_vector<
            unsigned int, 
            detail::SyncGateLockDependencyMaxCount
        > lockDependencyIds;
        boost::container::static_vector<
            detail::SyncGateLockDependencyCallback,
            detail::SyncGateLockDependencyMaxCount
        > lockDependencyCallbacks;
        boost::container::static_vector<
            bool,
            detail::SyncGateLockDependencyMaxCount
        > lockDependencyUnlockedFlags;

        unsigned int lockDependencyCount{0};
        unsigned int lockDependencyLatestId{0};
    };
    SharedData sharedData_;
    std::mutex sharedDataMutex_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/
inline unsigned int SyncGate::addLockDependency(detail::SyncGateLockDependencyCallback const& lockDependencyCallback) {
    std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

    this->sharedData_.lockDependencyCount++;
    this->sharedData_.lockDependencyLatestId++;
    this->sharedData_.lockDependencyIds.push_back(this->sharedData_.lockDependencyLatestId);
    this->sharedData_.lockDependencyCallbacks.push_back(lockDependencyCallback);
    this->sharedData_.lockDependencyUnlockedFlags.push_back(false);

    return static_cast<unsigned int>(this->sharedData_.lockDependencyLatestId);
}

inline void SyncGate::removeLockDependency(unsigned int lockDependencyId) {
    bool unlocked = false;
    boost::container::static_vector<
        detail::SyncGateLockDependencyCallback,
        detail::SyncGateLockDependencyMaxCount
    > lockDependencyCallbacks;
    {
        std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

        auto const ite = std::find(
            this->sharedData_.lockDependencyIds.begin(),
            this->sharedData_.lockDependencyIds.end(),
            lockDependencyId
        );
        auto const found = (ite != this->sharedData_.lockDependencyIds.end());

        if (!found)
            return;
            
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
            /* lock all depdency */
            std::fill(
                this->sharedData_.lockDependencyUnlockedFlags.begin(),
                this->sharedData_.lockDependencyUnlockedFlags.end(),
                false
            );
            unlocked = true;
            lockDependencyCallbacks = this->sharedData_.lockDependencyCallbacks;
        }
    }
    if (unlocked)
        for (auto& e : lockDependencyCallbacks)
            e.onUnlocked();
}

inline void SyncGate::requestUnlock(unsigned int lockDependencyId) noexcept {
    bool unlocked = false;
    boost::container::static_vector<
        detail::SyncGateLockDependencyCallback,
        detail::SyncGateLockDependencyMaxCount
    > lockDependencyCallbacks;
    {
        std::unique_lock<std::mutex> lock{this->sharedDataMutex_};

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
            /* lock all depdency */
            std::fill(
                this->sharedData_.lockDependencyUnlockedFlags.begin(),
                this->sharedData_.lockDependencyUnlockedFlags.end(),
                false
            );
            unlocked = true;
            lockDependencyCallbacks = this->sharedData_.lockDependencyCallbacks;
        }
    }
    if (unlocked)
        for (auto& e : lockDependencyCallbacks)
            e.onUnlocked();
}

} /* namespace pidux */