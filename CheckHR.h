#pragma once

namespace WindowsCommon
{

// TODO: Consider making this class private once get_error_string is removed.
class HRESULT_exception : public std::exception
{
public:
    HRESULT_exception(HRESULT hr) noexcept;
    HRESULT_exception(const HRESULT_exception& that) noexcept;
    HRESULT_exception& operator=(const HRESULT_exception& that) noexcept;

    // Define a method besides exception::what() that doesn't require heap memory allocation.
    virtual void get_error_string(_Out_writes_z_(size) PTSTR error_string, size_t size) const noexcept;
    virtual const char* what() const noexcept override;

    HRESULT m_hr;

private:
    std::unique_ptr<char[]> m_error_string;
};

HRESULT hresult_from_last_error() noexcept;
void check_hr(HRESULT hr);
void check_windows_error(BOOL result);

// Macros allow for usage of __FILE__ and __LINE__.
#define CHECK_HR(zzz_expr)                              \
{                                                       \
    const HRESULT zzz_val = (zzz_expr);                 \
    if(FAILED(zzz_val))                                 \
    {                                                   \
        WindowsCommon::check_hr(zzz_val);               \
    }                                                   \
}

#define CHECK_WINDOWS_ERROR(zzz_expr)                   \
{                                                       \
    const bool zzz_val = (zzz_expr);                    \
    if(!zzz_val)                                        \
    {                                                   \
        WindowsCommon::check_windows_error(zzz_val);    \
    }                                                   \
    _Analysis_assume_((zzz_expr));                      \
}

} // namespace WindowsCommon

