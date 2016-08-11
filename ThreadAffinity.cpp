#include "PreCompile.h"
#include "ThreadAffinity.h"
#include "CheckHR.h"
#include <PortableRuntime/Tracing.h>

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

#if 0   // Need a more recent SDK to test this.
#include <processthreadsapi.h>
void dprintf_system_cpu_set_information()
{
    ULONG length;
    if(!GetSystemCpuSetInformation(nullptr, 0, &length, GetCurrentProcess(), 0))
    {
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(length);
            if(GetSystemCpuSetInformation(buffer, length, &length, GetCurrentProcess(), 0))
            {
                ULONG offset = 0;
                while(offset < length)
                {
                    SYSTEM_CPU_SET_INFORMATION* info = reinterpret_cast<SYSTEM_CPU_SET_INFORMATION*>(buffer.get() + offset);
                    if(info->Type == CpuSetInformation)
                    {
                        PortableRuntime::dprintf("Id: %u, Group: %u, Index: %u, Core Index: %u, LastLevelCacheIndex: %u\n",
                                                 info->Id,
                                                 info->Group,
                                                 info->LogicalProcessorIndex,
                                                 info->CoreIndex,
                                                 info->LastLevelCacheIndex);
                    }

                    offset += info->Size;
                }
            }
        }
    }
}
#endif

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

