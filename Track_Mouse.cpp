/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <tuple>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "memory_protect.hpp"
#include "inifile_op.hpp"
#include "modkeys.hpp"

#include "reactive_dlg.hpp"
#include "TrackLabel.hpp"
#include "Track_Mouse.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの周りのマウス入力．
////////////////////////////////
using namespace reactive_dlg::Track;
using namespace reactive_dlg::Track::Mouse;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static constinit POINT prev_drag_pos{};
static constinit int drag_step_size = -1, step_progress = 0;
static constexpr bool settings_drag_is_enabled() { return drag_step_size >= 0; }
template<auto cursor_id>
static inline HCURSOR cursor_cache() {
	static constinit HCURSOR cache = nullptr;
	if (cache == nullptr)
		cache = ::LoadCursorW(nullptr, reinterpret_cast<PCWSTR>(cursor_id));
	return cache;
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK track_label_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	uint32_t const flags = static_cast<uint32_t>(data); // "waiting for right-button released" flag.
	switch (message) {
	case WM_MOUSEMOVE:
	{
		if (flags != 0) return 0; // let do nothing.

		POINT pt; ::GetCursorPos(&pt);
		if (pt.x == prev_drag_pos.x && pt.y == prev_drag_pos.y) break;
		mk::modkeys const mkeys{ (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, ::GetKeyState(VK_MENU) < 0 };

		// calculate the effective delta move.
		int delta = settings.drag.vertical ?
			prev_drag_pos.y - pt.y : pt.x - prev_drag_pos.x;
		if (settings.drag.reverse) delta *= -1;
		if (int co_step = (wparam & MK_RBUTTON) == 0 ? settings.drag.r_step_size : settings.drag.step_size;
			co_step < drag_step_size) {
			// "stepping" behavior of mouse drag.
			step_progress += co_step * delta;
			delta = step_progress / drag_step_size;
			step_progress -= delta * drag_step_size;
			if (step_progress < 0) {
				step_progress += drag_step_size;
				delta--;
			}
		}
		// boosting drag.
		if (settings.drag.boost(mkeys)) delta *= settings.drag.rate_boost;

		if (settings.drag.fixed) {
			// rewind the cursor position.
			::SetCursorPos(prev_drag_pos.x, prev_drag_pos.y);

			// hide the cursor.
			::SetCursor(nullptr);
		}
		else {
			// absorb the cursor move.
			delta -= pt.x - prev_drag_pos.x;
			prev_drag_pos = pt;
		}

		// "fake" the starting point of the drag.
		*exedit.track_label_start_drag_x -= delta;

		if (settings.drag.frac(mkeys) ^ mkeys.has_flags(mk::modkeys::shift)) {
			// "fake" the shift key too. 
			// ::GetKeyState(VK_SHIFT) takes effect, rather than (wparam & MK_SHIFT).
			auto const prev = set_key_state(VK_SHIFT, mkeys.has_flags(mk::modkeys::shift) ? 0 : 0x80);
			auto const ret = ::DefSubclassProc(hwnd, message, wparam, lparam);
			set_key_state(VK_SHIFT, prev);
			return ret;
		}
		break;
	}

	// suppress the context menu by WM_RBUTTONUP.
	case WM_LBUTTONUP:
		if (settings.drag.step_size != settings.drag.r_step_size) {
			if ((wparam & MK_RBUTTON) != 0) {
				if (flags == 0) {
					// set the flag to "wait for right-button to be released" state.
					::SetWindowSubclass(hwnd, &track_label_hook, id, 1);
					::SetCursor(cursor_cache<IDC_ARROW>());
				}
				return 0; // keep capturing the mouse.
			}
		}
		break;
	case WM_RBUTTONUP:
		if (settings.drag.step_size != settings.drag.r_step_size) {
			if (flags != 0 && (wparam & MK_LBUTTON) == 0) {
				message = WM_LBUTTONUP; // let process as a left-up message.
				break;
			}
			return 0; // suppress the context menu.
		}
		break;

	case WM_CAPTURECHANGED:
		// ignore when re-capturing the same window.
		if (hwnd == reinterpret_cast<HWND>(lparam)) break;
		[[fallthrough]];
	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &track_label_hook, id);
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
			case EN_SETFOCUS:
				if (settings_drag_is_enabled() && find_trackinfo(wparam & 0xffff, ctrl) != nullptr &&
					::GetCapture() == ctrl) {
					// hook for tweaked drag behavior.
					::GetCursorPos(&prev_drag_pos);
					step_progress = drag_step_size >> 1;

					::SetWindowSubclass(ctrl, &track_label_hook, hook_uid(), 0);
					/* known issue:
					* when tab key is pressed while dragging to move the focus,
					* and the next time the user initiates the drag, the dragged
					* edit control will be the focused one wherever the mouse position is,
					* and the hook by this plugin won't take place this time.
					*/
				}
				break;
			}
		}
		break;

	case WM_MOUSEWHEEL:
		// control values by mouse wheel.
		if (settings.wheel.enabled && *exedit.SettingDialogObjectIndex >= 0) {
			mk::modkeys keys{ (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, key_pressed_any(VK_MENU) };

			// check for the activation key.
			if (keys >= settings.wheel.keys_activate &&
				settings.wheel.no_wrong_keys(keys) && !key_pressed_any(VK_LWIN, VK_RWIN)) {

				// find the pointer position.
				POINT pt = { static_cast<int16_t>(lparam & 0xffff), static_cast<int16_t>(lparam >> 16) };
				::ScreenToClient(hwnd, &pt);

				// find the TrackInfo to manipulate.
				TrackInfo* info = nullptr;
				if (HWND label; *exedit.track_label_is_dragging != 0 && (label = ::GetCapture()) != nullptr)
					info = find_trackinfo(label);
				if (info == nullptr) info = find_trackinfo(pt);

				// if the track is found, make an attempt to move the value.
				if (info != nullptr) {
					int const wheel = static_cast<int16_t>(wparam >> 16);

					// determine the delta value taking the modifier keys in account.
					int const delta = ((wheel > 0) ^ settings.wheel.reverse ? +1 : -1)
						* settings.wheel.calc_rate(keys, info->precision());

					// calculate the resulting value.
					auto val = std::clamp(info->val_int_curr + delta * info->delta, info->val_int_min, info->val_int_max);
					if (val != info->val_int_curr) {
						// rescale val to suit with the updown control.
						val /= info->delta;

						// send the new value.
						::SendMessageW(info->hwnd_updown, UDM_SETPOS32, 0, static_cast<LPARAM>(val));
						::SendMessageW(hwnd, WM_HSCROLL, (val << 16) | SB_THUMBPOSITION, reinterpret_cast<LPARAM>(info->hwnd_updown));

						return 0; // successfully moved the value.
					}
				}
			}
		}
		break;
	case WM_SETCURSOR:
		// change mouse cursor when wheeling is ready.
		if (settings.wheel.enabled && settings.wheel.cursor_react &&
			*exedit.SettingDialogObjectIndex >= 0) {
			// check if it's not in a dragging state.
			if (*exedit.track_label_is_dragging != 0 && ::GetCapture() != nullptr)
				return TRUE;

			// check for the activation key.
			auto keys = curr_modkeys();
			if (keys >= settings.wheel.keys_activate &&
				settings.wheel.no_wrong_keys(keys) && !key_pressed_any(VK_LWIN, VK_RWIN)) {
				// find the pointer position.
				POINT pt;
				::GetCursorPos(&pt);
				::ScreenToClient(hwnd, &pt);

				// if the track is found, change the cursor to hand.
				if (find_trackinfo(pt) != nullptr) {
					::SetCursor(cursor_cache<IDC_HAND>());
					return TRUE;
				}
			}
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &setting_dlg_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

static inline LRESULT CALLBACK trackbar_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_MOUSEWHEEL:
	{
		mk::modkeys keys = { (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, key_pressed_any(VK_MENU) };

		// as trackbars process wheel messages by their own,
		// the message doesn't propagate to the parent. it should be hooked.
		if (keys >= settings.wheel.keys_activate &&
			settings.wheel.no_wrong_keys(keys) && !key_pressed_any(VK_LWIN, VK_RWIN))
			return ::SendMessageW(*exedit.hwnd_setting_dlg, message, wparam, lparam);
		break;
	}

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &trackbar_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

// カーソル切り替えのためのキーボード監視用フック．
static constinit HHOOK keyboard_hook_handle = nullptr;
static inline LRESULT CALLBACK keyboard_hook(int code, WPARAM wparam, LPARAM lparam)
{
	constexpr auto is_target_key = [](WPARAM vk) -> bool {
		switch (vk) {
		case VK_CONTROL:
		case VK_SHIFT:
		case VK_MENU:
		case VK_LWIN:
		case VK_RWIN: return true;
		default: return false;
		}
	};

	if (code == HC_ACTION &&
		(((lparam << 1) ^ lparam) & (1u << 31)) == 0 && // handle only new presses or new releases.
		is_target_key(wparam)) {

		// check if the cursor lies on the setting dialog.
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (dlg != nullptr) {
			POINT pt;
			::GetCursorPos(&pt);
			auto const hwnd = ::WindowFromPoint(pt);
			if (hwnd != nullptr && (dlg == hwnd || ::IsChild(dlg, hwnd))) {
				// post a message to determine the cursor shape.
				::PostMessageW(hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), HTCLIENT | (WM_MOUSEMOVE << 16));
			}
		}
	}
	return ::CallNextHookEx(keyboard_hook_handle, code, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Track::Mouse;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.wheel.enabled || settings.drag.is_enabled()) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			::SetWindowSubclass(dlg, &setting_dlg_hook, hook_uid(), {});

			if (settings.wheel.enabled) {
				for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++) {
					::SetWindowSubclass(exedit.trackinfo_left[i].hwnd_track, &trackbar_hook, hook_uid(), {});
					::SetWindowSubclass(exedit.trackinfo_right[i].hwnd_track, &trackbar_hook, hook_uid(), {});
				}

				// hooking keyboard inputs to change the cursor shape when certain keys are pressed.
				if (settings.wheel.cursor_react &&
					settings.wheel.keys_activate.has_flags() &&
					keyboard_hook_handle == nullptr) {
					keyboard_hook_handle = ::SetWindowsHookExW(WH_KEYBOARD, &keyboard_hook, nullptr, ::GetCurrentThreadId());
				}
			}

			if (settings.drag.vertical) {
				// 68 84 7F 00 00       push 7F84h  // IDC_SIZEWE
				// V
				// 68 85 7F 00 00       push 7F85h  // IDC_SIZENS
				sigma_lib::memory::ProtectHelper::write(exedit.push_cursor_track_drag + 1, IDC_SIZENS);
			}

			// works as a "cached value" of settings.drag.is_enabled().
			drag_step_size = settings.drag.is_enabled() ?
				settings.drag.step_size * settings.drag.r_step_size : -1;
		}
		else {
			::RemoveWindowSubclass(dlg, &setting_dlg_hook, hook_uid());
			if (keyboard_hook_handle != nullptr) {
				::UnhookWindowsHookEx(keyboard_hook_handle);
				keyboard_hook_handle = nullptr;
			}
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Track.Mouse";
	constexpr int32_t min_drag_step_size = 1, max_drag_step_size = 200;

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(modkey,wheel.keys_frac);
	read(modkey,wheel.keys_boost);
	read(bool,	wheel.def_frac);
	read(int,	wheel.rate_boost, wheel.min_rate_boost, wheel.max_rate_boost);
	read(bool,	wheel.enabled);
	read(bool,	wheel.reverse);
	read(bool,	wheel.cursor_react);
	read(modkey,wheel.keys_activate);

	read(modkey,drag.keys_frac);
	read(modkey,drag.keys_boost);
	read(bool,	drag.def_frac);
	read(int,	drag.rate_boost, drag.min_rate_boost, drag.max_rate_boost);
	read(bool,	drag.fixed);
	read(bool,	drag.vertical);
	read(bool,	drag.reverse);
	read(int,	drag.step_size, min_drag_step_size, max_drag_step_size);
	read(int,	drag.r_step_size, min_drag_step_size, max_drag_step_size);

#undef read
}
