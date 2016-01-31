#include "PreCompile.h"
#include "CheckHR.h"    // Pick up forward declarations to ensure correctness.
#include <PortableRuntime/Tracing.h>

namespace WindowsCommon
{

HRESULT_exception::HRESULT_exception(HRESULT hr) noexcept : m_hr(hr), m_error_string(nullptr)
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
        StringCchCopyW(system_message, ARRAYSIZE(system_message), L"Unknown");
    }

    WCHAR wide_error_string[1024];
    StringCchPrintfW(wide_error_string, ARRAYSIZE(wide_error_string), L"Error: %08x: %s", m_hr, system_message);
    const int byte_count = WideCharToMultiByte(CP_UTF8, 0, wide_error_string, -1, nullptr, 0, nullptr, nullptr);

    // Bound message_length to something reasonable to mitigate integer overflow potential on operator new.
    // byte_count includes null terminator.
    if(byte_count > 0)
    {
        // Allow the possibility of memory failure in this exception, as this matches the behavior of MSVC CRT.
        std::unique_ptr<char[]> utf8_error_string(new(std::nothrow) char[byte_count]);
        if(utf8_error_string != nullptr)
        {
            if(WideCharToMultiByte(CP_UTF8, 0, wide_error_string, -1, utf8_error_string.get(), byte_count, nullptr, nullptr) == byte_count)
            {
                std::swap(utf8_error_string, m_error_string);
            }
            else
            {
                PortableRuntime::dprintf("Invalid conversion in exception: %08x.\n", hresult_from_last_error());
            }
        }
        else
        {
            PortableRuntime::dprintf("Buffer allocation failed in exception.\n");
        }
    }
    else
    {
        PortableRuntime::dprintf("Invalid conversion in exception: %08x.\n", hresult_from_last_error());
    }
}

HRESULT_exception::HRESULT_exception(const HRESULT_exception& that) noexcept : m_hr(E_FAIL), m_error_string(nullptr)
{
    *this = that;
}

HRESULT_exception& HRESULT_exception::operator=(const HRESULT_exception& that) noexcept
{
    if(this != &that)
    {
        m_hr = that.m_hr;

        std::unique_ptr<char[]> error_string;
        if(that.m_error_string != nullptr)
        {
            const size_t buffer_size = strlen(that.m_error_string.get()) + 1;
            error_string.reset(new(std::nothrow) char[buffer_size]);
            if(error_string != nullptr)
            {
                strcpy_s(error_string.get(), buffer_size, that.m_error_string.get());
            }
            else
            {
                PortableRuntime::dprintf("Buffer allocation failed in exception.\n");
            }
        }

        std::swap(error_string, m_error_string);
    }

    return *this;
}

// TODO: Keep this function until all projects have been converted away from it (particularly the Direct3D Exception class).
_Use_decl_annotations_
void HRESULT_exception::get_error_string(PTSTR error_string, size_t size) const noexcept
{
#ifdef _D3D9_H_
    // D3D errors should use D3D9_exception.
    assert(HRESULT_FACILITY(m_hr) != _FACD3D);
#endif

    // FORMAT_MESSAGE_IGNORE_INSERTS is required as arbitrary system messages may contain inserts.
    TCHAR message[128];
    if(0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          nullptr,
                          m_hr,
                          0,
                          message,
                          ARRAYSIZE(message),
                          nullptr))
    {
        StringCchCopy(message, ARRAYSIZE(message), TEXT("Unknown"));
    }

    StringCchPrintf(error_string, size, TEXT("Error: %08x: %s"), m_hr, message);
}

const char* HRESULT_exception::what() const noexcept
{
    return m_error_string != nullptr ? m_error_string.get() : std::exception::what();
}

HRESULT hresult_from_last_error() noexcept
{
    const DWORD error = GetLastError();
    const HRESULT hr = HRESULT_FROM_WIN32(error);
    assert(FAILED(hr));

    return hr;
}

} // namespace WindowsCommon

