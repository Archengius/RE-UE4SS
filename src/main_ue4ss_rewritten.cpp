#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <future>
#include <memory>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include "UE4SSProgram.hpp"

#define WIN_API_FUNCTION_NAME DllMain

using namespace RC;

// We're outside DllMain here
auto thread_dll_start(UE4SSProgram* program) -> unsigned long
{
    // Wrapper for entire program
    // Everything must be channeled through MProgram
    // One of the purposes for this is to forward any errors to main so that we can close or keep the window/console open
    // There is atleast one more purpose but I forgot what it was...
    program->init();

    if (auto e = program->get_error_object(); e->has_error())
    {
        // If the output system errored out then use printf_s as a fallback
        // Logging will only happen to the debug console but it's something at least
        if (!Output::has_internal_error())
        {
            Output::send<LogLevel::Error>(STR("Fatal Error: {}\n"), to_wstring(e->get_message()));
        }
        else
        {
            printf_s("Error: %s\n", e->get_message());
        }
    }

    return 0;
}

auto process_initialized(HMODULE moduleHandle) -> void
{
    wchar_t moduleFilenameBuffer[1024]{'\0'};
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, sizeof(moduleFilenameBuffer) / sizeof(wchar_t));

    auto program = new UE4SSProgram(moduleFilenameBuffer, {});
    if (HANDLE handle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_dll_start), (LPVOID)program, 0, nullptr); handle) { CloseHandle(handle); }
}

// We're still inside DllMain so be careful what you do here
auto dll_process_attached(HMODULE moduleHandle) -> void
{
    QueueUserAPC((PAPCFUNC)process_initialized, GetCurrentThread(), (ULONG_PTR)moduleHandle);
}

auto WIN_API_FUNCTION_NAME(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      [[maybe_unused]] LPVOID lpReserved
) -> BOOL
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            dll_process_attached(hModule);
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            UE4SSProgram::static_cleanup();
            break;
    }
    return TRUE;
}