/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <string>
#include <memory>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "modkeys.hpp"


////////////////////////////////
// テキストボックス検索/操作．
////////////////////////////////
namespace reactive_dlg::TextBox
{
	namespace mk = sigma_lib::modifier_keys;

	inline constinit struct Settings {
		using modkeys = mk::modkeys;

		mk::shortcut_key
			focus_forward{ VK_TAB, modkeys::none },
			focus_backward{ VK_TAB, modkeys::shift };

		bool batch = true;

		int16_t tabstops_text = -1, tabstops_script = -1;
		std::unique_ptr<std::wstring> replace_tab_text{}, replace_tab_script{};

		void load(char const* ini_file);
	} settings;

	/// starts or terminates the functinality.
	/// @param hwnd the handle to the window of this plugin.
	/// @param initializing `true` when hooking, `false` when unhooking.
	/// @return `true` when hook/unhook was necessary and successfully done, `false` otherwise.
	bool setup(HWND hwnd, bool initializing);

	/// determines the next focus target when traversing through text boxes.
	/// @param curr the currently focused edit.
	/// @param forward `true` when the focus is moving forward, `false` when backward.
	/// @return one of the following `HWND`:
	/// the timeline window, the edit for the text object, or the edit for Lua script.
	HWND next_edit_box(HWND curr, bool forward);
}
