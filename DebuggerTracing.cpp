#include "PreCompile.h"
#include "DebuggerTracing.h"            // Pick up forward declarations to ensure correctness.
#include <PortableRuntime/Unicode.h>

namespace WindowsCommon
{

// Note that it is not possible to ensure that the debugging tool that is capturing
// the traces has the fonts to display all required UTF-8 characters.
_Use_decl_annotations_
void debugger_dprintf(const char* output_string) noexcept
{
#ifndef NDEBUG
    const auto utf16_string = PortableRuntime::utf16_from_utf8(output_string);
    const auto c_string = utf16_string.c_str();
    const auto display_string = c_string[0] == L'!' ? c_string + 1 : c_string;

    if(c_string[0] == L'!')
    {
        if(IsDebuggerPresent())
        {
            DebugBreak();
        }
    }

    OutputDebugStringW(display_string);
#else
    (void)(output_string);
#endif
}

}

