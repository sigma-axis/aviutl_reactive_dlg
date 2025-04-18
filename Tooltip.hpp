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

using byte = uint8_t;
#include <exedit.hpp>


////////////////////////////////
// ツールチップの共有管理．
////////////////////////////////
namespace reactive_dlg::Tooltip
{
	inline constinit struct Settings {
		bool animation = true;
		uint16_t delay = 340, duration = 10000;
		int32_t text_color = -1;

		void load(char const* ini_file);
	} settings;

	/// starts or terminates the functinality.
	/// @param hwnd the handle to the window of this plugin.
	/// @param initializing `true` when hooking, `false` when unhooking.
	/// @param module_id currently unused.
	/// @return `true` when hook/unhook was necessary and successfully done, `false` otherwise.
	bool setup(HWND hwnd, bool initializing, [[maybe_unused]] uint32_t module_id);
	inline constinit HWND tooltip = nullptr;

	// color conversion.
	constexpr auto bgr2rgb(int32_t c) {
		return (std::rotl<uint32_t>(c, 16) & 0x00ff00ff) | (c & 0x0000ff00);
	}

	struct tooltip_content_base {
		constexpr bool is_valid() const {
			auto const& sz = const_cast<tooltip_content_base*>(this)->size();
			return sz.cx > 0 && sz.cy > 0;
		}
		void invalidate() { auto& sz = size(); sz.cx = sz.cy = 0; }

		virtual SIZE& size() = 0;
		virtual bool is_tip_worthy() const = 0;
		virtual void measure(HDC dc) = 0;
		virtual void draw(HDC dc, RECT const& rc) const = 0;
	};

	bool tooltip_callback(LRESULT& ret, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, tooltip_content_base& content);
}
