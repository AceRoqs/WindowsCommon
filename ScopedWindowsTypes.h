#pragma once

namespace WindowsCommon
{

template<typename RESOURCE, typename DELETER=std::function<void (RESOURCE)>>
class Scoped_resource
{
    DELETER m_deleter{};
    RESOURCE m_resource{};

    // Not implemented to prevent accidental copying.
    Scoped_resource(const Scoped_resource&) = delete;
    Scoped_resource& operator=(const Scoped_resource&) = delete;

public:
    explicit Scoped_resource() noexcept :
        m_resource{}
    {
    }

    explicit Scoped_resource(RESOURCE resource, DELETER&& deleter) noexcept :
        m_deleter(std::move(deleter)),
        m_resource(resource)
    {
    }

    Scoped_resource(Scoped_resource&& other) noexcept :
        m_deleter(std::move(other.m_deleter)),
        m_resource(std::move(other.m_resource))
    {
        other.release();
    }

    ~Scoped_resource() noexcept
    {
        invoke();
    }

    Scoped_resource& operator=(Scoped_resource&& other) noexcept
    {
        // Handle A=A case.
        if(this != &other)
        {
            invoke();
            m_deleter = std::move(other.m_deleter);
            m_resource = std::move(other.m_resource);
            other.release();
        }

        return *this;
    }

    operator const RESOURCE&() const noexcept
    {
        return m_resource;
    }

    void invoke() noexcept
    {
        if(m_resource != 0)
        {
            m_deleter(m_resource);
            m_resource = 0;
        }
    }

    RESOURCE release() noexcept
    {
        RESOURCE resource = m_resource;
        m_resource = 0;

        return resource;
    }
};

typedef Scoped_resource<ATOM> Scoped_atom;
typedef Scoped_resource<HWND> Scoped_window;
typedef Scoped_resource<HDC> Scoped_device_context;
typedef Scoped_resource<HANDLE> Scoped_handle;
typedef Scoped_resource<HFONT> Scoped_font;
typedef Scoped_resource<HLOCAL> Scoped_local;

Scoped_atom make_scoped_window_class(_In_ ATOM atom, _In_ HINSTANCE instance);
Scoped_window make_scoped_window(_In_ HWND window);

std::function<void (HDC)> release_device_context_functor(_In_ HWND window) noexcept;
std::function<void (HDC)> end_paint_functor(_In_ HWND window, _In_ PAINTSTRUCT* paint_struct) noexcept;

Scoped_device_context make_scoped_device_context(_In_ HDC device_context, std::function<void (HDC)> deleter);
Scoped_handle make_scoped_handle(_In_ HANDLE handle);

std::function<void (HFONT)> select_object_functor(_In_ HDC device_context) noexcept;
void delete_object(_In_ HFONT font) noexcept;

Scoped_font make_scoped_font(_In_ HFONT font, std::function<void (HFONT)> deleter = delete_object);

Scoped_local make_scoped_local(_In_ HLOCAL local);

// This class must remain exception free, as it will be used outside of the outermost try block
// so that fatal error messages have the proper code page.
class UTF8_console_code_page
{
    UINT m_code_page{CP_ACP};

public:
    UTF8_console_code_page() noexcept;
    ~UTF8_console_code_page() noexcept;
};

}

