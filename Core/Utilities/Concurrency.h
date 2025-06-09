// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Chrono.h"
#include <thread>
#include <future>
#include <latch>

namespace vamiga::util {

class Mutex
{
    std::mutex mutex;

public:

    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }
    bool tryLock() { return mutex.try_lock(); }
};

class ReentrantMutex
{
    std::recursive_mutex mutex;

public:
        
    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }
    bool tryLock() { return mutex.try_lock(); }
};

class AutoMutex
{
    ReentrantMutex &mutex;

public:

    bool active = true;

    AutoMutex(ReentrantMutex &ref) : mutex(ref) { mutex.lock(); }
    ~AutoMutex() { mutex.unlock(); }
};

}
