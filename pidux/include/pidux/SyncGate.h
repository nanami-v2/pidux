// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <mutex>
#include <boost/container/static_vector.hpp>

#include "./SyncGateCallback.h"

namespace pidux {

#ifndef PIDUX_SYNC_GATE_CALLBACK_MAX_COUNT
#define PIDUX_SYNC_GATE_CALLBACK_MAX_COUNT 32
#endif

class SyncGate {
public:
    static constexpr std::size_t CallbackMaxCount = PIDUX_SYNC_GATE_CALLBACK_MAX_COUNT;
public:
    SyncGate() noexcept = default;
    SyncGate(SyncGate const&) = delete;
    SyncGate(SyncGate&&) noexcept = delete;
    ~SyncGate() noexcept = default;

    SyncGate& operator=(SyncGate const&) = delete;
    SyncGate& operator=(SyncGate&&) noexcept = delete;

    void addLockDependency(SyncGateCallback& callback);
    void removeLockDependency(SyncGateCallback& callback);
    void requestUnlock() noexcept;
private:
    struct SharedData {
        unsigned int lockCount{0};
        unsigned int lockCountMax{0};
        boost::container::static_vector<SyncGateCallback*, CallbackMaxCount> callbacks;
    };
    SharedData sharedData_;
    std::mutex sharedDataMutex_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/
inline void SyncGate::addLockDependency(SyncGateCallback& callback) {
    std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

    this->sharedData_.lockCount++;
    this->sharedData_.lockCountMax++;
    this->sharedData_.callbacks.push_back(&callback);
}

inline void SyncGate::removeLockDependency(SyncGateCallback& callback) {
    bool unlocked = false;
    bool locked = false;
    boost::container::static_vector<SyncGateCallback*, CallbackMaxCount> callbacks;
    {
        std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

        auto const newEnd = std::remove(this->sharedData_.callbacks.begin(), this->sharedData_.callbacks.end(), &callback);
        auto const removeCount = std::distance(newEnd, this->sharedData_.callbacks.end());

        assert(static_cast<unsigned int>(removeCount) <= this->sharedData_.lockCount);
        assert(static_cast<unsigned int>(removeCount) <= this->sharedData_.lockCountMax);

        if (removeCount == 0)
            return;

        this->sharedData_.lockCount -= removeCount;
        this->sharedData_.lockCountMax -= removeCount;
        this->sharedData_.callbacks.erase(newEnd, this->sharedData_.callbacks.end());

        if (this->sharedData_.lockCount == 0) {
            this->sharedData_.lockCount = this->sharedData_.lockCountMax;

            unlocked = true;
            locked = true;
            callbacks = this->sharedData_.callbacks;
        }
    }
    if (unlocked)
        for (auto* const callback : callbacks)
            callback->onUnlocked();

    if (locked)
        for (auto* const callback : callbacks)
            callback->onLocked();
}

inline void SyncGate::requestUnlock() noexcept {
    bool unlocked = false;
    bool locked = false;
    boost::container::static_vector<SyncGateCallback*, CallbackMaxCount> callbacks;
    {
        std::unique_lock<std::mutex> lock{this->sharedDataMutex_};
        if (this->sharedData_.lockCount > 0)
            this->sharedData_.lockCount--;

        if (this->sharedData_.lockCount == 0) {
            this->sharedData_.lockCount = this->sharedData_.lockCountMax;

            unlocked = true;
            locked = true;
            callbacks = this->sharedData_.callbacks;
        }
    }
    if (unlocked)
        for (auto* const callback : callbacks)
            callback->onUnlocked();

    if (locked)
        for (auto* const callback : callbacks)
            callback->onLocked();
}

} /* namespace pidux */