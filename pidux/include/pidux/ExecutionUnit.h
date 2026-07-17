// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once
#include <type_traits>

namespace pidux {

class ExecutionUnit {
public:
    virtual ~ExecutionUnit() noexcept = default;
    virtual void run(void* ctx) = 0;
};

} /* namespace pidux */