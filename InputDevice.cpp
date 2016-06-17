#include "PreCompile.h"
#include "InputDevice.h"    // Pick up forward declarations to ensure correctness.
#include "CheckHR.h"

namespace WindowsCommon
{

_Use_decl_annotations_
Input_device::Input_device(HINSTANCE instance, HWND hwnd)
{
    // Create DirectInput keyboard device.
    Microsoft::WRL::ComPtr<IDirectInput8> direct_input;
    CHECK_HR(DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<PVOID*>(direct_input.GetAddressOf()), nullptr));
    CHECK_HR(direct_input->CreateDevice(GUID_SysKeyboard, m_device.GetAddressOf(), nullptr));

    assert(c_dfDIKeyboard.dwDataSize == keyboard_buffer_size);
    CHECK_HR(m_device->SetDataFormat(&c_dfDIKeyboard));
    CHECK_HR(m_device->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));

    // Acquisition is done before input is read the first time.

    direct_input.Reset();
}

Input_device::~Input_device() noexcept
{
    m_device->Unacquire();
}

_Use_decl_annotations_
void Input_device::get_input(Keyboard_state* keyboard_state) const
{
    if(SUCCEEDED(m_device->Acquire()))
    {
        CHECK_HR(m_device->GetDeviceState(static_cast<DWORD>(keyboard_state->size()), keyboard_state));
    }
    else
    {
        // Assume no state change if device cannot be acquired.
        keyboard_state->fill(0);
    }
}

}

