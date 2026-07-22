
#include <iostream>
#include <format>
#include <pidux.h>
#include <boost/circular_buffer.hpp>

#include "domain/AppContext.h"
#include "domain/ExecutionUnit.h"
#include "domain/ExecutionLineCallback.h"


int main() {
    sample::AppContext appContext;
    
    appContext.randSleepTimeMin = std::chrono::milliseconds{50};
    appContext.randSleepTimeMax = std::chrono::milliseconds{500};
    appContext.randEngine.seed(std::random_device{}());
    /*
        Line1: ---A---|---B---C--|-------
        Line2: -------|---H--------------
        Line3: ---O---|----------|---P---
    */
    pidux::SyncGate       syncGate1;
    pidux::SyncGate       syncGate2;
    sample::ExecutionUnit executionUnitA{'A'};
    sample::ExecutionUnit executionUnitB{'B'};
    sample::ExecutionUnit executionUnitC{'C'};
    sample::ExecutionUnit executionUnitH{'H'};
    sample::ExecutionUnit executionUnitO{'O'};
    sample::ExecutionUnit executionUnitP{'P'};

    sample::ExecutionLineCallback executionLine1Callback{1};
    sample::ExecutionLineCallback executionLine2Callback{2};
    sample::ExecutionLineCallback executionLine3Callback{3};

    pidux::ExecutionLine<sample::AppContext> executionLine1{
        {
            executionUnitA,
            syncGate1,
            executionUnitB,
            executionUnitC,
            syncGate2,
        },
        &executionLine1Callback
    };
    pidux::ExecutionLine<sample::AppContext> executionLine2{
        {
            syncGate1,
            executionUnitH,
        },
        &executionLine2Callback
    };
    pidux::ExecutionLine<sample::AppContext> executionLine3{
        {
            executionUnitO,
            syncGate1,
            syncGate2,
            executionUnitP
        },
        &executionLine3Callback
    };

    boost::container::static_vector<sample::ExecutionLineEvent, 10> eventBuffer;
    boost::circular_buffer<char> progressBuffer1{80}; /* capacity := 80 */
    boost::circular_buffer<char> progressBuffer2{80}; /* capacity := 80 */
    boost::circular_buffer<char> progressBuffer3{80}; /* capacity := 80 */
    boost::circular_buffer<std::string> logBuffer{20};

    executionLine1.start(appContext);
    executionLine2.start(appContext);
    executionLine3.start(appContext);

    while (true) {
        /* read event */
        {
            std::unique_lock<std::mutex> lock{
                appContext.eventQueueMutex
            };
            appContext.eventQueueCv.wait(lock, [&] {
                return !appContext.eventQueue.empty();
            });
            while (!appContext.eventQueue.empty()) {
                if (eventBuffer.size() == eventBuffer.capacity())
                    break;

                eventBuffer.push_back(appContext.eventQueue.back());
                appContext.eventQueue.pop();
            }
        }
        for (auto const& event : eventBuffer) {
            if (auto* const unitExecutedEvent = std::get_if<sample::ExecutionLineEventType::UnitExecuted>(&event)) {
                switch (unitExecutedEvent->lineNo) {
                    case 1:
                        progressBuffer1.push_back(unitExecutedEvent->unitId);
                        progressBuffer2.push_back('-');
                        progressBuffer3.push_back('-');
                        break;
                    case 2:
                        progressBuffer1.push_back('-');
                        progressBuffer2.push_back(unitExecutedEvent->unitId);
                        progressBuffer3.push_back('-');
                        break;
                    case 3:
                        progressBuffer1.push_back('-');
                        progressBuffer2.push_back('-');
                        progressBuffer3.push_back(unitExecutedEvent->unitId);
                        break;
                }
            }
        }
        std::cout << "\033[2J\033[H" << std::flush;

        std::cout << "Line1: ---A---|---B---C--|-------\n";
        std::cout << "Line2: -------|---H--------------\n";
        std::cout << "Line3: ---O---|----------|---P---\n";
        std::cout << "3 execution line (3 thread) exists. \n";
        std::cout << "\n";

        std::cout << "Line1: ";
        for (auto const c : progressBuffer1)
            std::cout << c;
        std::cout << "\n";

        std::cout << "Line2: ";
        for (auto const c : progressBuffer2)
            std::cout << c;
        std::cout << "\n";

        std::cout << "Line3: ";
        for (auto const c : progressBuffer3)
            std::cout << c;
        std::cout << "\n";
        std::cout << "\n";

        for (auto const& event : eventBuffer) {
            if (auto* const unitExecutedEvent = std::get_if<sample::ExecutionLineEventType::UnitExecuted>(&event)) {
                logBuffer.push_back(
                    std::format(
                        "line = {}, unit = {}, processingTime = {}",
                        unitExecutedEvent->lineNo,
                        unitExecutedEvent->unitId,
                        unitExecutedEvent->processingTime
                    )
                );
            }
        }
        for (auto const& log : logBuffer)
            std::cout << log << "\n";
        std::cout << std::flush;
        eventBuffer.clear();
    }
    return 0;
}