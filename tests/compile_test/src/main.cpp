
#include <atomic>
#include <mutex>
#include <bitset>
#include <iostream>
#include <csignal>
#include <pidux.h>

namespace {
    
struct Context {
    int randBase{500};
};

class TestExecutionUnit final : public pidux::ExecutionUnit {
public:
    explicit TestExecutionUnit(char const* unitName):
        unitName_{unitName}
    {}
    void run(void* ctx) override {
        auto const randBase = static_cast<Context*>(ctx)->randBase;
        auto const sleepTime = std::chrono::milliseconds{std::rand() % randBase};

        std::cout << unitName_.c_str() << ",";
        std::this_thread::sleep_for(sleepTime);
    }
private:
    std::string unitName_;
};

class TestExecutionLineCallback final : public pidux::ExecutionLineCallback {
public:
    explicit TestExecutionLineCallback(char const* lineName):
        lineName_{lineName}
    {}
    void onLineStart(void* ctx) override {
        std::cout
            << "line '" << this->lineName_ << "' start" << std::endl;
    }
    void onLineEnd(void* ctx) noexcept override {
        std::cout
            << "line '" << this->lineName_ << "' end" << std::endl;
    }
    void onFatalError(void* ctx, std::exception_ptr error) noexcept {
        try {
            std::rethrow_exception(error);
        }
        catch (std::exception const& e) {
            std::cerr
                << "[FATAL] " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr
                << "[FATAL] unknown error" << std::endl;
        }
    }
    void onGateUnlocked(void* ctx, pidux::Gate& gate) {}
    void onExecutionUnitStart(void* ctx, pidux::ExecutionUnit& executionUnit) {
        std::cout
            << "ExecutionUnit...'"
            << typeid(executionUnit).name()
            << "' start" << std::endl;
    }
    void onExecutionUnitEnd(void* ctx, pidux::ExecutionUnit& executionUnit) {
        std::cout
            << "ExecutionUnit...'"
            << typeid(executionUnit).name()
            << "' end" << std::endl;
    }
    void onExecutionUnitError(
        void* ctx,
        pidux::ExecutionUnit& executionUnit,
        std::exception_ptr executionUnitError
    ) noexcept {
        try {
            std::rethrow_exception(executionUnitError);
        }
        catch (std::exception const& e) {
            std::cerr
                << "[ERROR] '"
                << typeid(executionUnit).name()
                << "' : " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr
                << "[ERROR] '"
                << typeid(executionUnit).name()
                << "' : unknown error" << std::endl;
        }
    }
private:
    std::string lineName_;
};

} /* namespace */

int main() {
    /*
        Line1: ---A---|---B---
        Line2: -------|---C---
    */
    /* ExcutionUnit: class version */
    auto unitA = TestExecutionUnit{"A"};
    auto unitB = TestExecutionUnit{"B"};
    /* ExcutionUnit: lambda version */
    auto unitC = pidux::createExecutionUnit([](auto* ctx) {
        auto const randBase = static_cast<Context*>(ctx)->randBase;
        auto const sleepTime = std::chrono::milliseconds{std::rand() % randBase};

        std::cout << "C,";
        std::this_thread::sleep_for(sleepTime);
    });
    pidux::Gate syncGate{};

    auto line1CreationParams = pidux::ExecutionLine::CreationParams{
        {unitA, syncGate, unitB}
    };
    auto line2CreationParams = pidux::ExecutionLine::CreationParams{
        {syncGate, unitC}
    };
    pidux::ExecutionLine line1{line1CreationParams};
    pidux::ExecutionLine line2{line2CreationParams};

    Context ctx{};
    TestExecutionLineCallback line1Callback{"Line1"};
    TestExecutionLineCallback line2Callback{"Line2"};

    line1.start(&ctx, line1Callback);
    line2.start(&ctx, line2Callback);

    std::cin.get();

    return 0;
}