#pragma once

// Prevent <Windows.h> from defining min/max which is different from <algorithm>.
// Must be included before headers that include WinDef.h.
// http://support.microsoft.com/kb/143208
#define NOMINMAX

#include <cassert>

// C++ Standard Library.
#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Windows API.
#ifdef _WIN32

// Defines to decrease build times:
// http://support.microsoft.com/kb/166474
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <strsafe.h>
#include <processthreadsapi.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <wrl.h>

#endif  // _WIN32

#include <gl/GL.h>

