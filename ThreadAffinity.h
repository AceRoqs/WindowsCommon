#pragma once

namespace WindowsCommon
{

void lock_thread_to_first_processor();
void debug_set_current_thread_name(_In_z_ PCSTR thread_name) noexcept;

}

