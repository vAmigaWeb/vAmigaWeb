// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Wakeable.h"

namespace vamiga {

void
Wakeable::waitForWakeUp(util::Time timeout)
{
    auto now = std::chrono::system_clock::now();
    auto delay = std::chrono::nanoseconds(timeout.asNanoseconds());

    std::unique_lock<std::mutex> lock(condMutex);
    condVar.wait_until(lock, now + delay, [this]{ return ready; });
    ready = false;
}

void
Wakeable::wakeUp()
{
    {
        std::lock_guard<std::mutex> lock(condMutex);
        ready = true;
    }
    condVar.notify_one();
}

}
