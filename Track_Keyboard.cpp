/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cmath>
#include <cwchar>

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
#include "Track_Keyboard.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの数値入力ボックス操作．
////////////////////////////////
using namespace reactive_dlg::Track;
using namespace reactive_dlg::Track::Keyboard;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static constinit TrackInfo const* curr_info = nullptr;
static constinit wchar_t last_text[std::bit_ceil(TrackInfo::max_value_len + 1)] = L"";

static inline bool modify_number_core(auto& text, int len, double val, bool clamp)
{
	// doesn't clamp val into the min/max range by default,
	// as this is an edit control so the user may want to edit it afterward.
	if (clamp) val = std::clamp(val, curr_info->value_min(), curr_info->value_max());

	// track the position of the decimal point.
	int dec_diff = len;
	if (auto pt = std::wcschr(text, L'.'); pt != nullptr)
		dec_diff = pt - text; // text have fraction part.

	// find the number of digits below the decimal point.
	len = std::lroundf(std::log10f(static_cast<float>(std::max(curr_info->precision(), 1))));
	if (len > 0) dec_diff += 1 + len;

	// format the number.
	len = ::swprintf_s(text, L"%.*f", len, val);
	if (len <= 0) return false;
	dec_diff -= len;

	// get the current selection, and adjust it
	// so the relative position to the decimal point is preserved.
	auto [sel_l, sel_r] = get_edit_selection(curr_info->hwnd_label);
	sel_l -= dec_diff; sel_r -= dec_diff;

	// apply those changes.
	::SetWindowTextW(curr_info->hwnd_label, text);

	// restore the selection.
	set_edit_selection(curr_info->hwnd_label, std::clamp(sel_l, 0, len), std::clamp(sel_r, 0, len));

	return true;
}

static inline bool modify_number(auto&& modify, bool clamp)
{
	wchar_t text[std::size(last_text)];
	auto len = ::GetWindowTextW(curr_info->hwnd_label, text, std::size(text));
	if (len >= static_cast<int>(std::size(text)) - 1) return false;

	// parse the string as a number.
	double val;
	if (wchar_t* e; (val = std::wcstod(text, &e)) == HUGE_VAL ||
		(*e != L'\0' && *e != L' ')) return false;

	// apply the modification.
	val = modify(val);

	return modify_number_core(text, len, val, clamp);
}

decltype(::TrackPopupMenu) popup_easing_menu_hook;
static constinit auto* popup_easing_menu_hook_addr = &popup_easing_menu_hook;
static constinit auto const* popup_easing_menu = &popup_easing_menu_hook_addr;

static constinit struct {
	uint16_t idx = ~0;
	byte alt_state = 0;
	constexpr bool is_active() const { return idx < ExEdit::Object::MAX_TRACK; }
	constexpr void activate(size_t idx) { this->idx = static_cast<decltype(this->idx)>(idx); }
	constexpr void deactivate() { idx = ~0; alt_state = 0; }
} easing_menu;


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK track_label_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	constexpr auto discard_char_msg = [](HWND hwnd, UINT this_msg) {
		constexpr int diff = WM_CHAR - WM_KEYDOWN;
		static_assert(diff == WM_SYSCHAR - WM_SYSKEYDOWN);
		discard_message(hwnd, this_msg + diff);
	};

	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		// "updown" functionality.
		byte vkey = static_cast<byte>(wparam);
		if (settings.updown.enabled && (vkey == VK_UP || vkey == VK_DOWN)) {
			auto const keys = curr_modkeys();
			if (settings.updown.no_wrong_keys(keys) && !key_pressed_any(VK_LWIN, VK_RWIN)) {
				int const prec = curr_info->precision();
				double const delta = (vkey == VK_UP ? +1.0 : -1.0)
					* settings.updown.calc_rate(keys, prec) / prec;
				if (modify_number([delta](double x) { return x + delta; }, settings.updown.clamp))
					// prevent calling the default window procedure.
					return 0;
			}
		}

		// "negate" functionality.
		if (settings.negate.is_enabled() && settings.negate.vkey == vkey &&
			settings.negate.mkeys == curr_modkeys() && !key_pressed_any(VK_LWIN, VK_RWIN) &&
			modify_number([](double x) {return -x; }, settings.negate.clamp)) {
			// needs to remove WM_CHAR or WM_SYSCHAR messages
			// to avoid notification sounds or unnecessary inputs.
			discard_char_msg(hwnd, message);
			return 0;
		}

		// "escape" functionality.
		if (settings.escape && vkey == VK_ESCAPE &&
			!key_pressed_any(VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN)) {

			wchar_t text[std::size(last_text) + 1];
			auto const len = ::GetWindowTextW(curr_info->hwnd_label, text, std::size(text));

			if (len < static_cast<int>(std::size(text)) - 1 && std::wcscmp(text, last_text) == 0) {
				// the content didn't change. move the focus form this label.
				::SetFocus(exedit.fp->hwnd);
			}
			else {
				// worth reverting.
				::SetWindowTextW(curr_info->hwnd_label, last_text);
				set_edit_selection_all(curr_info->hwnd_label);
			}

			// needs to remove WM_CHAR or WM_SYSCHAR messages
			// to avoid notification sounds or unnecessary inputs.
			discard_char_msg(hwnd, message);
			return 0;
		}

		// "easing_menu" functionality.
		if (settings.easing_menu.is_enabled() && settings.easing_menu.vkey == vkey &&
			settings.easing_menu.mkeys == curr_modkeys() && !key_pressed_any(VK_LWIN, VK_RWIN)) {
			// suppress notification sound by discarding certain messages.
			discard_char_msg(hwnd, message);

			// save the index so it the hook is enabled.
			size_t const idx = static_cast<size_t>(data);
			easing_menu.activate(idx);

			// backup the current text and the selection.
			auto [l_pos, r_pos] = get_edit_selection(hwnd);
			wchar_t curr_text[std::size(last_text)];
			::GetWindowTextW(hwnd, curr_text, std::size(curr_text));

			// move off the focus form this edit box,
			// otherwise it would swallow the input of Enter key.
			::SetFocus(*exedit.hwnd_setting_dlg);

			// send the button-click notification.
			if (settings.easing_menu.mkeys.has_flags(mk::modkeys::alt))
				// as alt+clicking the button has a different functionality,
				// make the key recognized as released.
				easing_menu.alt_state = set_key_state(VK_MENU, 0);
			::SendMessageW(*exedit.hwnd_setting_dlg, WM_COMMAND,
				(BN_CLICKED << 16) | button_ctrl_id(idx),
				reinterpret_cast<LPARAM>(exedit.hwnd_track_buttons[idx]));

			// rewind the focus and other states.
			if ((::GetWindowLongW(hwnd, GWL_STYLE) & WS_DISABLED) == 0) {
				::SetFocus(hwnd);
				::SetWindowTextW(hwnd, curr_text);
				set_edit_selection(hwnd, l_pos, r_pos);

				// turn out of the hooking state.
				easing_menu.deactivate();
			}
			else {
				// if it's right side and disabled now, focus on left instead.
				auto left_one = exedit.trackinfo_left[idx].hwnd_label;

				easing_menu.deactivate(); // no more hooks.
				::SetFocus(left_one);
				set_edit_selection_all(left_one);
			}

			return 0;
		}
		break;
	}

	case WM_KILLFOCUS:
		curr_info = nullptr;
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
				if (TrackInfo const* info = find_trackinfo(wparam & 0xffff, ctrl);
					info != nullptr) {
					// hook the edit box control.
					curr_info = info;
					if (!easing_menu.is_active())
						::GetWindowTextW(curr_info->hwnd_label, last_text, std::size(last_text));
					::SetWindowSubclass(ctrl, &track_label_hook, hook_uid(), static_cast<DWORD_PTR>(info_index(info)));
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

// トラックバー変化方法のメニューをショートカットキーで表示させたときの座標調整．
static inline BOOL WINAPI popup_easing_menu_hook(HMENU menu, UINT flags, int x, int y, int nReserved, HWND hwnd, RECT const* prcRect)
{
	if (easing_menu.is_active()) {
		auto const btn = exedit.hwnd_track_buttons[easing_menu.idx];

		// Alt の状態を差し戻す．
		set_key_state(VK_MENU, easing_menu.alt_state);

		// メニューの位置をボタンの矩形に合わせて調整．
		TPMPARAMS p{ .cbSize = sizeof(p) };
		::GetWindowRect(btn, &p.rcExclude);

		if (*popup_easing_menu == &::TrackPopupMenu) {
			// メニュー表示．
			return ::TrackPopupMenuEx(menu, flags | TPM_VERTICAL,
				p.rcExclude.left, p.rcExclude.top, hwnd, &p);
		}
		else {
			x = p.rcExclude.left; y = p.rcExclude.bottom;

			// この関数以前に同じ関数がフックされているときは元の関数を呼び出す．
		}
	}

	// 条件を満たさない場合は未介入のデフォルト処理．
	return (*popup_easing_menu)(menu, flags, x, y, nReserved, hwnd, prcRect);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Track::Keyboard;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.updown.enabled || settings.negate.is_enabled() ||
		settings.escape || settings.easing_menu.is_enabled()) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			// トラックバー変化方法のメニューをショートカットキーで表示させたときの座標調整．
			if (settings.easing_menu.is_enabled())
				// ff 15 xx xx xx xx	call	dword ptr ds:[TrackPopupMenu]
				// V
				// ff 15 yy yy yy yy	call	dword ptr ds:[popup_easing_menu_hook]
				popup_easing_menu = sigma_lib::memory::replace_api_call(
					exedit.call_easing_popup_menu, &popup_easing_menu_hook_addr);

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

	constexpr auto section = "Track.Keyboard";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(modkey,updown.keys_frac);
	read(modkey,updown.keys_boost);
	read(bool,	updown.def_frac);
	read(int,	updown.rate_boost, updown.min_rate_boost, updown.max_rate_boost);
	read(bool,	updown.enabled);
	read(bool,	updown.clamp);

	read(int,	negate.vkey);
	read(modkey,negate.mkeys);
	read(bool,	negate.clamp);

	read(bool,	escape);

	read(int,	easing_menu.vkey);
	read(modkey,easing_menu.mkeys);

#undef read
}
