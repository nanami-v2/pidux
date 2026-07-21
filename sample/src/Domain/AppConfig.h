
#pragma once
#include <chrono>

struct AppConfig {
    std::chrono::milliseconds randSleepTimeMin;
    std::chrono::milliseconds randSleepTimeMax;    
};