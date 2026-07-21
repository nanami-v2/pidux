
#pragma once
#include <thread>
#include "./ExecutionLineEventQueue.h"

namespace sample {

class ExecutionLineEventLogger final: public ExecutionLineEventQueue::Callback {
public:
private:
    std::thread threaed_;

};

} /* namespace sample */