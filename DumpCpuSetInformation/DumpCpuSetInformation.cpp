#include "PreCompile.h"
#include <WindowsCommon/ScopedWindowsTypes.h>
#include <WindowsCommon/ThreadAffinity.h>
#include <PortableRuntime/Tracing.h>
#include <PortableRuntime/CheckException.h>
#include <PortableRuntime/Unicode.h>

namespace DumpCpuSetInformation
{

void stdout_dprintf(_In_z_ const char* format) noexcept
{
    std::wprintf(PortableRuntime::utf16_from_utf8(format).c_str());
}

}

int wmain(int argc, _In_reads_(argc) wchar_t** argv)
{
    (void)argc;     // Unreferenced parameter.
    (void)argv;

    // ERRORLEVEL zero is the success code.
    int error_level = 0;

    // Set outside the try block so error messages use the proper code page.
    // This class does not throw.
    WindowsCommon::UTF8_console_code_page code_page;

    try
    {
        PortableRuntime::set_dprintf(DumpCpuSetInformation::stdout_dprintf);

        // Set wprintf output to UTF-8 in Windows console.
        // CHECK_EXCEPTION ensures against the case that the CRT invalid parameter handler
        // routine is set by a global constructor.
        CHECK_EXCEPTION(_setmode(_fileno(stdout), _O_U8TEXT) != -1, u8"Failed to set UTF-8 output mode.");
        CHECK_EXCEPTION(_setmode(_fileno(stderr), _O_U8TEXT) != -1, u8"Failed to set UTF-8 output mode.");

        WindowsCommon::dprintf_system_cpu_set_information();
    }
    catch(const std::exception& ex)
    {
        std::fwprintf(stderr, L"\n%s\n", PortableRuntime::utf16_from_utf8(ex.what()).c_str());
        error_level = 1;
    }

    return error_level;
}

