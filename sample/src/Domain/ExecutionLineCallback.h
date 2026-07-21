
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
    {}
    void onLineStart([[maybeunused]] AppContext& ctx) override {
        /* do noting */
    }
    void onLineEnd([[maybeunused]] AppContext& ctx) noexcept override {
        /* do noting */
    }
    void onFatalError(AppContext& ctx, std::exception_ptr error) noexcept override {
    }
    void onSyncGateUnlocked(
        [[maybeunused]] AppContext& ctx,
        [[maybeunused]] pidux::SyncGate& syncGate
    ) override {
        /* do nothing */
    }
    void onExecutionUnitStart(
        [[maybeunused]] AppContext& ctx,
        [[maybeunused]] pidux::ExecutionUnit<AppContext>& executionUnit
    ) override {
        this->currentUnitStartTime_ = std::chrono::system_clock::now();
    }
    void onExecutionUnitEnd(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit
    ) override {
        this->currentUnitEndTime_ = std::chrono::system_clock::now();

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
    }
    void onExecutionUnitError(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit,
        std::exception_ptr executionUnitError
    ) noexcept override {
    }
private:
    unsigned int lineNo_;
    std::chrono::system_clock::time_point currentUnitStartTime_;
    std::chrono::system_clock::time_point currentUnitEndTime_;
};

} /* namespace sample */