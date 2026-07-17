// SPDX-License-Identifier: MIT
// Copyright (c) 2026 nanami-v2
#pragma once

namespace pidux {

class SyncGateCallback {
public:
    virtual ~SyncGateCallback() noexcept = default;
    virtual void onLocked() = 0;
    virtual void onUnlocked() = 0;
};

} /* namespace pidux */