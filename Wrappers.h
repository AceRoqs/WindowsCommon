#pragma once

#include <WindowsCommon/ScopedWindowsTypes.h>

namespace WindowsCommon
{

class Window_procedure
{
    // Not implemented to prevent accidental copying/moving.  The risk on copy/move is
    // that the original may be inadvertantly destroyed before the HWND itself is.
    Window_procedure(const Window_procedure&) = delete;
    Window_procedure(Window_procedure&&) noexcept = delete;
    Window_procedure& operator=(const Window_procedure&) = delete;
    Window_procedure& operator=(Window_procedure&&) noexcept = delete;

protected:
    virtual LRESULT window_proc(_In_ HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept = 0;

public:
    Window_procedure() = default;
    virtual ~Window_procedure() noexcept = default;
    static LRESULT CALLBACK static_window_proc(_In_ HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept;
};

class Window_class
{
    // Initialize strings first, as m_window_class will reference these by pointer.
    std::wstring m_menu_name{};
    std::wstring m_class_name{};

    // m_window_class must be initialized in constructor body, so don't
    // default init, to prevent double initialization.
    WNDCLASSEXW m_window_class;

    Window_class(const Window_class& other) noexcept = delete;
    Window_class& operator=(const Window_class&) = delete;
    Window_class& operator=(Window_class&&) noexcept = delete;

public:
    Window_class(UINT style, _In_ WNDPROC window_proc, int class_extra, int window_extra, _In_ HINSTANCE instance, _In_opt_ HICON icon, _In_ HCURSOR cursor,
        _In_opt_ HBRUSH background, _In_opt_ PCSTR menu_name, _In_ PCSTR class_name, _In_opt_ HICON small_icon);
    Window_class(Window_class&& other) noexcept;
    ~Window_class() noexcept = default;
    operator const WNDCLASSEXW&() const noexcept;
};

Window_class get_default_blank_window_class(_In_ HINSTANCE instance, _In_ WNDPROC window_proc, _In_ PCSTR window_class_name) noexcept;
Scoped_atom register_window_class(const WNDCLASSEX& window_class);

Scoped_window create_window(_In_opt_ PCSTR class_name, _In_opt_ PCSTR window_name, DWORD style, int x, int y,
    int width, int height, _In_opt_ HWND parent, _In_opt_ HMENU menu, _In_opt_ HINSTANCE instance, _In_opt_ PVOID param);
Scoped_window create_normal_window(_In_ PCSTR class_name, _In_ PCSTR window_name, int width, int height, _In_opt_ HINSTANCE instance, _In_opt_ PVOID param);
Scoped_device_context get_device_context(_In_ HWND window);
Scoped_handle create_file(_In_ PCSTR file_name, DWORD desired_access, DWORD share_mode,
    _In_opt_ PSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags, _In_opt_ HANDLE template_file);
Scoped_handle create_event(_In_opt_ PSECURITY_ATTRIBUTES security_attributes, bool manual_reset, bool initial_state, _In_opt_ PCSTR name);
Scoped_font select_font(_In_ HFONT font, _In_ HDC device_context);
Scoped_font create_font_indirect(_In_ LOGFONT* log_font);
Scoped_device_context begin_paint(_In_ HWND window, _Out_ PAINTSTRUCT* paint_struct);

}

