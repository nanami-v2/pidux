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

#include "./Gate.h"
#include "./ExecutionUnit.h"
#include "./ExecutionLineCallback.h"

namespace pidux {

#ifndef PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT
#define PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT 64
#endif

class ExecutionLine {
public:
    using Element = std::variant<
        std::reference_wrapper<ExecutionUnit>,
        std::reference_wrapper<Gate>
    >;
    static constexpr std::size_t ElementMaxCount = PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT;

    struct CreationParams {
        boost::container::static_vector<
            ExecutionLine::Element,
            ExecutionLine::ElementMaxCount
        > lineElements;
        ExecutionLineCallback* callback{nullptr};
    };
public:
    explicit ExecutionLine(CreationParams const& params);
    ExecutionLine(ExecutionLine const&) = delete;
    ExecutionLine(ExecutionLine&&) noexcept = delete;
    ~ExecutionLine() noexcept;

    ExecutionLine& operator=(ExecutionLine const&) = delete;
    ExecutionLine& operator=(ExecutionLine&&) noexcept = delete;

    void start(void* ctx);
    void destroy() noexcept;

private:
    struct SharedData {
        bool                         shutdownFlag{false};
        std::bitset<ElementMaxCount> gateUnlockedFlags{};
    };
    std::thread             thread_;
    SharedData              sharedData_;
    std::condition_variable sharedDataCv_;
    std::mutex              sharedDataMutex_;

    class GateEventHandler final : public GateCallback {
    public:
        explicit GateEventHandler(
            std::size_t              gateIndex,
            SharedData&              sharedData,
            std::condition_variable& sharedDataCv,
            std::mutex&              sharedDataMutex
        ):
            gateIndex{gateIndex},
            sharedData{sharedData},
            sharedDataCv{sharedDataCv},
            sharedDataMutex{sharedDataMutex}
        {}
        void onLocked() override {};
        void onUnlocked() override {
            std::unique_lock<std::mutex> lock{sharedDataMutex};

            sharedData.gateUnlockedFlags[gateIndex] = true;
            sharedDataCv.notify_one();   
        }
    private:
        std::size_t              gateIndex;
        SharedData&              sharedData;
        std::condition_variable& sharedDataCv;
        std::mutex&              sharedDataMutex;
    };
    
    boost::container::static_vector<GateEventHandler, ElementMaxCount> gateEventHandlers_;
    boost::container::static_vector<Element, ElementMaxCount> lineElements_;
    ExecutionLineCallback* callback_{nullptr};
};

/*-----------------------------------------------------------------------------
    Implementation
-----------------------------------------------------------------------------*/

inline ExecutionLine::ExecutionLine(CreationParams const& params):
    lineElements_{params.lineElements},
    callback_{params.callback}
{
    std::size_t gateIndex = 0;

    for (auto& e : params.lineElements) {
        if (auto* const refGate = std::get_if<std::reference_wrapper<Gate>>(&e)) {
            this->gateEventHandlers_.push_back(
                GateEventHandler{
                    gateIndex,
                    this->sharedData_,
                    this->sharedDataCv_,
                    this->sharedDataMutex_
                }
            );
            refGate->get().addLockDependency(
                this->gateEventHandlers_.back()
            );
            gateIndex++;
        }
    }
}

inline ExecutionLine::~ExecutionLine() noexcept {
    this->destroy();
}

inline void ExecutionLine::start(void* ctx) {
    this->thread_ = std::thread{[this, ctx]() {
        try {
            if (this->callback_)
                this->callback_->onLineStart(ctx);

            while (true) {
                std::size_t gateCursor   = 0;
                bool        shutdownFlag = false;

                for (auto& e : this->lineElements_) {
                    auto* const refExecutionUnit = std::get_if<std::reference_wrapper<ExecutionUnit>>(&e);
                    auto* const refGate          = std::get_if<std::reference_wrapper<Gate>>(&e);

                    if (refExecutionUnit) {
                        {
                            std::unique_lock<std::mutex> lock{this->sharedDataMutex_};
                            shutdownFlag = this->sharedData_.shutdownFlag;
                        }
                        if (shutdownFlag) {
                            if (this->callback_)
                                this->callback_->onLineEnd(ctx);

                            return;
                        }
                        try {
                            if (this->callback_)
                                this->callback_->onExecutionUnitStart(ctx, refExecutionUnit->get());

                            refExecutionUnit->get().run(ctx);

                            if (this->callback_)
                                this->callback_->onExecutionUnitEnd(ctx, refExecutionUnit->get());
                        }
                        catch (...) {
                            if (this->callback_) {
                                this->callback_->onExecutionUnitError(ctx, refExecutionUnit->get(), std::current_exception());
                                this->callback_->onLineEnd(ctx);
                            }
                            return;
                        }
                    }
                    if (refGate) {
                        refGate->get().requestUnlock();
                        {
                            std::unique_lock<std::mutex> lock{this->sharedDataMutex_};
                            this->sharedDataCv_.wait(lock, [this, gateCursor] {
                                return (
                                    this->sharedData_.shutdownFlag ||
                                    this->sharedData_.gateUnlockedFlags[gateCursor]
                                );
                            });
                            if (this->sharedData_.shutdownFlag)
                                shutdownFlag = true;
                            else
                                this->sharedData_.gateUnlockedFlags[gateCursor] = false;
                        }
                        if (shutdownFlag) {
                            if (this->callback_)
                                this->callback_->onLineEnd(ctx);

                            return;
                        }
                        if (this->callback_)
                            this->callback_->onGateUnlocked(ctx, refGate->get());

                        gateCursor++;
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

inline void ExecutionLine::destroy() noexcept {
    if (this->thread_.joinable()) {
        {
            std::unique_lock<std::mutex> lock{this->sharedDataMutex_};
            this->sharedData_.shutdownFlag = true;
            this->sharedDataCv_.notify_one();
        }
        this->thread_.join();

        std::size_t gateIndex = 0;

        for (auto& e : this->lineElements_) {
            if (auto* const refGate = std::get_if<std::reference_wrapper<Gate>>(&e)) {
                refGate->get().removeLockDependency(
                    this->gateEventHandlers_[gateIndex]
                );
                gateIndex++;
            }
        }
    }
}

} /* namespace pidux */
