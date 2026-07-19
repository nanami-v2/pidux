
#pragma once
#include <pidux.h>
#include <random>
#include "./AppContext.h"

class ExecutionUnit final: public pidux::ExecutionUnit<AppContext> {
public:
    void run(AppContext& ctx) {
        this->refThreadLocalRandEngine_.
    }
private:
    std::default_random_engine& refThreadLocalRandEngine_;
};