
#pragma once
#include <pidux.h>
#include "./AppContext.h"

namespace sample {

class ExecutionUnit final: public pidux::ExecutionUnit<AppContext> {
public:
    explicit ExecutionUnit(unsigned int unitId):
        unitId_{unitId}
    {}
    void run(AppContext& ctx) override {
        if (this->uniformDistributionUninitialized_) {
            this->uniformDistributionUninitialized_ = false;
            this->uniformDistribution_ = std::uniform_int_distribution<std::chrono::milliseconds::rep>{
                ctx.randSleepTimeMin.count(),
                ctx.randSleepTimeMax.count()
            };
        }
        std::chrono::milliseconds sleepTime;
        {
            std::unique_lock<std::mutex> lock{ctx.randEngineMutex};
            sleepTime = std::chrono::milliseconds{
                this->uniformDistribution_(ctx.randEngine)
            };
        }
        std::this_thread::sleep_for(sleepTime);
    }
    unsigned int unitId() const noexcept {
        return this->unitId_;
    }
private:
    unsigned int unitId_;
    bool uniformDistributionUninitialized_{true};
    std::uniform_int_distribution<std::chrono::milliseconds::rep> uniformDistribution_;
};

} /* namespace sample */