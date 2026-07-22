
#include <iostream>
#include <format>
#include <pidux.h>
#include "domain/AppContext.h"
#include "domain/ExecutionUnit.h"
#include "domain/ExecutionLineCallback.h"


int main() {
    sample::AppContext appContext;
    
    appContext.randSleepTimeMin = std::chrono::milliseconds{100};
    appContext.randSleepTimeMax = std::chrono::milliseconds{900};
    appContext.randEngine.seed(std::random_device{}());

    /*
        Line1: ---A---|---B---C--|-------
        Line2: -------|---D------⤵-------
        Line3: ---E---|----------|---F---

        3 execution line (3 thread) exists.
    */
    pidux::SyncGate       syncGate1;
    pidux::SyncGate       syncGate2;
    sample::ExecutionUnit executionUnitA{1};
    sample::ExecutionUnit executionUnitB{2};
    sample::ExecutionUnit executionUnitC{3};
    sample::ExecutionUnit executionUnitD{4};
    sample::ExecutionUnit executionUnitE{5};
    sample::ExecutionUnit executionUnitF{6};

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
            executionUnitD,
        },
        &executionLine2Callback
    };
    pidux::ExecutionLine<sample::AppContext> executionLine3{
        {
            executionUnitE,
            syncGate1,
            syncGate2,
            executionUnitF
        },
        &executionLine3Callback
    };

    boost::container::static_vector<sample::ExecutionLineEvent, 10> eventBuffer;

    executionLine1.start(appContext);
    executionLine2.start(appContext);
    executionLine3.start(appContext);

    while (true) {
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
            if (auto* const unitExecuted = std::get_if<sample::ExecutionLineEventType::UnitExecuted>(&event)) {
                std::cout << std::format(
                    "lineNo = {}, unitId = {}, processingTime = {}",
                    unitExecuted->lineNo,
                    unitExecuted->unitId,
                    unitExecuted->processingTime
                ) << std::endl;
            }
        }
        eventBuffer.clear();
    }
    return 0;
}