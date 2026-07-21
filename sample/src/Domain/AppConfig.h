
#pragma once
#include <chrono>

namespace sample {

struct AppConfig {
    std::chrono::milliseconds randSleepTimeMin;
    std::chrono::milliseconds randSleepTimeMax;    
};

} /* namespace sample */