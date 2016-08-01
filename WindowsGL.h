#pragma once

#include <WindowsCommon/ScopedWindowsTypes.h>
#include <WindowsCommon/Wrappers.h>

namespace WindowsCommon
{

typedef Scoped_resource<HGLRC> Scoped_gl_context;

class WGL_state
{
    Scoped_device_context m_device_context{};
    Scoped_gl_context m_gl_context{};

public:
    ~WGL_state() noexcept;

    void attach(_In_ HWND window, Window_procedure& proc);
    void detach();

    void make_current();
};

class OpenGL_window : public Window_procedure
{
    bool m_windowed{};

    // The order of these fields matter, as destruction must happen in the opposite order.
    Scoped_atom m_atom{};
    Scoped_window m_window{};
    WGL_state m_state{};

protected:
    virtual LRESULT window_proc(_In_ HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept override;

public:
    OpenGL_window(_In_ PCSTR window_title, _In_ HINSTANCE instance, bool windowed);
    virtual ~OpenGL_window() noexcept override;

    const Scoped_window& window() const noexcept;
};

}

