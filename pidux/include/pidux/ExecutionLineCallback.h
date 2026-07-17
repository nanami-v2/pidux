
#pragma once
#include <exception>
#include "./ExecutionUnit.h"

namespace pidux {

class ExecutionLineCallback {
public:
    virtual ~ExecutionLineCallback() noexcept = default;
    
    virtual void onLineStart(void* ctx) = 0;
    virtual void onLineEnd(void* ctx) noexcept = 0;
    virtual void onFatalError(void* ctx, std::exception_ptr error) noexcept = 0;
    virtual void onGateUnlocked(void* ctx, Gate& gate) = 0;
    virtual void onExecutionUnitStart(void* ctx, ExecutionUnit& executionUnit) = 0;
    virtual void onExecutionUnitEnd(void* ctx, ExecutionUnit& executionUnit) = 0;
    virtual void onExecutionUnitError(
        void* ctx,
        ExecutionUnit& executionUnit,
        std::exception_ptr executionUnitError
    ) noexcept = 0;
};

} /* namespace pidux */