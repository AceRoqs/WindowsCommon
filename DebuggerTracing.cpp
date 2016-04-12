#include "PreCompile.h"
#include "DebuggerTracing.h"            // Pick up forward declarations to ensure correctness.
#include <PortableRuntime/Unicode.h>

namespace WindowsCommon
{

static bool is_beep_character(char16_t ch) noexcept
{
    return ch == u'^';
}

static bool is_break_character(char16_t ch) noexcept
{
    return ch == u'!';
}

static bool is_control_character(char16_t ch) noexcept
{
    return is_beep_character(ch) || is_break_character(ch);
}

// Note that it is not possible to ensure that the debugging tool that is capturing
// the traces has the fonts to display all required UTF-8 characters.
_Use_decl_annotations_
void debugger_dprintf(const char* output_string) noexcept
{
#ifndef NDEBUG
    const auto utf16_string = PortableRuntime::utf16_from_utf8(output_string);
    const auto c_string = utf16_string.c_str();

    // Strip the control character for display so that trace lines can be selected in
    // the Output window for fast navigation to the trace source in Visual Studio.
    // TODO: 2016: Consider making all traces have file/line.
    const auto display_string = is_control_character(c_string[0]) ? c_string + 1 : c_string;

    // Require debugger attached as to not block other development
    // when this trace doesn't require immediate attention.
    if(IsDebuggerPresent())
    {
        if(is_beep_character(c_string[0]))
        {
            MessageBeep(MB_ICONASTERISK);
        }
        else if(is_break_character(c_string[0]))
        {
            DebugBreak();
        }
    }

    // OutputDebugStringA is faster, but it accepts ANSI, not UTF-8.
    OutputDebugStringW(display_string);
#else
    (void)(output_string);
#endif
}

}

