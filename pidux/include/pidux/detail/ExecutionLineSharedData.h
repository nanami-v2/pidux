
#pragma once
#include <bitset>
#include <mutex>
#include <thread>
#include "./Constants.h"

namespace pidux::detail {

struct ExecutionLineSharedData {
    bool shutdownFlag{false};
    std::bitset<ExecutionLineElementMaxCount> syncGateUnlockedFlags{};
    std::condition_variable cv;
    std::mutex mutex;
};

} /* namespace pidux::detail */