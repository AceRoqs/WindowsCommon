#include "PreCompile.h"
#include "WindowsGL.h"      // Pick up forward declarations to ensure correctness.
#include "CheckHR.h"
#include <PortableRuntime/CheckException.h>     // TODO: Can this be removed?
#include <PortableRuntime/Tracing.h>

namespace WindowsCommon
{

#ifndef NDEBUG // These functions are not currently called in Release.  Avoid C4505: unreferenced local function has been removed.
static std::vector<std::string> tokenize_string(_In_z_ const char* str, _In_z_ const char* delimiters)
{
    std::vector<std::string> tokens;

    const char* begin = str;
    const char* end = str + strlen(str);
    const char* delimiters_end = delimiters + strlen(delimiters);

    const char* iter;
    while((iter = std::find_first_of(begin, end, delimiters, delimiters_end)) != end)
    {
        tokens.emplace_back(std::string(begin, iter - begin));
        begin = iter + 1;
    }

    // Handle case where delimiter is not in string (usually the final token).
    if(begin < end)
    {
        tokens.emplace_back(begin);
    }

    return tokens;
}

static std::vector<std::string> get_opengl_extensions()
{
    // The VMware OpenGL Mesa driver includes an extra space on the end of the string.
    // The tokenizer works either way, but this appears to be working around buggy tokenizers that
    // miss the final OpenGL extension string.
    const char* extensions_string = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

    std::vector<std::string> extensions;
    if(extensions_string != nullptr)
    {
        extensions = tokenize_string(extensions_string, " ");
        std::sort(std::begin(extensions), std::end(extensions));
    }

    return extensions;
}
#endif

static void dprintf_gl_strings()
{
#ifndef NDEBUG
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    PortableRuntime::dprintf("OpenGL vendor: %s\n", vendor != nullptr ? vendor : "");

    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    PortableRuntime::dprintf("OpenGL renderer: %s\n", renderer != nullptr ? renderer : "");

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    PortableRuntime::dprintf("OpenGL version: %s\n", version != nullptr ? version : "");

    auto extensions = get_opengl_extensions();
    PortableRuntime::dprintf("OpenGL extensions:\n");
    std::for_each(extensions.cbegin(), extensions.cend(), [](const std::string& extension)
    {
        PortableRuntime::dprintf("%s\n", extension.c_str());
    });
#endif
}

static bool is_window_32bits_per_pixel(_In_ HWND window)
{
    Scoped_device_context device_context = get_device_context(window);

    if(::GetDeviceCaps(device_context, BITSPIXEL) < 32)
    {
        return false;
    }

    return true;
}

_Use_decl_annotations_
LRESULT OpenGL_window::window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) noexcept
{
    LRESULT return_value = Window_procedure::window_proc(window, message, w_param, l_param);

    switch(message)
    {
        case WM_SIZE:
        {
            RECT client_rectangle;
            GetClientRect(window, &client_rectangle);

            glViewport(client_rectangle.left, client_rectangle.top, client_rectangle.right, client_rectangle.bottom);

            break;
        }

        case WM_ERASEBKGND:
        {
            // Do not erase background, as the whole window area will be refreshed anyway.
            break;
        }

        case WM_DESTROY:
        {
            m_state.make_current_context.invoke();
            m_state.gl_context.invoke();
            m_state.device_context.invoke();

            // No need to invoke destructor of window, as that would dispatch another WM_DESTROY.
            m_window.release();

            break;
        }
    }

    return return_value;
}

// TODO: set window width/height if full screen
_Use_decl_annotations_
OpenGL_window::OpenGL_window(PCSTR window_title, HINSTANCE instance, bool windowed) : m_windowed(windowed)
{
    constexpr int window_width = 800;
    constexpr int window_height = 600;

    const Window_class window_class = get_default_blank_window_class(instance, Window_procedure::static_window_proc, window_title);

    m_atom = register_window_class(window_class);

    if(windowed)
    {
        m_window = create_normal_window(window_title, window_title, window_width, window_height, instance, this);
    }
    else
    {
        DEVMODEW DevMode {};
        DevMode.dmSize = sizeof(DEVMODE);
        DevMode.dmBitsPerPel = 32;
        DevMode.dmPelsWidth = window_width;
        DevMode.dmPelsHeight = window_height;
        DevMode.dmFields = DM_BITSPERPEL;

        ChangeDisplaySettingsW(&DevMode, CDS_FULLSCREEN);
        DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettingsW(&DevMode, CDS_FULLSCREEN);

        m_window = create_window(
            window_title,
            window_title,
            WS_POPUP | WS_CLIPSIBLINGS,
            0,
            0,
            DevMode.dmPelsWidth,
            DevMode.dmPelsHeight,
            nullptr,
            nullptr,
            instance,
            this);

        ShowCursor(false);
    }

    // TODO: 2014: This message text is good - find some way to pass this via the exception.
    //MessageBox(window, TEXT("3D Engine demo requires 32-bit color."), TEXT("System requirements"), MB_OK);
    CHECK_EXCEPTION(is_window_32bits_per_pixel(m_window), u8"Window is not 32bpp.");

    m_state.device_context = get_device_context(m_window);
    m_state.gl_context = create_gl_context(m_state.device_context);
    m_state.make_current_context = create_current_context(m_state.device_context, m_state.gl_context);

    dprintf_gl_strings();
}

OpenGL_window::~OpenGL_window() noexcept
{
    // TODO: 2014: This is just a placeholder - the fullscreen OpenGL code isn't currently exercised.
    // TODO: 2016: Should be able to always do this, as it's a null operation if it wasn't set to begin with.
    if(!m_windowed)
    {
        ChangeDisplaySettingsW(nullptr, 0);
    }
}

const Scoped_window& OpenGL_window::window() const noexcept
{
    return m_window;
}

_Use_decl_annotations_
Scoped_gl_context create_gl_context(HDC device_context)
{
    const PIXELFORMATDESCRIPTOR descriptor =
    {
        sizeof(PIXELFORMATDESCRIPTOR),      // Size of this descriptor.
        1,                                  // Version number.
        PFD_DRAW_TO_WINDOW |                // Support window.
        PFD_SUPPORT_OPENGL |                // Support OpenGL.
        PFD_GENERIC_ACCELERATED |           // Support hardware acceleration.
        PFD_DOUBLEBUFFER,                   // Double buffered.
        PFD_TYPE_RGBA,                      // RGBA type.
        32,                                 // 32-bit color depth.
        0, 0, 0, 0, 0, 0,                   // Color bits ignored.
        0,                                  // No alpha buffer.
        0,                                  // Shift bit ignored.
        0,                                  // No accumulation buffer.
        0, 0, 0, 0,                         // Accum bits ignored.
        24,                                 // 24-bit z-buffer.
        8,                                  // 8-bit stencil buffer.
        0,                                  // No auxiliary buffer.
        PFD_MAIN_PLANE,                     // Main layer.
        0,                                  // Reserved.
        0, 0, 0                             // Layer masks ignored.
    };

    const int pixel_format = ChoosePixelFormat(device_context, &descriptor);
    CHECK_BOOL_LAST_ERROR(pixel_format != 0);

    CHECK_BOOL_LAST_ERROR(SetPixelFormat(device_context, pixel_format, &descriptor));

    const auto rendering_context = wglCreateContext(device_context);
    CHECK_BOOL_LAST_ERROR(nullptr != rendering_context);

    return make_scoped_gl_context(rendering_context);
}

_Use_decl_annotations_
Scoped_current_context create_current_context(HDC device_context, HGLRC gl_context)
{
    CHECK_BOOL_LAST_ERROR(wglMakeCurrent(device_context, gl_context));

    return make_scoped_current_context(gl_context);
}

static void delete_gl_context(_In_ HGLRC gl_context) noexcept
{
    if(!wglDeleteContext(gl_context))
    {
        const auto hr = hresult_from_last_error();
        (hr);
        assert(SUCCEEDED(hr));
    }
}

_Use_decl_annotations_
Scoped_gl_context make_scoped_gl_context(HGLRC gl_context)
{
    return Scoped_gl_context(gl_context, std::function<void (HGLRC)>(delete_gl_context));
}

static void clear_gl_context(_In_opt_ HGLRC gl_context) noexcept
{
    (void)gl_context;   // Unreferenced parameter.

    if(!wglMakeCurrent(nullptr, nullptr))
    {
        const auto hr = hresult_from_last_error();
        (hr);
        assert(SUCCEEDED(hr));
    }
}

_Use_decl_annotations_
Scoped_current_context make_scoped_current_context(HGLRC gl_context)
{
    // TODO: I can't think of a better way than to pass a gl_context, even though it is unused.
    // A non-null variable is required for the deleter to be part of move construction.
    return Scoped_current_context(gl_context, std::function<void (HGLRC)>(clear_gl_context));
}

}

