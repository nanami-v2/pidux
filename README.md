<!-- omit in toc -->
# pidux

`pidux` is C++ Header-only library to easily building multi-threaded pipeline.

- [Concept](#concept)
- [Requirements](#requirements)
- [How to Use](#how-to-use)

## Concept

<dl>
  <dt>Execution Line</dt>
  <dd>
    An Execution Line represents a parallel execution path. It is implemented by a thread. An Execution Line has multiple Execution Units and is connected to Sync Gate(s). Execution Units within a line run sequentially. A Sync Gate is used for synchronization with other Execution Lines.
  </dd>
</dl>
<dl>
  <dt>Execution Unit</dt>
  <dd>
    An Execution Unit represents an executable component on an Execution Line. It is implemented as an interface class, so the user must implement this interface to perform the desired processing.
  </dd>
</dl>
<dl>
  <dt>Sync Gate</dt>
  <dd>
    A Sync Gate is a synchronization mechanism for multiple Execution Lines. When an Execution Line reaches this gate, the line requests the gate to unlock and waits for the gate to open. The Sync Gate remains locked until all connected Execution Lines reach the gate and request to unlock. Once all Execution Lines connected to the gate request to unlock, the gate opens to allow all Execution Lines to continue running subsequent Execution Units and immediately relocks.
  </dd>
</dl>

## Requirements

- `c++17` or later
- `boost/container`

## How to Use

See `sample` directory. The following code is snippet.

```c++
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

    /* Setup Execution Lines */
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
    ...
    ...
    ...
    /* Run Execution Lines */
    executionLine1.start(appContext);
    executionLine2.start(appContext);
    executionLine3.start(appContext);
```
