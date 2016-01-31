#include "PreCompile.h"
#include "CheckHR.h"    // Pick up forward declarations to ensure correctness.
#include <PortableRuntime/Tracing.h>
#include <PortableRuntime/Unicode.h>

namespace WindowsCommon
{

_Use_decl_annotations_
HRESULT_exception::HRESULT_exception(HRESULT hr, const char* file_name, int line) noexcept : Exception(file_name, line), m_hr(hr)
{
    try
    {
#ifdef _D3D9_H_
        // D3D errors should use D3D9_exception.
        assert(HRESULT_FACILITY(m_hr) != _FACD3D);
#endif

        WCHAR system_message[128];
        if(0 == FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr,
                               m_hr,
                               0,
                               system_message,
                               ARRAYSIZE(system_message),
                               nullptr))
        {
            StringCchCopyW(system_message, ARRAYSIZE(system_message), L"Unknown error.");
        }

        WCHAR wide_error_string[1024];
        StringCchPrintfW(wide_error_string, ARRAYSIZE(wide_error_string), L"Error: %08x: %s", m_hr, system_message);

        const auto error_string = PortableRuntime::utf8_from_utf16(wide_error_string);
        PortableRuntime::dprintf("!%s(%d): %s\n", file_name, line, error_string.c_str());

        // Just save off the message now, but do the full formatting in what(), to allow exception unwind to free up some resources.
        m_what = std::make_shared<std::string>(std::move(error_string));
    }
    catch(const std::bad_alloc& ex)
    {
        (void)(ex);     // Unreferenced parameter.

        PortableRuntime::dprintf("!Failed creation of exception object.\n");
    }
}

HRESULT hresult_from_last_error() noexcept
{
    const DWORD error = GetLastError();
    const HRESULT hr = HRESULT_FROM_WIN32(error);
    assert(FAILED(hr));

    return hr;
}

} // namespace WindowsCommon

