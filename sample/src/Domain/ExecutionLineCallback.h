
#pragma once
#include <optional>
#include <pidux.h>
#include "./AppContext.h"
#include "./ExecutionUnit.h"
#include "./ExecutionLineEvent.h"

class ExecutionLineCallback final: public pidux::ExecutionLineCallback<AppContext> {
public:
    explicit ExecutionLineCallback(unsigned int lineNo):
        lineNo_{lineNo}
    {}

    void onLineStart(
        AppContext& ctx
    ) override {
        /* do noting */
    }
    void onLineEnd(
        AppContext& ctx
    ) noexcept override {
        /* do noting */
    }
    void onFatalError(
        AppContext& ctx,
        std::exception_ptr error
    ) noexcept override {
    }
    void onSyncGateUnlocked(
        [[maybeunused]] AppContext& ctx,
        [[maybeunused]] pidux::SyncGate& syncGate
    ) override {
        /* do nothing */
    }
    void onExecutionUnitStart(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit
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
                this->currentUnitStartTime_.value(),
                this->currentUnitEndTime_.value(),
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    this->currentUnitEndTime_.value() - 
                    this->currentUnitStartTime_.value()
                )
            }
        );
        this->currentUnitStartTime_ = std::nullopt;
        this->currentUnitEndTime_ = std::nullopt;
    }
    void onExecutionUnitError(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit,
        std::exception_ptr executionUnitError
    ) noexcept override {
    }
private:
    unsigned int lineNo_;
    std::optional<std::chrono::system_clock::time_point> currentUnitStartTime_{std::nullopt};
    std::optional<std::chrono::system_clock::time_point> currentUnitEndTime_{std::nullopt};
};