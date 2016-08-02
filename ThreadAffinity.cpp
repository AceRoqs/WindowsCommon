#include "PreCompile.h"
#include "ThreadAffinity.h"
#include "CheckHR.h"

namespace WindowsCommon
{

void lock_thread_to_first_processor()
{
    const HANDLE process = GetCurrentProcess();

    DWORD_PTR process_mask;
    DWORD_PTR system_mask;
    CHECK_BOOL_LAST_ERROR(GetProcessAffinityMask(process, &process_mask, &system_mask));

    if(process_mask != 0 && system_mask != 0)
    {
        const DWORD_PTR thread_mask = process_mask & ~(process_mask - 1);

        const HANDLE thread = GetCurrentThread();
        CHECK_BOOL_LAST_ERROR(SetThreadAffinityMask(thread, thread_mask) != 0);
    }
}

constexpr DWORD MS_VC_exception = 0x406D1388;
constexpr DWORD_PTR MS_VC_debug_set_name = 0x1000;

#pragma pack(push, 8)
struct Thread_name_info
{
    DWORD_PTR type;    // Must be 0x1000.
    PCSTR thread_name; // Pointer to name.
    DWORD thread_id;   // Thread ID (-1 = current thread).
    DWORD flags;       // Reserved for future use, must be zero.
};
#pragma pack(pop)

static __checkReturn LONG thread_name_exception_handler(_In_ PEXCEPTION_POINTERS exception_pointers) noexcept
{
    LONG policy;

    if(MS_VC_exception == exception_pointers->ExceptionRecord->ExceptionCode)
    {
        policy = EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        policy = EXCEPTION_CONTINUE_SEARCH;
    }

    return policy;
}

// thread_name must be ASCII.
static void debug_set_thread_name(DWORD thread_id, _In_ PCSTR thread_name) noexcept
{
    (void)thread_name;
#ifndef NDEBUG
    // Set up a Visual Studio/WinDBG specific structure to pass as
    // the exception data.
    // http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    Thread_name_info thread_info;
    thread_info.type        = MS_VC_debug_set_name;
    thread_info.thread_name = thread_name;
    thread_info.thread_id   = thread_id;
    thread_info.flags       = 0;

    __try
    {
        RaiseException(MS_VC_exception,
                       0,
                       (sizeof(thread_info) + sizeof(ULONG_PTR) - 1) / sizeof(ULONG_PTR),
                       reinterpret_cast<ULONG_PTR*>(&thread_info));
    }
    __except(thread_name_exception_handler(GetExceptionInformation()))
    {
        __noop;
    }
#endif
}

// thread_name must be ASCII.
_Use_decl_annotations_
void debug_set_current_thread_name(PCSTR thread_name) noexcept
{
    debug_set_thread_name(0xffffffff, thread_name);
}

}

