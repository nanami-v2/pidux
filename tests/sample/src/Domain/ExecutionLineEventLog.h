
#pragma once
#include <mutex>
#include <queue>
#include <variant>
#include "./ExecutionLineEvent.h"

class ExecutionLineEventLog {
public:
    class Callback {
    public:
        virtual ~Callback() = default;
        virtual void onWrite(ExecutionLineEvent event) = 0;
    };    
public:
    void write(ExecutionLineEvent const& event) {
        auto const copy = event;
        {
            std::unique_lock lock{this->eventQueueMutex_};
            this->eventQueue_.push(event);
        }

    }
private:
    std::queue<ExecutionLineEvent> eventQueue_;
    std::mutex                     eventQueueMutex_;
    
};