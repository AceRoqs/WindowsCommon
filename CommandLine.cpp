#include "PreCompile.h"
#include <WindowsCommon/CheckHR.h>
#include <WindowsCommon/ScopedWindowsTypes.h>
#include <PortableRuntime/Unicode.h>

namespace WindowsCommon {

std::vector<std::string> args_from_command_line()
{
    std::vector<std::string> args;

    const auto command_line = GetCommandLineW();

    int arg_count;
    const auto naked_args = CommandLineToArgvW(command_line, &arg_count);
    CHECK_BOOL_LAST_ERROR(naked_args != nullptr);

    const auto wide_args = WindowsCommon::make_scoped_local(naked_args);

    std::for_each(naked_args, naked_args + arg_count, [&args](PCWSTR arg)
    {
        args.push_back(PortableRuntime::utf8_from_utf16(arg));
    });

    return args;
}

}

