// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <mutex>
#include <boost/container/static_vector.hpp>

#include "./GateCallback.h"

namespace pidux {

#ifndef PIDUX_GATE_CALLBACK_MAX_COUNT
#define PIDUX_GATE_CALLBACK_MAX_COUNT 32
#endif

class Gate {
public:
    static constexpr std::size_t CallbackMaxCount = PIDUX_GATE_CALLBACK_MAX_COUNT;
public:
    Gate() noexcept = default;
    Gate(Gate const&) = delete;
    Gate(Gate&&) noexcept = delete;
    ~Gate() noexcept = default;

    Gate& operator=(Gate const&) = delete;
    Gate& operator=(Gate&&) noexcept = delete;

    void addLockDependency(GateCallback& callback);
    void removeLockDependency(GateCallback& callback);
    void requestUnlock() noexcept;
private:
    struct SharedData {
        unsigned int lockCount{0};
        unsigned int lockCountMax{0};
        boost::container::static_vector<GateCallback*, CallbackMaxCount> callbacks;
    };
    SharedData sharedData_;
    std::mutex sharedDataMutex_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/
inline void Gate::addLockDependency(GateCallback& callback) {
    std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

    this->sharedData_.lockCount++;
    this->sharedData_.lockCountMax++;
    this->sharedData_.callbacks.push_back(&callback);
}

inline void Gate::removeLockDependency(GateCallback& callback) {
    bool unlocked = false;
    bool locked = false;
    boost::container::static_vector<GateCallback*, CallbackMaxCount> callbacks;
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

inline void Gate::requestUnlock() noexcept {
    bool unlocked = false;
    bool locked = false;
    boost::container::static_vector<GateCallback*, CallbackMaxCount> callbacks;
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