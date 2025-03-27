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

using byte = uint8_t;
#include <exedit.hpp>

#include "str_encodes.hpp"
#include "inifile_op.hpp"
#include "monitors.hpp"

#include "reactive_dlg.hpp"
#include "Easings.hpp"
#include "Easings_Tooltip.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// 変化方法のツールチップ表示．
////////////////////////////////
using namespace reactive_dlg::Easings;
using namespace reactive_dlg::Easings::Tooltip;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static constinit HWND tooltip = nullptr;

// formatting a string that describes to the track mode.
static inline std::wstring format_easing(ExEdit::Object::TrackMode const& mode, int32_t param, easing_name_spec const& name_spec)
{
	// get name and specification.
	auto ret = sigma_lib::string::encode_sys::to_wide_str(name_spec.name);

	// integral parameter.
	if (name_spec.spec.param)
		ret += L"\nパラメタ: " + std::to_wstring(param);

	// two more booleans.
	if (bool const acc = (mode.num & mode.isAccelerate) != 0,
		dec = (mode.num & mode.isDecelerate) != 0;
		acc || dec) {
		ret += name_spec.spec.param ? L", " : L"\n";
		if (acc) ret += L"+加速 ";
		if (dec) ret += L"+減速 ";
		ret.pop_back();
	}

	return ret;
}

// storage of the tooltip content.
#ifndef _DEBUG
constinit
#endif // !_DEBUG
static struct {
	std::wstring easing, values;
	int w, h, h2;

	// TODO: let this class handle most of the layout-draw jobs.
} tooltip_content{};

// color conversion.
constexpr auto bgr2rgb(int32_t c) {
	return (std::rotl<uint32_t>(c, 16) & 0x00ff00ff) | (c & 0x0000ff00);
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK param_button_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	if (*exedit.SettingDialogObjectIndex < 0) return ::DefSubclassProc(hwnd, message, wparam, lparam);

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
			switch (hdr->code) {
			case TTN_GETDISPINFOA:
			{
				// compose the tooltip string.
				size_t idx = static_cast<size_t>(data);
				auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
				auto const& mode = obj.track_mode[idx];
				if ((mode.num & 0x0f) == 0) break; // 移動無し
				if (settings.mode ||
					(settings.values.is_enabled() && !(
						obj.index_midpt_leader < 0 || // no mid-points.
						easing_name_spec(mode).spec.twopoints)))
					reinterpret_cast<NMTTDISPINFOA*>(lparam)->lpszText = const_cast<char*>(" ");
				break;
			}
			case TTN_SHOW:
			{
				// adjust the tooltip size to fit with the content.
				RECT rc;
				::GetWindowRect(tooltip, &rc);
				::SendMessageW(tooltip, TTM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rc));
				rc.right = rc.left + tooltip_content.w + 2; // add slight extra space on the right and bottom.
				rc.bottom = rc.top + tooltip_content.h + 1;
				::SendMessageW(tooltip, TTM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&rc));

				// adjust the position not to clip edges of screens.
				rc = sigma_lib::W32::monitor<true>{ rc.left, rc.top }
					.expand(-8).clamp(rc); // 8 pixels of padding.
				::SetWindowPos(tooltip, nullptr, rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return TRUE;
			}

			case NM_CUSTOMDRAW:
			{
				constexpr int gap_h = 4;
				constexpr UINT draw_text_options = DT_NOCLIP | DT_HIDEPREFIX;
				auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
				if (::IsWindowVisible(tooltip) == FALSE) {
					// prepare the tooltip content.

					size_t idx = static_cast<size_t>(data);
					auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
					auto const& mode = obj.track_mode[idx];
					easing_name_spec name_spec{ mode };

					// the name and desc of the easing.
					if (settings.mode) tooltip_content.easing = format_easing(mode, obj.track_param[idx], name_spec);

					// values at each midpoint.
					if (settings.values.is_enabled()) {
						if (obj.index_midpt_leader < 0 || // no mid-points.
							(mode.num & 0x0f) == 0 || // static value.
							name_spec.spec.twopoints)
							tooltip_content.values = L"";
						else {
							tooltip_content.values = formatted_values{ obj, idx }.span()
								.trim_from_sect(settings.values.left, settings.values.right)
								.to_string(exedit.trackinfo_left[idx].precision(), true, true, settings.values.zigzag);
						}
					}

					// measure those text.
					RECT rc1{}, rc2{};
					if (!tooltip_content.easing.empty())
						::DrawTextW(dhdr->nmcd.hdc, tooltip_content.easing.c_str(), tooltip_content.easing.size(),
							&rc1, DT_CALCRECT | draw_text_options);
					if (!tooltip_content.values.empty())
						::DrawTextW(dhdr->nmcd.hdc, tooltip_content.values.c_str(), tooltip_content.values.size(),
							&rc2, DT_CALCRECT | draw_text_options | DT_SINGLELINE);

					// layout the contents and detemine the size.
					tooltip_content.w = std::max(rc1.right - rc1.left, rc2.right - rc2.left);
					tooltip_content.h2 = rc1.bottom - rc1.top;
					if (!tooltip_content.easing.empty() && !tooltip_content.values.empty())
						tooltip_content.h2 += gap_h;
					tooltip_content.h = tooltip_content.h2 + (rc2.bottom - rc2.top);
				}
				else if (dhdr->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYPOSTPAINT;
				else {
					// change the text color if specified.
					if (settings.text_color >= 0)
						::SetTextColor(dhdr->nmcd.hdc, bgr2rgb(settings.text_color));

					// actual drawing, using content.easing and content.values.
					RECT rc{ dhdr->nmcd.rc };
					if (!tooltip_content.easing.empty())
						::DrawTextW(dhdr->nmcd.hdc, tooltip_content.easing.c_str(), tooltip_content.easing.size(),
							&rc, draw_text_options);
					if (!tooltip_content.values.empty()) {
						rc.top += tooltip_content.h2;
						::DrawTextW(dhdr->nmcd.hdc, tooltip_content.values.c_str(), tooltip_content.values.size(),
							&rc, draw_text_options | DT_SINGLELINE);
					}
				}
				break;
			}
			}
		}
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings::Tooltip;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.is_enabled()) {
		if (initializing) {
			// create the tooltip window.
			tooltip = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP |
				(settings.animation ? TTS_NOFADE | TTS_NOANIMATE : 0),
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				hwnd, nullptr, exedit.fp->hinst_parent, nullptr);

			::SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			// associate the tooltip with each trackbar button.
			TTTOOLINFOW ti{
				.cbSize = TTTOOLINFOW_V1_SIZE,
				.uFlags = TTF_IDISHWND | TTF_TRANSPARENT,
				.hinst = exedit.fp->hinst_parent,
				.lpszText = LPSTR_TEXTCALLBACKW,
			};
			for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++) {
				HWND tgt = exedit.hwnd_track_buttons[i];

				ti.hwnd = tgt;
				ti.uId = reinterpret_cast<uintptr_t>(tgt);

				::SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
				::SetWindowSubclass(tgt, &param_button_hook, hook_uid(), { i });
			}

			// settings of delay time for the tooltip.
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_INITIAL,	0xffff & settings.delay);
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP,	0xffff & settings.duration);
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_RESHOW,	0xffff & (settings.delay / 5));
			// initial values are... TTDT_INITIAL: 340, TTDT_AUTOPOP: 3400, TTDT_RESHOW: 68.
		}
		else {
			::DestroyWindow(tooltip);
			tooltip = nullptr;
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Easings.Tooltip";
	constexpr int min_vals = -1, max_vals = 100;
	constexpr int min_time = 0, max_time = 60'000;

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool,	mode);
	read(int,	values.left,	min_vals, max_vals);
	read(int,	values.right,	min_vals, max_vals);
	read(bool,	values.zigzag);

	read(bool,	animation);
	read(int,	delay,		min_time, max_time);
	read(int,	duration,	min_time, max_time);
	read(int,	text_color, -1, 0xffffff);

#undef read
}
