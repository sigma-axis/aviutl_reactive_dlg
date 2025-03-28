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


////////////////////////////////
// 変化方法のツールチップ表示．
////////////////////////////////
namespace reactive_dlg::Easings::Tooltip
{
	inline constinit struct Settings {
		bool mode = true;
		struct {
			int8_t left, right;
			bool zigzag;
			bool is_enabled() const { return left != 0 || right != 0; }
		} values{ 5, 5, false };

		bool animation = true;
		uint16_t delay = 340, duration = 10000;
		int32_t text_color = -1;

		struct {
			bool enabled;
			int32_t width, height;
			uint32_t plots, curve_width;
			uint32_t curve_color, line_color_1, line_color_2, line_color_3;

			constexpr static size_t pixel_scale = 256;
		} graph{ true, 64, 64, 17, 384, 0xff0000, 0x000000, 0x808080, 0xc0c0c0 };

		void load(char const* ini_file);
		bool is_enabled() const {
			return mode || values.is_enabled();
		}
	} settings;

	/// starts or terminates the functinality.
	/// @param hwnd the handle to the window of this plugin.
	/// @param initializing `true` when hooking, `false` when unhooking.
	/// @return `true` when hook/unhook was necessary and successfully done, `false` otherwise.
	bool setup(HWND hwnd, bool initializing);
}
