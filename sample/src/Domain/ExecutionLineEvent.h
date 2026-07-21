
#pragma once
#include <variant>
#include <chrono>

namespace ExecutionLineEventType {
    struct UnitExecuted {
        unsigned int lineNo;
        unsigned int unitId;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        std::chrono::milliseconds processingTime;
    };
};

using ExecutionLineEvent = std::variant<
    ExecutionLineEventType::UnitExecuted
>;