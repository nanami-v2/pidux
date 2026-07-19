
#pragma once
#include <variant>

namespace ExecutionLineEventType {
    struct UnitExecuted {
        int lineNo;
        int unitId;
    };
};

using ExecutionLineEvent = std::variant<
    ExecutionLineEventType::UnitExecuted
>;