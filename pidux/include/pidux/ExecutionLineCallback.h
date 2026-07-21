// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <exception>
#include "./ExecutionUnit.h"

namespace pidux {

template<typename T>
class ExecutionLineCallback {
public:
    virtual ~ExecutionLineCallback() noexcept = default;
    
    virtual void onLineStart(T& ctx) = 0;
    virtual void onLineEnd(T& ctx) noexcept = 0;
    virtual void onCriticalError(T& ctx, std::exception_ptr error) noexcept = 0;
    virtual void onExecutionUnitStart(T& ctx, ExecutionUnit<T>& executionUnit) = 0;
    virtual void onExecutionUnitEnd(T& ctx, ExecutionUnit<T>& executionUnit) = 0;
    virtual void onExecutionUnitError(
        T& ctx,
        ExecutionUnit<T>& executionUnit,
        std::exception_ptr executionUnitError,
        bool& executionUnitErrorRecovered
    ) noexcept = 0;
};

} /* namespace pidux */