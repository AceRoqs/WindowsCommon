#pragma once

namespace WindowsCommon
{

// Code assumes that keyboard_buffer_size <= MAXDWORD.
const size_t keyboard_buffer_size = 256;
typedef std::array<uint8_t, keyboard_buffer_size> Keyboard_state;

class Input_device
{
    Microsoft::WRL::ComPtr<IDirectInputDevice8> m_device{};

public:
    Input_device(_In_ HINSTANCE instance, _In_ HWND hwnd);
    ~Input_device() noexcept;
    void get_input(_Out_ Keyboard_state* keyboard_state) const;
};

}

