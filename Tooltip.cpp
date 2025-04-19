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
#include "monitors.hpp"

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

bool expt::tooltip_callback(LRESULT& ret, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, tooltip_content_base& content)
{
	if (*exedit.SettingDialogObjectIndex < 0) return false;

	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	{
		// relayed messages.
		MSG msg{ .hwnd = hwnd, .message = message, .wParam = wparam, .lParam = lparam };
		::SendMessageW(tooltip, TTM_RELAYEVENT,
			message == WM_MOUSEMOVE ? ::GetMessageExtraInfo() : 0, reinterpret_cast<LPARAM>(&msg));
		break;
	}

	case WM_NOTIFY:
		if (auto const hdr = reinterpret_cast<NMHDR*>(lparam); hdr->hwndFrom == tooltip) {
			constexpr auto dummy_text_a = "\t";
			constexpr auto dummy_text_w = L"\t";

			switch (hdr->code) {
			case TTN_GETDISPINFOA:
			{
				// supply the content string, which is this time a dummy,
				// or nothing if tooltip has no use for the current state.
				if (content.is_tip_worthy())
					reinterpret_cast<NMTTDISPINFOA*>(lparam)
						->lpszText = const_cast<char*>(dummy_text_a);
				return ret = {}, true;
			}
			case TTN_SHOW:
			{
				// adjust the tooltip size to fit with the content.
				RECT rc;
				::GetWindowRect(tooltip, &rc);
				::SendMessageW(tooltip, TTM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rc));
				auto const& size = content.size();
				rc.right = rc.left + size.cx + 2; // add slight extra space on the right and bottom.
				rc.bottom = rc.top + size.cy + 1;
				::SendMessageW(tooltip, TTM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&rc));

				// adjust the position not to clip edges of screens.
				rc = sigma_lib::W32::monitor<true>{ rc.left, rc.top }
				.expand(-8).clamp(rc); // 8 pixels of padding.
				::SetWindowPos(tooltip, nullptr, rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return ret = TRUE, true;
			}
			case TTN_POP:
			{
				content.invalidate();
				return ret = {}, true;
			}

			case NM_CUSTOMDRAW:
			{
				auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
				switch (dhdr->nmcd.dwDrawStage) {
				case CDDS_PREPAINT:
				{
					// prepare the tooltip content.
					if (!content.is_valid())
						content.measure(dhdr->nmcd.hdc);

					// actual drawing is done on the CDDS_POSTPAINT notification.
					if (::IsWindowVisible(tooltip) != FALSE)
						return ret = CDRF_NOTIFYPOSTPAINT, true;
					break;
				}
				case CDDS_POSTPAINT:
				{
					auto dc = dhdr->nmcd.hdc;

					// change the text color if specified.
					if (settings.text_color >= 0)
						::SetTextColor(dc, bgr2rgb(settings.text_color));

					// draw the content.
					if (content.is_valid())
						content.draw(dc, dhdr->nmcd.rc);
					break;
				}
				}
				return ret = CDRF_DODEFAULT, true;
			}
			}
		}
		break;
	}
	return false;
}
