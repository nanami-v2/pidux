#pragma once
#include <cstddef>

namespace pidux::detail {

#ifndef PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT
#define PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT 64
#endif

#ifndef PIDUX_SYNC_GATE_LOCK_DEPENDENCY_MAX_COUNT
#define PIDUX_SYNC_GATE_LOCK_DEPENDENCY_MAX_COUNT 16
#endif

static constexpr std::size_t ExecutionLineElementMaxCount = PIDUX_EXECUTION_LINE_ELEMENT_MAX_COUNT;
static constexpr std::size_t SyncGateLockDependencyMaxCount = PIDUX_SYNC_GATE_LOCK_DEPENDENCY_MAX_COUNT;

} /* namespace pidux::detail */