/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "memory_protect.hpp"
#include "inifile_op.hpp"

#include "reactive_dlg.hpp"
#include "Easings.hpp"
#include "Easings_Misc.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの変化方法の諸機能．
////////////////////////////////
using namespace reactive_dlg::Easings;
using namespace reactive_dlg::Easings::Misc;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK param_button_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_MBUTTONUP:
	{
		// customized behavior of wheel click.
		if (*exedit.SettingDialogObjectIndex >= 0) {
			// send a command to edit the easing parameter.
			constexpr uint16_t param_set_command = 1122;
			::PostMessageW(*exedit.hwnd_setting_dlg, WM_COMMAND, param_set_command, static_cast<LPARAM>(data));
		}
		break;
	}

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &param_button_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings::Misc;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (initializing) {
		if (settings.linked_track_invert_shift) {
			// 0f 8c 87 00 00 00	jl
			// V
			// 0f 8d 87 00 00 00	jge
			sigma_lib::memory::ProtectHelper::write(exedit.cmp_shift_state_easing + 1, byte{ 0x8d });
		}

		if (settings.wheel_click) {
			for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++)
				::SetWindowSubclass(exedit.hwnd_track_buttons[i], &param_button_hook, hook_uid(), { i });
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Easings";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool,	linked_track_invert_shift);
	read(bool,	wheel_click);

#undef read
}
