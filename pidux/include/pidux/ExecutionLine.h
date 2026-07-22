// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <exception>
#include <mutex>
#include <thread>
#include <variant>
#include <boost/container/static_vector.hpp>

#include "./Config.h"
#include "./SyncGate.h"
#include "./ExecutionUnit.h"
#include "./ExecutionLineCallback.h"
#include "./detail/ExecutionLineSharedData.h"
#include "./detail/ExecutionLineSyncGateCallback.h"

namespace pidux {

template<typename T>
class ExecutionLine {
public:
    using LineElement = std::variant<
        std::reference_wrapper<ExecutionUnit<T>>,
        std::reference_wrapper<SyncGate>
    >;
public:
    explicit ExecutionLine(
        boost::container::static_vector<LineElement, ExecutionLineElementMaxCount> const& lineElements,
        ExecutionLineCallback<T>* callback = nullptr
    );
    ExecutionLine(ExecutionLine const&) = delete;
    ExecutionLine(ExecutionLine&&) noexcept = delete;
    ~ExecutionLine() noexcept;

    ExecutionLine& operator=(ExecutionLine const&) = delete;
    ExecutionLine& operator=(ExecutionLine&&) noexcept = delete;

    void start(T& ctx);
    void destroy() noexcept;

private:
    std::thread thread_;
    detail::ExecutionLineSharedData sharedData_;    
    std::condition_variable         sharedDataCv_;
    std::mutex                      sharedDataMutex_;

    boost::container::static_vector<
        detail::ExecutionLineSyncGateCallback,
        ExecutionLineElementMaxCount
    > syncGateCallbacks_;
    boost::container::static_vector<
        unsigned int,
        ExecutionLineElementMaxCount
    > syncGateLockDependencyIds_;
    boost::container::static_vector<
        LineElement,
        ExecutionLineElementMaxCount
    > lineElements_;
    ExecutionLineCallback<T>* callback_;
    bool destroyed_;
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/

template<typename T>
inline ExecutionLine<T>::ExecutionLine(
    boost::container::static_vector<LineElement, ExecutionLineElementMaxCount> const& lineElements,
    ExecutionLineCallback<T>* callback
):
    lineElements_{lineElements},
    callback_{callback},
    destroyed_{false}
{
    std::size_t syncGateIndex = 0;

    for (auto& e : this->lineElements_) {
        if (auto* const syncGate = std::get_if<std::reference_wrapper<SyncGate>>(&e)) {
            this->syncGateCallbacks_.push_back(
                detail::ExecutionLineSyncGateCallback{
                    syncGateIndex,
                    this->sharedData_,
                    this->sharedDataCv_,
                    this->sharedDataMutex_
                }
            );
            this->syncGateLockDependencyIds_.push_back(
                syncGate->get().addLockDependency(
                    this->syncGateCallbacks_.back()
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
                            std::unique_lock<std::mutex> lock{
                                this->sharedDataMutex_
                            };
                            shutdownFlag = this->sharedData_.shutdownFlag;
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
                                bool executionUnitErrorRecovered = false;
                                auto executionUnitError = std::current_exception();

                                this->callback_->onExecutionUnitError(
                                    ctx,
                                    executionUnit->get(),
                                    executionUnitError,
                                    executionUnitErrorRecovered
                                );
                                if (!executionUnitErrorRecovered) {
                                    this->callback_->onCriticalError(ctx, executionUnitError);
                                    this->callback_->onLineEnd(ctx);
                                    return;
                                }
                            }
                            return;
                        }
                    }
                    if (syncGate) {
                        syncGate->get().requestUnlock(
                            this->syncGateLockDependencyIds_[syncGateIndex]
                        );
                        {
                            std::unique_lock<std::mutex> lock{
                                this->sharedDataMutex_
                            };
                            this->sharedDataCv_.wait(lock, [this, syncGateIndex] {
                                return (
                                    this->sharedData_.shutdownFlag ||
                                    this->sharedData_.syncGateUnlockedFlags[syncGateIndex]
                                );
                            });
                            if (this->sharedData_.shutdownFlag)
                                shutdownFlag = true;
                            else
                                this->sharedData_.syncGateUnlockedFlags[syncGateIndex] = false;
                        }
                        if (shutdownFlag) {
                            if (this->callback_)
                                this->callback_->onLineEnd(ctx);
                            return;
                        }
                        syncGateIndex++;
                    }
                }
            }
        }
        catch (...) {
            if (this->callback_) {
                this->callback_->onCriticalError(ctx, std::current_exception());
                this->callback_->onLineEnd(ctx);
            }
        }
    }};
}

template<typename T>
inline void ExecutionLine<T>::destroy() noexcept {
    if (this->thread_.joinable()) {
        {
            std::unique_lock<std::mutex> lock{
                this->sharedDataMutex_
            };
            this->sharedData_.shutdownFlag = true;
            this->sharedDataCv_.notify_one();
        }
        this->thread_.join();

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
        this->syncGateCallbacks_.clear();
        this->lineElements_.clear();
        this->callback_ = nullptr;
        this->destroyed_ = true;
    }
}

} /* namespace pidux */
