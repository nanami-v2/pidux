
#pragma once
#include <optional>
#include <pidux.h>
#include "./AppContext.h"
#include "./ExecutionUnit.h"

class ExecutionLineCallback final: public pidux::ExecutionLineCallback<AppContext> {
public:
    void onLineStart(
        AppContext& ctx
    ) override;
    void onLineEnd(
        AppContext& ctx
    ) noexcept;
    void onFatalError(
        AppContext& ctx,
        std::exception_ptr error
    ) noexcept;
    void onSyncGateUnlocked(
        AppContext& ctx,
        pidux::SyncGate& syncGate
    ) override;
    void onExecutionUnitStart(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit
    ) override {
        auto& unit = static_cast<ExecutionUnit>()
    }
    void onExecutionUnitEnd(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit
    ) override;
    void onExecutionUnitError(
        AppContext& ctx,
        pidux::ExecutionUnit<AppContext>& executionUnit,
        std::exception_ptr executionUnitError
    ) noexcept override;
private:
    std::optional<unsigned int> latestUnitId{std::nullopt};
    std::optional<std::chrono::system_clock::time_point> latestUnitStartTime{std::nullopt};
    std::optional<std::chrono::system_clock::time_point> latestUnitEndTime{std::nullopt};
};