
#pragma once
#include <bitset>
#include <mutex>
#include <thread>
#include "../Config.h"

namespace pidux::detail {

struct ExecutionLineSharedData {
    bool shutdownFlag{false};
    std::bitset<ExecutionLineElementMaxCount> syncGateUnlockedFlags{};
};

} /* namespace pidux::detail */