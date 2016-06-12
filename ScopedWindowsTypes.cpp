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

UTF8_console_code_page::UTF8_console_code_page()
    : m_code_page(GetConsoleCP())
{
    // Set the console output code page to 65001 (UTF-8).  UTF-8 characters can
    // be passed to printf and the glyphs printed will be correct, given an appropriate font.
    // cout/cerr output the correct bytes, when redirected to a file, but not when printing
    // to a console.  This is a problem only on Windows, as Linux and MacOS do the right
    // thing by default.
    //
    // One option previously attempted was to use wide print functions, which worked well,
    // except it made it difficult to redirect stdin/stdout.  For example, wostream was used
    // to output to files, which means that wcout would have been used to redirect to stdout.
    // However, simple things, such as writing vectors of bytes, were difficult, since the
    // vectors had to be widened first.
    //
    // Even a simple program like the following will not behave identically on all platforms:
    //  #include <cstdio>
    //  int main(int argc, char** argv) {
    //      printf("%s\n", argv[0]);    // If argv[0] has non-ASCII characters,
    //                                  // the behavior differs across platforms.
    //      return 0;
    //  }
    //
    // Of course, the string encoding is not guaranteed by the standard, but as a matter of
    // practicality, modern operating systems generally use UTF-8, and these are the only
    // operating systems of interest here.
    //
    // In the end, Windows and Visual C++ are just broken with respect to UTF-8 output,
    // since it is effectively impossible to write a program that is UTF-8 correct across
    // Windows/Linux/MacOS.  The most portable approach appears to be to use printf for
    // general output, and cout/cerr when passing to streams for redirection.  When opening
    // a fstream on Windows, pass a UTF-16 string as the filename (which also makes the code
    // non-portable).  Lastly, read Windows command line arguments as UTF-16, and convert
    // immediately to UTF-8 for interaction with anything else.
    SetConsoleOutputCP(CP_UTF8);
}

UTF8_console_code_page::~UTF8_console_code_page()
{
    // If the app exits without this destructor, the code page will not be set back.
    // chcp may say that the code page has been set back, but it lies (tested
    // against Windows 10 November 2015 Update).
    SetConsoleOutputCP(m_code_page);
}

}

