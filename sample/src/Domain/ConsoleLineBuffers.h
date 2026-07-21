
#pragma once
#include <boost/circular_buffer.hpp>
#include <vector>

template<typename T>
class ConsoleLineBuffers {
    explicit ConsoleLineBuffers(std::size_t lineCount, std::size_t lineBufferCapacity):
        lineCount_{lineCount},
        lineBufferCapacity_{lineBufferCapacity}
    {
        for (auto i = 0; i < lineCount; ++i)
            this->lineBuffers_.push_back(
                boost::circular_buffer<T>{lineBufferCapacity}
            );
    }
    ConsoleLineBuffers(ConsoleLineBuffers const&) = default;
    ConsoleLineBuffers(ConsoleLineBuffers&&) noexcept = default;
    ~ConsoleLineBuffers() noexcept = default;

    ConsoleLineBuffers& operator=(ConsoleLineBuffers const&) = default;
    ConsoleLineBuffers& operator=(ConsoleLineBuffers&&) noexcept = default;

    std::size_t lineCount() const noexcept {
        return this->lineCount_;
    }
    std::size_t lineBufferCapacity() const noexcept {
        return this->lineBufferCapacity_;
    }
    boost::circular_buffer<T>& lineBuffer(std::size_t lineNo) noexcept {
        return this->lineBuffers_[lineNo];
    }
    boost::circular_buffer<T> const& lineBuffer(std::size_t lineNo) const noexcept {
        return this->lineBuffers_[lineNo];
    }
private:
    std::size_t lineCount_;
    std::size_t lineBufferCapacity_;
    std::vector<boost::circular_buffer<T>> lineBuffers_;
};