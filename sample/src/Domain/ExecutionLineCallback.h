
#pragma once
#include <optional>
#include <pidux.h>
#include "./AppContext.h"
#include "./ExecutionUnit.h"
#include "./ExecutionLineEvent.h"

namespace sample {

class ExecutionLineCallback final: public pidux::ExecutionLineCallback<AppContext> {
public:
    explicit ExecutionLineCallback(unsigned int lineNo):
        lineNo_{lineNo}
    {
        assert(lineNo >= 1);
    }
    void onLineStart([[maybe_unused]] AppContext& ctx) override {
        /* do noting */
    }
    void onLineEnd([[maybe_unused]] AppContext& ctx) noexcept override {
        /* do noting */
    }
    void onCriticalError(
        [[maybe_unused]] AppContext& ctx,
        [[maybe_unused]] std::exception_ptr error
    ) noexcept override {
        /* TODO: error handling */
    }
    void onExecutionUnitStart(
        [[maybe_unused]] AppContext& ctx,
        [[maybe_unused]] pidux::ExecutionUnit<AppContext>& executionUnit
    ) override {
        this->currentUnitStartTime_ = std::chrono::system_clock::now();
    }
    void onExecutionUnitEnd(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit
    ) override {
        this->currentUnitEndTime_ = std::chrono::system_clock::now();

        std::unique_lock<std::mutex> lock{
            ctx.eventQueueMutex
        };
        ctx.eventQueue.push(
            ExecutionLineEventType::UnitExecuted{
                this->lineNo_,
                static_cast<ExecutionUnit&>(executionUnit).unitId(),
                this->currentUnitStartTime_,
                this->currentUnitEndTime_,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    this->currentUnitEndTime_ - 
                    this->currentUnitStartTime_
                )
            }
        );
        ctx.eventQueueCv.notify_one();
    }
    void onExecutionUnitError(
        [[maybe_unused]] AppContext& ctx,
        [[maybe_unused]] pidux::ExecutionUnit<AppContext>& executionUnit,
        [[maybe_unused]] std::exception_ptr executionUnitError,
        [[maybe_unused]] bool& executuinUnitRecovered
    ) noexcept override {
        /* TODO: error handling */
    }
private:
    unsigned int lineNo_;
    std::chrono::system_clock::time_point currentUnitStartTime_;
    std::chrono::system_clock::time_point currentUnitEndTime_;
};

} /* namespace sample */