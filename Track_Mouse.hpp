/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "modkeys.hpp"


////////////////////////////////
// トラックバーの周りのマウス入力．
////////////////////////////////
namespace reactive_dlg::Track::Mouse
{
	namespace mk = sigma_lib::modifier_keys;

	inline constinit struct Settings {
		using modkeys = mk::modkeys;

		struct : mk::modkey_boost {
			bool enabled, reverse, cursor_react;
			modkeys keys_activate;

			constexpr bool no_wrong_keys(modkeys keys) const {
				// avoid counting ctrl and activation keys as wrong keys.
				return mk::modkey_boost::no_wrong_keys(keys & ~(keys_activate | modkeys::ctrl));
			}
		} wheel{ modkeys::shift, modkeys::alt, false, 10, true, false, true, modkeys::ctrl };

		struct : mk::modkey_boost {
			bool fixed, vertical, reverse;
			uint8_t step_size, r_step_size;

			constexpr bool is_enabled() const {
				return keys_frac != modkeys::shift || keys_boost != modkeys::none || def_frac
					|| fixed || vertical || reverse
					|| step_size > 1 || r_step_size > 1;
			}
		} drag{ modkeys::shift, modkeys::alt, false, 10, false, false, false, 1, 10 };

		void load(char const* ini_file);
	} settings;

	/// starts or terminates the functinality.
	/// @param hwnd the handle to the window of this plugin.
	/// @param initializing `true` when hooking, `false` when unhooking.
	/// @return `true` when hook/unhook was necessary and successfully done, `false` otherwise.
	bool setup(HWND hwnd, bool initializing);
}
