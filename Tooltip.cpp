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

#include "inifile_op.hpp"

#include "reactive_dlg.hpp"
#include "Tooltip.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// ツールチップの共有管理．
////////////////////////////////
using namespace reactive_dlg::Tooltip;

static constinit int refcount = 0;

NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Tooltip;

bool expt::setup(HWND hwnd, bool initializing, [[maybe_unused]] uint32_t module_id)
{
	if (initializing && (refcount++) == 0) {
		// create the tooltip window.
		tooltip = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP |
			(settings.animation ? TTS_NOFADE | TTS_NOANIMATE : 0),
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hwnd, nullptr, exedit.fp->hinst_parent, nullptr);

		::SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// settings of delay time for the tooltip.
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 0xffff & settings.delay);
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 0xffff & settings.duration);
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_RESHOW, 0xffff & (settings.delay / 5));
		// initial values are... TTDT_INITIAL: 340, TTDT_AUTOPOP: 3400, TTDT_RESHOW: 68.
	}
	else if (!initializing && (--refcount) == 0){
		::DestroyWindow(tooltip);
		tooltip = nullptr;
	}

	return true;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Tooltip";
	constexpr int min_color = 0x000000, max_color = 0xffffff;
	constexpr int min_time = 0, max_time = 60'000;

#define read(func, hd, fld, ...)	hd fld = read_ini_##func(hd fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool,,	animation);
	read(int,,	delay,		min_time, max_time);
	read(int,,	duration,	min_time, max_time);
	read(int,,	text_color,	-1, max_color);

#undef read
}
