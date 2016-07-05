#pragma once

namespace WindowsCommon
{

class Clock
{
    // Members are all initialized in constructor body, so don't
    // default-init, to prevent double initialization.
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_last_counter;

public:
    Clock() noexcept;
    float ellapsed_milliseconds() noexcept;
};

}

