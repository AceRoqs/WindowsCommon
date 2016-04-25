#include "PreCompile.h"
#include "ScopedWindowsTypes.h"     // Pick up forward declarations to ensure correctness.
#include "CheckHR.h"

namespace WindowsCommon
{

static void unregister_atom(_In_ ATOM atom, _In_ HINSTANCE instance) noexcept
{
    BOOL result = UnregisterClassW(MAKEINTATOM(atom), instance);
    if(!result)
    {
        const auto hr = hresult_from_last_error();
        (void)(hr);
        assert(SUCCEEDED(hr));
    }
}

static std::function<void (ATOM)> unregister_class_functor(_In_ HINSTANCE instance) noexcept
{
    return [=](ATOM atom)
    {
        unregister_atom(atom, instance);
    };
}

_Use_decl_annotations_
Scoped_atom make_scoped_window_class(ATOM atom, HINSTANCE instance)
{
    return Scoped_atom(atom, unregister_class_functor(instance));
}

static void destroy_window(_In_ HWND window) noexcept
{
    if(!DestroyWindow(window))
    {
        const auto hr = hresult_from_last_error();
        (void)(hr);
        assert(SUCCEEDED(hr));
    }
}

_Use_decl_annotations_
Scoped_window make_scoped_window(HWND window)
{
    return Scoped_window(window, std::function<void (HWND)>(destroy_window));
}

static void release_device_context(_In_ HDC device_context, _In_ HWND window) noexcept
{
    if(!ReleaseDC(window, device_context))
    {
        assert(false);
    }
}

_Use_decl_annotations_
std::function<void (HDC)> release_device_context_functor(HWND window) noexcept
{
    return [=](HDC device_context)
    {
        release_device_context(device_context, window);
    };
}

_Use_decl_annotations_
std::function<void (HDC)> end_paint_functor(HWND window, PAINTSTRUCT* paint_struct) noexcept
{
    return [=](HDC device_context)
    {
        (void)device_context;   // Unreferenced parameter.
        EndPaint(window, paint_struct);
    };
}

_Use_decl_annotations_
Scoped_device_context make_scoped_device_context(HDC device_context, std::function<void (HDC)> deleter)
{
    return Scoped_device_context(device_context, std::move(deleter));
}

static void close_handle(_In_ HANDLE handle) noexcept
{
    if(!CloseHandle(handle))
    {
        const auto hr = hresult_from_last_error();
        (void)(hr);
        assert(SUCCEEDED(hr));
    }
}

_Use_decl_annotations_
Scoped_handle make_scoped_handle(HANDLE handle)
{
    return Scoped_handle(handle, std::function<void (HANDLE)>(close_handle));
}

static void select_object(_In_ HDC device_context, HGDIOBJ gdi_object) noexcept
{
    auto result = SelectObject(device_context, gdi_object);
    (result);
    assert(result != nullptr);
}

_Use_decl_annotations_
std::function<void (HFONT)> select_object_functor(HDC device_context) noexcept
{
    return [=](HFONT font)
    {
        select_object(device_context, font);
    };
}

_Use_decl_annotations_
void delete_object(HFONT font) noexcept
{
    auto result = DeleteObject(font);
    (result);
    assert(result != 0);
}

_Use_decl_annotations_
Scoped_font make_scoped_font(HFONT font, std::function<void (HFONT)> deleter)
{
    return Scoped_font(font, std::move(deleter));
}

static void local_free(_In_ HLOCAL local) noexcept
{
    if(LocalFree(local) != nullptr)
    {
        const auto hr = hresult_from_last_error();
        (void)(hr);
        assert(SUCCEEDED(hr));
    }
}

_Use_decl_annotations_
Scoped_local make_scoped_local(HLOCAL local)
{
    return std::move(Scoped_local(local, std::function<void (HLOCAL)>(local_free)));
}

}

