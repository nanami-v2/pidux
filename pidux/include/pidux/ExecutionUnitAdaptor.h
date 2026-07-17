
#pragma once
#include "./ExecutionUnit.h"


namespace pidux {

template<typename F>
class ExecutionUnitAdaptor final : public ExecutionUnit {
public:
    explicit ExecutionUnitAdaptor(F f): f_{f}
    {}
    ExecutionUnitAdaptor(ExecutionUnitAdaptor const&) = default;
    ExecutionUnitAdaptor(ExecutionUnitAdaptor&&) noexcept = default;
    ~ExecutionUnitAdaptor() noexcept override = default;

    ExecutionUnitAdaptor& operator=(ExecutionUnitAdaptor const&) = default;
    ExecutionUnitAdaptor& operator=(ExecutionUnitAdaptor&&) noexcept = default;

    void run(void* ctx) override {
        this->f_(ctx);
    }
private:
    F f_;
};

template<typename F>
ExecutionUnitAdaptor<F> createExecutionUnit(F f) {
    static_assert(
        std::is_invocable_v<F, void*>,
        "Error: lambda must be callable with an void* argument"
    );
    return ExecutionUnitAdaptor<F>(f);
}

} /* namespace pidux */
