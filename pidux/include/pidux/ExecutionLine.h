// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <bitset>
#include <exception>
#include <optional>
#include <mutex>
#include <thread>
#include <variant>
#include <boost/container/static_vector.hpp>

#include "./SyncGate.h"
#include "./ExecutionUnit.h"
#include "./ExecutionLineCallback.h"
#include "./detail/ExecutionLineSharedData.h"
#include "./detail/SyncGateLockDependencyCallback.h"

namespace pidux {

template<typename T>
class ExecutionLine {
public:
    using Element = std::variant<
        std::reference_wrapper<ExecutionUnit<T>>,
        std::reference_wrapper<SyncGate>
    >;
    struct CreationParams {
        boost::container::static_vector<
            ExecutionLine::Element,
            detail::ExecutionLineElementMaxCount
        > lineElements;
        ExecutionLineCallback<T>* callback{nullptr};
    };
public:
    explicit ExecutionLine(CreationParams const& params);
    ExecutionLine(ExecutionLine const&) = delete;
    ExecutionLine(ExecutionLine&&) noexcept = delete;
    ~ExecutionLine() noexcept;

    ExecutionLine& operator=(ExecutionLine const&) = delete;
    ExecutionLine& operator=(ExecutionLine&&) noexcept = delete;

    void start(T& ctx);
    void destroy() noexcept;

private:
    std::thread thread_;
    std::shared_ptr<detail::ExecutionLineSharedData> sharedData_;

    boost::container::static_vector<
        unsigned int,
        detail::ExecutionLineElementMaxCount
    > syncGateLockDependencyIds_;
    boost::container::static_vector<
        Element,
        detail::ExecutionLineElementMaxCount
    > lineElements_;
    ExecutionLineCallback<T>* callback_;
    bool destroyed_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/

template<typename T>
inline ExecutionLine<T>::ExecutionLine(CreationParams const& params):
    lineElements_{params.lineElements},
    callback_{params.callback},
    sharedData_{std::make_shared<detail::ExecutionLineSharedData>()},
    destroyed_{false}
{
    std::size_t syncGateIndex = 0;

    for (auto& e : this->lineElements_) {
        if (auto* const syncGate = std::get_if<std::reference_wrapper<SyncGate>>(&e)) {
            this->syncGateLockDependencyIds_.push_back(
                syncGate->get().addLockDependency(
                    detail::SyncGateLockDependencyCallback{
                        syncGateIndex,
                        this->sharedData_
                    }
                )
            );
            syncGateIndex++;
        }
    }
}

template<typename T>
inline ExecutionLine<T>::~ExecutionLine() noexcept {
    this->destroy();
}

template<typename T>
inline void ExecutionLine<T>::start(T& ctx) {
    assert(!this->destroyed_);
    
    this->thread_ = std::thread{[this, &ctx]() {
        try {
            if (this->callback_)
                this->callback_->onLineStart(ctx);

            while (true) {
                std::size_t syncGateIndex = 0;
                bool        shutdownFlag = false;

                for (auto& e : this->lineElements_) {
                    auto* const executionUnit = std::get_if<std::reference_wrapper<ExecutionUnit<T>>>(&e);
                    auto* const syncGate = std::get_if<std::reference_wrapper<SyncGate>>(&e);

                    if (executionUnit) {
                        {
                            std::unique_lock<std::mutex> const lock{
                                this->sharedData_->mutex
                            };
                            shutdownFlag = this->sharedData_->shutdownFlag;
                        }
                        if (shutdownFlag) {
                            if (this->callback_)
                                this->callback_->onLineEnd(ctx);
                            return;
                        }
                        try {
                            if (this->callback_)
                                this->callback_->onExecutionUnitStart(ctx, executionUnit->get());

                            executionUnit->get().run(ctx);

                            if (this->callback_)
                                this->callback_->onExecutionUnitEnd(ctx, executionUnit->get());
                        }
                        catch (...) {
                            if (this->callback_) {
                                this->callback_->onExecutionUnitError(ctx, executionUnit->get(), std::current_exception());
                                this->callback_->onLineEnd(ctx);
                            }
                            return;
                        }
                    }
                    if (syncGate) {
                        syncGate->get().requestUnlock(
                            this->syncGateLockDependencyIds_[syncGateIndex]
                        );
                        {
                            std::unique_lock<std::mutex> const lock{
                                this->sharedData_->mutex
                            };
                            this->sharedData_->cv.wait(lock, [this, syncGateIndex] {
                                return (
                                    this->sharedData_->shutdownFlag ||
                                    this->sharedData_->syncGateUnlockedFlags[syncGateIndex]
                                );
                            });
                            if (this->sharedData_->shutdownFlag)
                                shutdownFlag = true;
                            else
                                this->sharedData_->syncGateUnlockedFlags[syncGateIndex] = false;
                        }
                        if (shutdownFlag) {
                            if (this->callback_)
                                this->callback_->onLineEnd(ctx);
                            return;
                        }
                        if (this->callback_)
                            this->callback_->onSyncGateUnlocked(ctx, syncGate->get());

                        syncGateIndex++;
                    }
                }
            }
        }
        catch (...) {
            if (this->callback_) {
                this->callback_->onFatalError(ctx, std::current_exception());
                this->callback_->onLineEnd(ctx);
            }
        }
    }};
}

template<typename T>
inline void ExecutionLine<T>::destroy() noexcept {
    if (this->thread_.joinable()) {
        {
            std::unique_lock<std::mutex> lock{this->sharedData_->mutex};
            this->sharedData_->shutdownFlag = true;
            this->sharedData_->cv.notify_one();
        }
        this->thread_.join();
        this->sharedData_ = nullptr;

        std::size_t syncGateIndex = 0;

        for (auto& e : this->lineElements_) {
            if (auto* const syncGate = std::get_if<std::reference_wrapper<SyncGate>>(&e)) {
                syncGate->get().removeLockDependency(
                    this->syncGateLockDependencyIds_[syncGateIndex]
                );
                syncGateIndex++;
            }
        }
        this->syncGateLockDependencyIds_.clear();
        this->lineElements_.clear();
        this->callback_ = nullptr;
        this->destroyed_ = true;
    }
}

} /* namespace pidux */
