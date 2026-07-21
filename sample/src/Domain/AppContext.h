
#pragma once
#include <random>
#include <vector>
#include <boost/circular_buffer.hpp>

#include "./AppConfig.h"
#include "./ExecutionLineEventQueue.h"

namespace sample {

struct AppContext {
    struct alignas(std::hardware_destructive_interference_size) RandEngineBucket {
        std::default_random_engine engine;
    };
    AppConfig appConfig;
    ExecutionLineEventQueue eventQueue;
    std::vector<RandEngineBucket> randEngineBuckets;
};

} /* namespace sample */