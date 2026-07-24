
#pragma once
#include <random>
#include <vector>
#include <queue>
#include <mutex>
#include "./ExecutionLineEvent.h"

namespace sample {

struct AppContext {
    std::default_random_engine randEngine;
    std::mutex                 randEngineMutex;
    std::chrono::milliseconds  randSleepTimeMin;
    std::chrono::milliseconds  randSleepTimeMax;

    std::queue<ExecutionLineEvent> eventQueue;
    std::mutex                     eventQueueMutex;
    std::condition_variable        eventQueueCv;
};

} /* namespace sample */