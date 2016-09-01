#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <string>

#include <sal.h>

#ifdef _MSC_VER

// APIs for MSVCRT UTF-8 output.
#include <fcntl.h>
#include <io.h>

#endif

#ifdef _WIN32

#include <windows.h>

#endif

