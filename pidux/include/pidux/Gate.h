// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <mutex>
#include <boost/container/static_vector.hpp>

namespace pidux {

#ifndef PIDUX_GATE_CALLBACK_MAX_COUNT
#define PIDUX_GATE_CALLBACK_MAX_COUNT 32
#endif

class Gate {
public:
    class Callback {
    public:
        virtual ~Callback() noexcept = default;
        virtual void onLocked() = 0;
        virtual void onUnlocked() = 0;
    };
    static constexpr std::size_t CallbackMaxCount = PIDUX_GATE_CALLBACK_MAX_COUNT;
public:
    Gate() noexcept = default;
    Gate(Gate const&) = delete;
    Gate(Gate&&) noexcept = delete;
    ~Gate() noexcept = default;

    Gate& operator=(Gate const&) = delete;
    Gate& operator=(Gate&&) noexcept = delete;

    void connectToLine(Callback& callback);
    void requestUnlock() noexcept;
private:
    struct SharedData {
        unsigned int lockCount{0};
        unsigned int lockCountMax{0};
    };
    SharedData sharedData_;
    std::mutex sharedDataMutex_;
    boost::container::static_vector<Callback*, CallbackMaxCount> callbacks_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/
inline void Gate::connectToLine(Callback& callback) {
    std::unique_lock<std::mutex> const lock{this->sharedDataMutex_};

    this->sharedData_.lockCount++;
    this->sharedData_.lockCountMax++;
    this->callbacks_.push_back(&callback);
}

inline void Gate::requestUnlock() noexcept {
    bool unlocked = false;
    bool locked = false;    
    {
        std::unique_lock<std::mutex> lock{this->sharedDataMutex_};
        if (this->sharedData_.lockCount > 0)
            this->sharedData_.lockCount--;

        if (this->sharedData_.lockCount == 0) {
            this->sharedData_.lockCount = this->sharedData_.lockCountMax;
            /* auto lock */
            unlocked = true;
            locked = true;
        }
    }
    if (unlocked) {
        for (auto* const callback : this->callbacks_)
            callback->onUnlocked();
    }
    if (locked) {
        for (auto* const callback : this->callbacks_)
            callback->onLocked();
    }
}

} /* namespace pidux */