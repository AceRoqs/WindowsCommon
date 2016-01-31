#pragma once

#include <PortableRuntime/CheckException.h>

namespace WindowsCommon
{

// TODO: Consider making this class private.
class HRESULT_exception : public PortableRuntime::Exception
{
protected:
    HRESULT m_hr;

public:
    HRESULT_exception(HRESULT hr, _In_z_ const char* file_name, int line) noexcept;
};

HRESULT hresult_from_last_error() noexcept;

// Macros allow for usage of __FILE__ and __LINE__.
#define CHECK_HR(zzz_expr)                                                      \
{                                                                               \
    const HRESULT zzz_val = (zzz_expr);                                         \
    if(FAILED(zzz_val))                                                         \
    {                                                                           \
        throw WindowsCommon::HRESULT_exception(zzz_val, __FILE__, __LINE__);    \
    }                                                                           \
}

#define CHECK_BOOL_LAST_ERROR(zzz_expr)                                         \
{                                                                               \
    const bool zzz_val = !!(zzz_expr);                                          \
    if(!zzz_val)                                                                \
    {                                                                           \
        const auto zzz_hr = WindowsCommon::hresult_from_last_error();           \
        throw WindowsCommon::HRESULT_exception(zzz_hr, __FILE__, __LINE__);     \
    }                                                                           \
    _Analysis_assume_((zzz_expr));                                              \
}

} // namespace WindowsCommon

