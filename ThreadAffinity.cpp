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

}
