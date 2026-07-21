
#pragma once
#include <pidux.h>
#include "./AppContext.h"

class ExecutionUnit final: public pidux::ExecutionUnit<AppContext> {
public:
    explicit ExecutionUnit(unsigned int lineNo, unsigned int unitId):
        lineNo_{lineNo},
        unitId_{unitId}
    {}
    void run(AppContext& ctx) override {
        if (this->uniformDistributionUninitialized_) {
            this->uniformDistributionUninitialized_ = false;
            this->uniformDistribution_ = std::uniform_int_distribution<std::chrono::milliseconds::rep>{
                ctx.appConfig.randSleepTimeMin.count(),
                ctx.appConfig.randSleepTimeMax.count()
            };
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds{
                this->uniformDistribution_(ctx.randEngineBuckets[this->lineNo_].engine)
            }
        );
    }
    unsigned int unitId() const noexcept {
        return this->unitId_;
    }
private:
    unsigned int lineNo_;
    unsigned int unitId_;
    bool uniformDistributionUninitialized_{true};
    std::uniform_int_distribution<std::chrono::milliseconds::rep> uniformDistribution_;
};