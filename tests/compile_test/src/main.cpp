
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
    {
        std::cout << "constructed...callback for " << this->lineName_ << std::endl;
    }
    ~TestExecutionLineCallback() noexcept {
        std::cout << "destructed...callback for " << this->lineName_ << std::endl;
    }
    void onLineStart(void* ctx) override {
        std::cout
            << "line '" << this->lineName_ << "' start" << std::endl
            << std::flush;
    }
    void onLineEnd(void* ctx) noexcept override {
        std::cout
            << "line '" << this->lineName_ << "' end" << std::endl
            << std::flush;
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
            << "' start" << std::endl
            << std::flush;
    }
    void onExecutionUnitEnd(void* ctx, pidux::ExecutionUnit& executionUnit) {
        std::cout
            << "ExecutionUnit...'"
            << typeid(executionUnit).name()
            << "' end" << std::endl
            << std::flush;
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
    {
        Context ctx{};        
        TestExecutionLineCallback line1Callback{"Line1"};
        TestExecutionLineCallback line2Callback{"Line2"};
        /*
            Line1: ---A---|---B---
            Line2: -------|---C---
        */
        pidux::ExecutionLine::CreationParams const line1CreationParams{
            {unitA, syncGate, unitB},
            &line1Callback
        };
        pidux::ExecutionLine::CreationParams const line2CreationParams{
            {syncGate, unitC},
            &line2Callback
        };
        pidux::ExecutionLine line1{line1CreationParams};
        pidux::ExecutionLine line2{line2CreationParams};

        line1.start(&ctx);
        line2.start(&ctx);

        std::this_thread::sleep_for(std::chrono::seconds{5});

        //line1.destroy();
        //line2.destroy();
    }
    std::this_thread::sleep_for(std::chrono::seconds{1});

    return 0;
}