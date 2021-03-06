#include "PreCompile.h"
#include "Wrappers.h"       // Pick up forward declarations to ensure correctness.
#include "CheckHR.h"
#include "WindowMessages.h"
#include <PortableRuntime/CheckException.h>
#include <PortableRuntime/Unicode.h>
#include <PortableRuntime/Tracing.h>

namespace WindowsCommon
{

_Use_decl_annotations_
LRESULT Window_procedure::window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept
{
    LRESULT return_value;

    if(m_handlers.count(message) != 0)
    {
        return_value = m_handlers[message](window, message, w_param, l_param);
    }
    else
    {
        return_value = DefWindowProc(window, message, w_param, l_param);
    }

    return return_value;
}

_Use_decl_annotations_
LRESULT CALLBACK Window_procedure::static_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept
{
    PortableRuntime::dprintf("%s\n", string_from_window_message(message));

    // Sent by CreateWindow.
    if(message == WM_NCCREATE)
    {
        const auto create_struct = reinterpret_cast<const CREATESTRUCT*>(l_param);

        // This function should never fail.
        const auto app = reinterpret_cast<const Window_procedure*>(create_struct->lpCreateParams);
        assert(app != nullptr);     // Confirm that instance was passed to CreateWindow.
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    LRESULT return_value;

    // GetWindowLongPtr should never fail.
    // The variable 'app' is not valid until WM_NCCREATE has been sent.
    const auto app = reinterpret_cast<Window_procedure*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if(app != nullptr)
    {
        return_value = app->window_proc(window, message, w_param, l_param);
    }
    else
    {
        return_value = DefWindowProcW(window, message, w_param, l_param);
    }

    return return_value;
}

void Window_procedure::add_handler(unsigned int message, std::function<LRESULT(_In_ HWND window, UINT message, WPARAM w_param, LPARAM l_param)> handler)
{
    assert(m_handlers.count(message) == 0);

    m_handlers[message] = handler;
}

void Window_procedure::remove_handler(unsigned int message)
{
    m_handlers.erase(message);
}

_Use_decl_annotations_
Window_class::Window_class(UINT style, WNDPROC window_proc, int class_extra, int window_extra, HINSTANCE instance, HICON icon, HCURSOR cursor,
                           HBRUSH background, PCSTR menu_name, PCSTR class_name, HICON small_icon) :
    m_menu_name(menu_name ? PortableRuntime::utf16_from_utf8(menu_name) : L""),
    m_class_name(PortableRuntime::utf16_from_utf8(class_name))
{
    m_window_class.cbSize        = sizeof(m_window_class);
    m_window_class.style         = style;
    m_window_class.lpfnWndProc   = window_proc;
    m_window_class.cbClsExtra    = class_extra;
    m_window_class.cbWndExtra    = window_extra;
    m_window_class.hInstance     = instance;
    m_window_class.hIcon         = icon;
    m_window_class.hCursor       = cursor;
    m_window_class.hbrBackground = background;
    m_window_class.lpszMenuName  = m_menu_name.length() > 0 ? m_menu_name.c_str() : nullptr;
    m_window_class.lpszClassName = m_class_name.c_str();
    m_window_class.hIconSm       = small_icon;
}

Window_class::Window_class(Window_class&& other) noexcept :
    m_menu_name(std::move(other.m_menu_name)),
    m_class_name(std::move(other.m_class_name)),
    m_window_class(other.m_window_class)
{
    m_window_class.lpszMenuName  = m_menu_name.length() > 0 ? m_menu_name.c_str() : nullptr;
    m_window_class.lpszClassName = m_class_name.c_str();
}

Window_class::operator const WNDCLASSEXW&() const noexcept
{
    return m_window_class;
}

_Use_decl_annotations_
Window_class get_default_blank_window_class(HINSTANCE instance, WNDPROC window_proc, PCSTR window_class_name) noexcept
{
    Window_class window_class(CS_HREDRAW | CS_VREDRAW,
                              window_proc,
                              0,
                              0,
                              instance,
                              nullptr,
                              LoadCursorW(nullptr, IDC_ARROW),
                              nullptr,
                              nullptr,
                              window_class_name,
                              nullptr);

    // Return value optimization expected.
    return window_class;
}

Scoped_atom register_window_class(const WNDCLASSEXW& window_class)
{
    const auto atom = RegisterClassExW(&window_class);

    if(0 == atom)
    {
        const auto hr = hresult_from_last_error();
        assert(HRESULT_FROM_WIN32(ERROR_CLASS_ALREADY_EXISTS) != hr);
        CHECK_HR(hr);
    }

    return make_scoped_window_class(atom, window_class.hInstance);
}

_Use_decl_annotations_
Scoped_window create_window(
    PCSTR class_name,
    PCSTR window_name,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    HMENU menu,
    HINSTANCE instance,
    PVOID param)
{
    const auto class_name_utf16 = PortableRuntime::utf16_from_utf8(class_name);
    const auto window_name_utf16 = PortableRuntime::utf16_from_utf8(window_name);
    const auto window = CreateWindowW(class_name_utf16.c_str(), window_name_utf16.c_str(), style, x, y, width, height, parent, menu, instance, param);

    CHECK_BOOL_LAST_ERROR(nullptr != window);

    return make_scoped_window(window);
}

_Use_decl_annotations_
Scoped_window create_normal_window(PCSTR class_name, PCSTR window_name, int width, int height, HINSTANCE instance, PVOID param){
    return create_window(
        class_name,             // class_name.
        window_name,            // window_name.
        WS_OVERLAPPEDWINDOW |
        WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS,        // style.
        CW_USEDEFAULT,          // x.
        CW_USEDEFAULT,          // y.
        width,                  // width.
        height,                 // height.
        HWND_DESKTOP,           // parent.
        nullptr,                // menu.
        instance,               // instance.
        param);                 // param.
}

_Use_decl_annotations_
Scoped_device_context get_device_context(HWND window)
{
    const auto device_context = GetDC(window);
    CHECK_EXCEPTION(nullptr != device_context, u8"Device context is null.");

    return make_scoped_device_context(device_context, release_device_context_functor(window));
}

_Use_decl_annotations_
Scoped_handle create_file(
    PCSTR file_name,
    DWORD desired_access,
    DWORD share_mode,
    PSECURITY_ATTRIBUTES security_attributes,
    DWORD creation_disposition,
    DWORD flags,
    HANDLE template_file)
{
    const auto handle = CreateFileW(PortableRuntime::utf16_from_utf8(file_name).c_str(),
                                    desired_access,
                                    share_mode,
                                    security_attributes,
                                    creation_disposition,
                                    flags,
                                    template_file);

    CHECK_BOOL_LAST_ERROR(INVALID_HANDLE_VALUE != handle);

    return make_scoped_handle(handle);
}

_Use_decl_annotations_
Scoped_handle create_event(
    PSECURITY_ATTRIBUTES security_attributes,
    bool manual_reset,
    bool initial_state,
    PCSTR name)
{
    const auto handle = CreateEventW(security_attributes,
                                     manual_reset,
                                     initial_state,
                                     name != nullptr ? PortableRuntime::utf16_from_utf8(name).c_str() : nullptr);

    CHECK_BOOL_LAST_ERROR(INVALID_HANDLE_VALUE != handle);
    assert(handle != 0);    // Per Win32 contract.
    _Analysis_assume_(handle != 0);

    return make_scoped_handle(handle);
}

_Use_decl_annotations_
Scoped_font select_font(HFONT font, HDC device_context)
{
    const auto old_font = static_cast<HFONT>(SelectObject(device_context, static_cast<HGDIOBJ>(font)));
    CHECK_EXCEPTION(old_font != nullptr, u8"Font handle is null.");

    return make_scoped_font(old_font, select_object_functor(device_context));
}

_Use_decl_annotations_
Scoped_font create_font_indirect(LOGFONT* log_font)
{
    const HFONT font = CreateFontIndirectW(log_font);
    CHECK_EXCEPTION(font != nullptr, u8"Font handle is null.");

    return make_scoped_font(font);
}

_Use_decl_annotations_
Scoped_device_context begin_paint(HWND window, PAINTSTRUCT* paint_struct)
{
    const auto device_context = BeginPaint(window, paint_struct);
    CHECK_EXCEPTION(device_context != nullptr, u8"Device context is null.");

    return make_scoped_device_context(device_context, end_paint_functor(window, paint_struct));
}

}

