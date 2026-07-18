// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include "./ExecutionUnit.h"

namespace pidux {

template<typename T, typename F>
class ExecutionUnitAdaptor final : public ExecutionUnit<T> {
public:
    explicit ExecutionUnitAdaptor(F f): f_{f}
    {}
    ExecutionUnitAdaptor(ExecutionUnitAdaptor const&) = default;
    ExecutionUnitAdaptor(ExecutionUnitAdaptor&&) noexcept = default;
    ~ExecutionUnitAdaptor() noexcept override = default;

    ExecutionUnitAdaptor& operator=(ExecutionUnitAdaptor const&) = default;
    ExecutionUnitAdaptor& operator=(ExecutionUnitAdaptor&&) noexcept = default;

    void run(T& ctx) override {
        this->f_(ctx);
    }
private:
    F f_;
};

template<typename T, typename F>
ExecutionUnitAdaptor<T, F> createExecutionUnit(F f) {
    static_assert(
        std::is_invocable_v<F, T&>,
        "Error: lambda must be callable with T&"
    );
    return ExecutionUnitAdaptor<T, F>{f};
}

} /* namespace pidux */
