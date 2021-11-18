// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Thread.h"
#include "Chrono.h"
#include <iostream>

Thread::Thread()
{
    // Initialize the sync timer
    targetTime = util::Time::now();


#ifndef __EMSCRIPTEN__    
    // Start the thread and enter the main function
    thread = std::thread(&Thread::main, this);
#endif
}

Thread::~Thread()
{
#ifndef __EMSCRIPTEN__    

    // Wait until the thread has terminated
    join();
#endif
}

template <> void
Thread::execute <Thread::SyncMode::Periodic> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
}

template <> void
Thread::execute <Thread::SyncMode::Pulsed> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
    
}

template <> void
Thread::sleep <Thread::SyncMode::Periodic> ()
{
    auto now = util::Time::now();

    // Only proceed if we're not running in warp mode
    if (warpMode) return;
        
    // Check if we're running too slow...
    if (now > targetTime) {
        
        // Check if we're completely out of sync...
        if ((now - targetTime).asMilliseconds() > 200) {
                        
            warn("Emulation is way too slow: %f\n",(now - targetTime).asSeconds());

            // Restart the sync timer
            targetTime = util::Time::now();
        }
    }
    
    // Check if we're running too fast...
    if (now < targetTime) {
        
        // Check if we're completely out of sync...
        if ((targetTime - now).asMilliseconds() > 200) {
            
            warn("Emulation is way too slow: %f\n",(targetTime - now).asSeconds());

            // Restart the sync timer
            targetTime = util::Time::now();
        }
    }
        
    // Sleep for a while
    // std::cout << "Sleeping... " << targetTime.asMilliseconds() << std::endl;
    // std::cout << "Delay = " << delay.asNanoseconds() << std::endl;
    targetTime += delay;
    targetTime.sleepUntil();
}

template <> void
Thread::sleep <Thread::SyncMode::Pulsed> ()
{
    // Wait for the next pulse
    if (!warpMode) waitForWakeUp();
}

void
Thread::main()
{
    debug(RUN_DEBUG, "main()\n");
#ifndef __EMSCRIPTEN__    
    while (++loopCounter) {
           
        if (isRunning()) {
                        
            switch (mode) {
                case SyncMode::Periodic: execute<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: execute<SyncMode::Pulsed>(); break;
            }
        }
        
        if (!warpMode || isPaused()) {

            switch (mode) {
                case SyncMode::Periodic: sleep<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: sleep<SyncMode::Pulsed>(); break;
            }
        }
        
        // Are we requested to enter or exit warp mode?
        while (newWarpMode != warpMode) {
            
            AmigaComponent::warpOnOff(newWarpMode);
            warpMode = newWarpMode;
            break;
        }

        // Are we requested to enter or exit warp mode?
        while (newDebugMode != debugMode) {
            
            AmigaComponent::debugOnOff(newDebugMode);
            debugMode = newDebugMode;
            break;
        }

        // Are we requested to change state?
        while (newState != state) {
            
            if (state == EXEC_OFF && newState == EXEC_PAUSED) {
                
                AmigaComponent::powerOn();
                state = EXEC_PAUSED;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_OFF) {
                
                AmigaComponent::powerOff();
                state = EXEC_OFF;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_RUNNING) {
                
                AmigaComponent::run();
                state = EXEC_RUNNING;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_OFF) {
                
                AmigaComponent::pause();
                state = EXEC_PAUSED;
                AmigaComponent::powerOff();
                state = EXEC_OFF;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_PAUSED) {
                
                AmigaComponent::pause();
                state = EXEC_PAUSED;
                break;
            }
            
            if (newState == EXEC_HALTED) {
                
                AmigaComponent::halt();
                state = EXEC_HALTED;
                return;
            }
            
            // Invalid state transition
            fatalError;
            break;
        }
        
        // Compute the CPU load once in a while
        if (loopCounter % 32 == 0) {
            
            auto used  = loadClock.getElapsedTime().asSeconds();
            auto total = nonstopClock.getElapsedTime().asSeconds();
            
            cpuLoad = used / total;
            
            loadClock.restart();
            loadClock.stop();
            nonstopClock.restart();
        }
    }
#endif
}

void
Thread::setSyncDelay(util::Time newDelay)
{
    delay = newDelay;
}

void
Thread::setMode(SyncMode newMode)
{
    mode = newMode;
}

void
Thread::setWarpLock(bool value)
{
    warpLock = value;
}

void
Thread::setDebugLock(bool value)
{
    debugLock = value;
}

void
Thread::powerOn(bool blocking)
{
    debug(RUN_DEBUG, "powerOn()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (isPoweredOff()) {
        
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::powerOff(bool blocking)
{
    debug(RUN_DEBUG, "powerOff()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (!isPoweredOff()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_OFF, blocking);
    }
}

void
Thread::run(bool blocking)
{
    debug(RUN_DEBUG, "run()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    printf("**** State %s\n",ExecutionStateEnum::key(state));

    if (!isRunning()) {

        assert(isPoweredOn());

        // Throw an exception if the emulator is not ready to run
        isReady();
        
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_RUNNING, blocking);
    }

    printf("**** State %s\n",ExecutionStateEnum::key(state));
}

void
Thread::pause(bool blocking)
{
    debug(RUN_DEBUG, "pause()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    printf("**** State %s\n",ExecutionStateEnum::key(state));

    if (isRunning()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::halt(bool blocking)
{
    changeStateTo(EXEC_HALTED, blocking);
}

void
Thread::warpOn(bool blocking)
{
    if (!warpLock) changeWarpTo(true, blocking);
}

void
Thread::warpOff(bool blocking)
{
    if (!warpLock) changeWarpTo(false, blocking);
}

void
Thread::debugOn(bool blocking)
{
    if (!debugLock) changeDebugTo(true, blocking);
}

void
Thread::debugOff(bool blocking)
{
    if (!debugLock) changeDebugTo(false, blocking);
}

void
Thread::changeStateTo(ExecutionState requestedState, bool blocking)
{
    newState = requestedState;
//    if (blocking) while (state != newState) { };

    printf("**** State change %s -> %s\n",ExecutionStateEnum::key(state),ExecutionStateEnum::key(newState));
    // Are we requested to change state?
    while (newState != state) {
        
        if (state == EXEC_OFF && newState == EXEC_PAUSED) {
            
            AmigaComponent::powerOn();
            state = EXEC_PAUSED;
            break;
        }

        if (state == EXEC_PAUSED && newState == EXEC_OFF) {
            
            AmigaComponent::powerOff();
            state = EXEC_OFF;
            break;
        }

        if (state == EXEC_PAUSED && newState == EXEC_RUNNING) {
            
            AmigaComponent::run();
            state = EXEC_RUNNING;
            break;
        }

        if (state == EXEC_RUNNING && newState == EXEC_OFF) {
            
            AmigaComponent::pause();
            state = EXEC_PAUSED;
            AmigaComponent::powerOff();
            state = EXEC_OFF;
            break;
        }

        if (state == EXEC_RUNNING && newState == EXEC_PAUSED) {
            
            AmigaComponent::pause();
            state = EXEC_PAUSED;
            break;
        }
        
        if (newState == EXEC_HALTED) {
            
            AmigaComponent::halt();
            state = EXEC_HALTED;
            return;
        }
        
        // Invalid state transition
        fatalError;
        break;
    }

}

void
Thread::changeWarpTo(bool value, bool blocking)
{
    newWarpMode = value;
//    if (blocking) while (warpMode != newWarpMode) { };
    printf("**** warp change %u -> %u\n",warpMode,newWarpMode);
        
    // Are we requested to enter or exit warp mode?
    while (newWarpMode != warpMode) {
        
        AmigaComponent::warpOnOff(newWarpMode);
        warpMode = newWarpMode;
        break;
    }
}

void
Thread::changeDebugTo(bool value, bool blocking)
{
    newDebugMode = value;
//    if (blocking) while (debugMode != newDebugMode) { };
    printf("**** debug change %u -> %u\n",debugMode,newDebugMode);


    // Are we requested to enter or exit warp mode?
    while (newDebugMode != debugMode) {
        
        AmigaComponent::debugOnOff(newDebugMode);
        debugMode = newDebugMode;
        break;
    }
}

void
Thread::wakeUp()
{
    if (mode == SyncMode::Pulsed) util::Wakeable::wakeUp();
}
