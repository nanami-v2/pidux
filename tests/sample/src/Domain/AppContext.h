
#pragma once
#include <random>
#include <boost/circular_buffer.hpp>
#include "./ExecutionLineEventQueue.h"

struct AppContext {
    struct Config {
        std::chrono::milliseconds randSleepTimeMin;
        std::chrono::milliseconds randSleepTimeMax;    
    };
    struct ThreadBucket {
        std::default_random_engine randEngine;
        //boost::circular_buffer<
    };

    Config config;
    ExecutionLineEventQueue eventQueue;
};