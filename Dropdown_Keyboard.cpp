/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

#include "reactive_dlg.hpp"
#include "Dropdown_Keyboard.hpp"
#include "inifile_op.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// Combo Box でのキー入力．
////////////////////////////////
using namespace reactive_dlg::Dropdown::Keyboard;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }
static constinit HWND combo = nullptr;


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK mainwindow_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto...)
{
	// 設定ダイアログへの通常キー入力はなぜか
	// AviUtl のメインウィンドウに送られるようになっている．
	switch (message) {
	case WM_KEYDOWN:
	case WM_CHAR:
		// 念のため無限ループに陥らないようガード．
		if (combo != nullptr) {
			auto t = combo; combo = nullptr;
			::SendMessageW(t, message, wparam, lparam);
			combo = t;
			return 0;
		}
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

static inline LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_COMMAND:
		if (auto ctrl = reinterpret_cast<HWND>(lparam); ctrl != nullptr) {
			switch (wparam >> 16) {
				// 開いている Combo Box の追跡．
			case CBN_DROPDOWN:
				if (check_window_class(ctrl, WC_COMBOBOXW)) {
					combo = ctrl;
					::SetWindowSubclass(exedit.fp->hwnd_parent, &mainwindow_hook, hook_uid(), {});
				}
				break;

			case CBN_CLOSEUP:
				if (check_window_class(ctrl, WC_COMBOBOXW)) {
					combo = nullptr;
					::RemoveWindowSubclass(exedit.fp->hwnd_parent, &mainwindow_hook, hook_uid());
				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &setting_dlg_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Dropdown::Keyboard;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.search) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			::SetWindowSubclass(dlg, &setting_dlg_hook, hook_uid(), {});
		}
		else {
			::RemoveWindowSubclass(dlg, &setting_dlg_hook, hook_uid());
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Dropdown.Keyboard";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool, search);

#undef read
}
