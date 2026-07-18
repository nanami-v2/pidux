
#pragma once
#include <mutex>
#include <queue>
#include <variant>
#include "./ExecutionLineEvent.h"

class ExecutionLineEventQueue {
public:
    class PushEventSubsciber {
    public:
        virtual ~PushEventSubsciber() = default;
        virtual void onPushed(ExecutionLineEventQueue& queue) = 0;
    };    
public:
    void pushEvent(ExecutionLineEvent const& event) {
        std::unique_lock lock{this->eventQueueMutex_};
        this->eventQueue_.push(event);
    }
    bool tryPop(ExecutionLineEvent& outputEvent) {
        std::unique_lock lock{this->eventQueueMutex_};
        
        if (this->eventQueue_.empty())
            return false;

        outputEvent = std::move(this->eventQueue_.front());
        this->eventQueue_.pop();
        return true;
    }
private:
    std::queue<ExecutionLineEvent> eventQueue_;
    std::mutex                     eventQueueMutex_;
};