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

static const char* string_from_cache_type(PROCESSOR_CACHE_TYPE type)
{
    if(type == CacheUnified)     { return u8"CacheUnified"; }
    if(type == CacheInstruction) { return u8"CacheInstruction"; }
    if(type == CacheData)        { return u8"CacheData"; }
    if(type == CacheTrace)       { return u8"CacheTrace"; }
    return u8"CacheUnknown";
}

static void dump_system_cpu_set_information()
{
    ULONG length = 0;
    CHECK_EXCEPTION(!GetSystemCpuSetInformation(nullptr, 0, &length, GetCurrentProcess(), 0), u8"Failed to get CPU Set information.");
    CHECK_EXCEPTION(GetLastError() == ERROR_INSUFFICIENT_BUFFER, u8"Failed to get CPU Set information.");

    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(length);
    CHECK_BOOL_LAST_ERROR(GetSystemCpuSetInformation(reinterpret_cast<SYSTEM_CPU_SET_INFORMATION*>(buffer.get()), length, &length, GetCurrentProcess(), 0));

    ULONG offset = 0;
    while(offset < length)
    {
        CHECK_EXCEPTION((length - offset) >= sizeof(SYSTEM_CPU_SET_INFORMATION), u8"Failed to get CPU Set information.");

        SYSTEM_CPU_SET_INFORMATION* info = reinterpret_cast<SYSTEM_CPU_SET_INFORMATION*>(buffer.get() + offset);
        if(info->Type == CpuSetInformation)
        {
            PortableRuntime::dprintf("Id: %u, Group: %u, LPIndex: %u, CoreIndex: %u, LastLevelCacheIndex: %u, Parked: %u, Allocated: %u, AllocatedToTargetProcess: %u, RealTime: %u\n",
                                     info->CpuSet.Id,
                                     info->CpuSet.Group,
                                     info->CpuSet.LogicalProcessorIndex,
                                     info->CpuSet.CoreIndex,
                                     info->CpuSet.LastLevelCacheIndex,
                                     info->CpuSet.Parked,
                                     info->CpuSet.Allocated,
                                     info->CpuSet.AllocatedToTargetProcess,
                                     info->CpuSet.RealTime);
        }

        offset += info->Size;
    }
}

static void dump_cpu_cache_hierarchy()
{
    DWORD length = 0;
    CHECK_EXCEPTION(!GetLogicalProcessorInformation(nullptr, &length), u8"Failed to get CPU Set information.");
    CHECK_EXCEPTION(GetLastError() == ERROR_INSUFFICIENT_BUFFER, u8"Failed to get CPU Set information.");

    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(length);
    CHECK_BOOL_LAST_ERROR(GetLogicalProcessorInformation(reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(buffer.get()), &length));

    DWORD offset = 0;
    while(offset < length)
    {
        CHECK_EXCEPTION((length - offset) >= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION), u8"Failed to get CPU Set information.");

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(buffer.get() + offset);
        switch(info->Relationship)
        {
            case RelationProcessorCore:
            {
                PortableRuntime::dprintf("Mask: %p, Relationship: RelationProcessorCore, Flags: %u\n", info->ProcessorMask, info->ProcessorCore.Flags);
                break;
            }

            case RelationCache:
            {
                PortableRuntime::dprintf("Mask: %p, Relationship: RelationCache, Level: L%u, Associativity: %u, LineSize: %u, Size: %u, Type: %s\n",
                                         info->ProcessorMask,
                                         info->Cache.Level,
                                         info->Cache.Associativity,
                                         info->Cache.LineSize,
                                         info->Cache.Size,
                                         string_from_cache_type(info->Cache.Type));
                break;
            }

            case RelationProcessorPackage:
            {
                PortableRuntime::dprintf("Mask: %p, Relationship: RelationProcessorPackage\n", info->ProcessorMask);
                break;
            }
        }

        offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    }
}

void dprintf_system_cpu_set_information()
{
    PortableRuntime::dprintf("CPU Set Information:\n");
    dump_system_cpu_set_information();

    PortableRuntime::dprintf("CPU Cache Hierarchy:\n");
    dump_cpu_cache_hierarchy();
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
    (void)thread_id;
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

