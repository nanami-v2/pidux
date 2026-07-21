
#pragma once
#include <mutex>
#include <queue>
#include <variant>
#include <boost/circular_buffer.hpp>
#include "./ExecutionLineEvent.h"

namespace sample {

class ExecutionLineEventQueue {
public:
    class Callback {
    public:
        virtual ~Callback() = default;
        virtual void onWrite() = 0;
    };    
public:
    ExecutionLineEventQueue() = default;
    ExecutionLineEventQueue(ExecutionLineEventQueue const&) = delete;
    ExecutionLineEventQueue(ExecutionLineEventQueue&&) noexcept = delete;
    ~ExecutionLineEventQueue() noexcept = default;

    ExecutionLineEventQueue& operator=(ExecutionLineEventQueue const&) = delete;
    ExecutionLineEventQueue& operator=(ExecutionLineEventQueue&&) noexcept = delete;

    void registerCallback(Callback& callback) noexcept {
        //this->callback_ =  &calback;
    }
    void push(ExecutionLineEvent const& event) {
        {
            std::unique_lock lock{this->queueMutex_};
            this->queue_.push(event);
        }
        //this->callback_.get().onWrite();
    }
    bool tryPop(ExecutionLineEvent& output) noexcept {
        std::unique_lock lock{this->queueMutex_};

        if (this->queue_.empty())
            return false;

        output = std::move(this->queue_.back());
        this->queue_.pop();
        return true;
    }
    bool tryBurstPop(
        ExecutionLineEvent* outputBuffer,
        std::size_t outputBufferSize,
        std::size_t& outputCount
    ) noexcept {
        std::unique_lock lock{this->queueMutex_};

        if (this->queue_.empty())
            return false;

        outputCount = std::min(this->queue_.size(), outputBufferSize);

        for (std::size_t i = 0; i < outputCount; ++i) {
            if (!this->queue_.empty()) {
                outputBuffer[i] = std::move(this->queue_.back());
                this->queue_.pop();
            }
        }
        return true;
    }
private:
    std::queue<ExecutionLineEvent> queue_;
    std::mutex queueMutex_;
    Callback* callback_{nullptr};
};

} /* namespace sample */