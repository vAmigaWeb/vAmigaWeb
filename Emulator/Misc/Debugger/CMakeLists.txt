target_include_directories(vAmigaCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(vAmigaCore PRIVATE

Debugger.cpp
OSDescriptors.cpp
OSDebugger.cpp
OSDebuggerRead.cpp
OSDebuggerDump.cpp

)
