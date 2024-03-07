/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <string>
#include <memory>
#include <bit>
#include <concepts>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

using namespace AviUtl;
using namespace ExEdit;

#include "modkeys.hpp"

////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
struct TrackInfo {
	int32_t	val_int_curr;	// current value in the internal scale.
	int32_t	val_int_min;	// minimum value in the internal scale.
	int32_t	val_int_max;	// maximum value in the internal scale.
	int32_t	denom;			// ratio between the internal and displayed values; can be 0, which is recognized as 1. always 100 for scripts. (possible values: 0, 1, 10 and 100.)
	int32_t	val_int_def;	// default value in the internal scale.
	HWND	hwnd_track;		// the window handle to the trackbar control.
	HWND	hwnd_label;		// the window handle to the edit control.
	HWND	hwnd_updown;	// the window handle to the updown control.
	int32_t	pos_v;			// vertical position of the group of the trackbar UIs in pixels. 0 if hidden.
	int32_t	delta;			// the smallest distance between possible internal values. can be > 1 only if it's for scripts. (possible values: 1, 10 and 100.)

	// not hidden such as by folding or unassigned to any value in the current object.
	constexpr bool is_visible() const { return pos_v != 0; }
	// calculates the displayed value from an internal value.
	constexpr double calc_value(int val_int) const { return val_int / static_cast<double>(denominator()); }
	// current displayed value.
	constexpr auto value() const { return calc_value(val_int_curr); }
	// minimum displayed value.
	constexpr auto value_min() const { return calc_value(val_int_min); }
	// maximum displayed value.
	constexpr auto value_max() const { return calc_value(val_int_max); }
	// default displayed value.
	constexpr auto value_def() const { return calc_value(val_int_def); }
	// ratio between the internal and displayed values.
	constexpr int denominator() const { return denom != 0 ? denom : 1; }
	// reciprocal of the smallest distance between the displayed values.
	constexpr int precision() const { return denominator() / (delta != 0 ? delta : 1); }

	// all the fields are initialized to zero at startup,
	// and won't contain valid values until they're once used,
	// which is the only case delta is zero.
	constexpr bool has_valid_values() const { return delta != 0; }

	// smallest distance between pos_v's, representing the heights of the group of trackbar UIs.
	constexpr static int TrackHeight = 25;
};
static_assert(sizeof(TrackInfo) == 40);

inline constinit struct ExEdit092 {
	FilterPlugin* fp;
	constexpr static const char* info_exedit092 = "拡張編集(exedit) version 0.92 by ＫＥＮくん";
	bool init(FilterPlugin* this_fp)
	{
		if (fp != nullptr) return true;
		SysInfo si; this_fp->exfunc->get_sys_info(nullptr, &si);
		for (int i = 0; i < si.filter_n; i++) {
			auto that_fp = this_fp->exfunc->get_filterp(i);
			if (that_fp->information != nullptr &&
				0 == std::strcmp(that_fp->information, info_exedit092)) {
				fp = that_fp;
				init_pointers();
				return true;
			}
		}
		return false;
	}

	Object**	ObjectArray_ptr;			// 0x1e0fa4
	int32_t*	SettingDialogObjectIndex;	// 0x177a10
	HWND*		hwnd_setting_dlg;			// 0x1539c8
	//int32_t*	is_playing;					// 0x1a52ec; 0: editing, 1: playing.
	HWND*		hwnd_edit_text;				// 0x236328
	HWND*		hwnd_edit_script;			// 0x230C78
	TrackInfo*	trackinfo_left;				// 0x14d4c8
	TrackInfo*	trackinfo_right;			// 0x14def0

	//WNDPROC		setting_dlg_wndproc;		// 0x02cde0

private:
	void init_pointers()
	{
		auto pick_addr = [exedit_base=reinterpret_cast<uintptr_t>(fp->dll_hinst)]
			<class T>(T& target, ptrdiff_t offset) { target = reinterpret_cast<T>(exedit_base + offset); };
		pick_addr(ObjectArray_ptr,			0x1e0fa4);
		pick_addr(SettingDialogObjectIndex,	0x177a10);
		pick_addr(hwnd_setting_dlg,			0x1539c8);
		//pick_addr(is_playing,				0x1a52ec);
		pick_addr(hwnd_edit_text,			0x236328);
		pick_addr(hwnd_edit_script,			0x230C78);
		pick_addr(trackinfo_left,			0x14d4c8);
		pick_addr(trackinfo_right,			0x14def0);

		//pick_addr(setting_dlg_wndproc,		0x02cde0);
	}
} exedit;


////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////
template<int N>
bool check_window_class(HWND hwnd, const wchar_t(&classname)[N])
{
	wchar_t buf[N + 1];
	return ::GetClassNameW(hwnd, buf, N + 1) == N - 1
		&& std::memcmp(buf, classname, sizeof(wchar_t) * (N - 1)) == 0;
}

inline auto get_edit_selection(HWND edit_box)
{
	int32_t st, ed;
	::SendMessageW(edit_box, EM_GETSEL, reinterpret_cast<WPARAM>(&st), reinterpret_cast<LPARAM>(&ed));
	return std::pair{ st, ed };
}
inline void set_edit_selection(HWND edit_box, int start, int end) {
	::SendMessageW(edit_box, EM_SETSEL, static_cast<WPARAM>(start), static_cast<LPARAM>(end));
}
inline void set_edit_caret(HWND edit_box, int pos) { set_edit_selection(edit_box, pos, pos); }
inline void set_edit_selection_all(HWND edit_box) { set_edit_selection(edit_box, 0, -1); }

inline modkeys curr_modkeys() {
	return { ::GetKeyState(VK_CONTROL) < 0, ::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_MENU) < 0 };
}
inline bool key_pressed_any(auto... vkeys) {
	return ((::GetKeyState(vkeys) < 0) || ...);
}

inline void discard_message(HWND hwnd, UINT message) {
	MSG msg;
	while (::PeekMessageW(&msg, hwnd, message, message, PM_REMOVE));
}


////////////////////////////////
// 文字エンコード変換．
////////////////////////////////
struct Encodes {
	template<UINT codepage = CP_UTF8>
	static std::wstring to_wstring(const std::string_view& src) {
		if (src.length() == 0) return L"";

		auto wlen = ::MultiByteToWideChar(codepage, 0, src.data(), src.length(), nullptr, 0);
		std::wstring ret(wlen, L'\0');
		::MultiByteToWideChar(codepage, 0, src.data(), src.length(), ret.data(), wlen);

		return ret;
	}
};


////////////////////////////////
// 設定ダイアログ関連．
////////////////////////////////
struct SettingDlg {
	static HWND get_hwnd() { return *exedit.hwnd_setting_dlg; }
	static bool is_valid() { return get_hwnd() != nullptr; }
	static uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&exedit); }
};

// テキストボックス検索/操作．
struct TextBox : SettingDlg {
	static uintptr_t hook_uid() { return SettingDlg::hook_uid() + 1; }

	static bool check_edit_box_style(HWND edit)
	{
		// check whether ...
		// - it allows multiple lines,
		// - it accepts return,
		// - it's enabled, and
		// - it's visible.
		constexpr uint32_t
			required = ES_MULTILINE | ES_WANTRETURN | WS_VISIBLE,
			rejected = WS_DISABLED;
		auto style = ::GetWindowLongW(edit, GWL_STYLE);
		return (style & (required | rejected)) == required;
	}

	enum class kind : uint8_t { unspecified, text, script };
	static kind edit_box_kind(HWND hwnd) {
		if (hwnd == nullptr) return kind::unspecified; // in case the following two are not created.
		if (hwnd == *exedit.hwnd_edit_text) return kind::text;
		if (hwnd == *exedit.hwnd_edit_script) return kind::script;
		return kind::unspecified;
	}

	// finds the next edit box for the focus target.
	static HWND next_edit_box(HWND curr, bool forward)
	{
		if (!is_valid() || *exedit.SettingDialogObjectIndex < 0) return nullptr;

		HWND edits[]{ *exedit.hwnd_edit_text, *exedit.hwnd_edit_script };
		if (!forward) std::reverse(std::begin(edits), std::end(edits));

		auto next = std::end(edits);
		while (--next >= std::begin(edits) && !(*next != nullptr && *next == curr));
		while (++next < std::end(edits)) {
			if (*next != nullptr && check_edit_box_style(*next)) return *next;
		}
		return nullptr;
	}

public:
	// テキストボックスの TAB 文字サイズ．
	static void set_tabstops(HWND hwnd, const auto& choose)
	{
		auto kind = edit_box_kind(hwnd);
		if (kind == kind::unspecified) return;
		if (auto value = choose(kind); value >= 0)
			::SendMessageW(hwnd, EM_SETTABSTOPS, { 1 }, reinterpret_cast<LPARAM>(&value));
	}

	// テキスト入力の画面反映処理を一括する機能．
	inline static constinit struct {
		void operator()()
		{
			// ignore unexpected calls.
			if (!posted) return;
			posted = false;

			batch_call = true;
			// calling exedit.setting_dlg_wndproc() would be faster,
			// but might ignore hooks from other plugins.
			::SendMessageW(hwnd, message, wparam, lparam);
			batch_call = false;
		}
		bool is_batch_call() const { return batch_call; }
		void post(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, HWND hwnd_host, UINT message_host)
		{
			// update message parameters.
			this->hwnd = hwnd;
			this->message = message;
			this->wparam = wparam;
			this->lparam = lparam;

			// post a message if not yet.
			if (posted) return;
			posted = true;
			::PostMessageW(hwnd_host, message_host, {}, {});
		}
		void discard(HWND hwnd_host, UINT message_host)
		{
			if (!posted) return;
			posted = false;
			discard_message(hwnd_host, message_host);
		}

	private:
		// indicates if delaying call was already posted.
		bool posted = false;

		// indicates if it's a delayed batch call.
		bool batch_call = false;

		// message paramters.
		HWND hwnd{};
		UINT message{};
		WPARAM wparam{};
		LPARAM lparam{};
	} batch;

	// テキスト入力中にカーソルを隠す機能．
	static inline constinit struct {
	private:
		static constexpr auto dist(const POINT& p1, const POINT& p2) {
			// so-called L^\infty-distance.
			return std::max(std::abs(p1.x - p2.x), std::abs(p1.y - p2.y));
		}
		bool hide = false;
		POINT pos{};

	public:
		bool is_hidden() const { return hide; }
		void reset() { hide = false; }
		void on_edit(const POINT& pt) { hide = true; pos = pt; }
		void on_move(const POINT& pt) {
			constexpr int move_threshold = 8;
			if (!hide || dist(pt, pos) <= move_threshold) return;
			hide = false;
		}
	} hide_cursor;
};

// トラックバーの数値入力ボックス検索/操作．
struct TrackLabel : SettingDlg {
private:
	// finding edit boxes for trackbars.
	constexpr static int id_label_left_base = 4100, id_label_right_base = 4200;

public:
	static uintptr_t hook_uid() { return SettingDlg::hook_uid() + 2; }

	static std::pair<bool, int8_t> check_edit_box_id(uint32_t id)
	{
		bool left = id < id_label_right_base;
		id -= left ? id_label_left_base : id_label_right_base;
		if (id >= Object::MAX_TRACK) return { false, -1 };
		return { left , id };
	}
	static bool check_edit_box_style(HWND edit)
	{
		// check whether ...
		// - it's center-aligned,
		// - it doesn't allow multiple lines,
		// - it doesn't accept return,
		// - it's enabled, and
		// - it's visible.
		constexpr uint32_t
			required = ES_CENTER | WS_VISIBLE,
			rejected = ES_MULTILINE | ES_WANTRETURN | WS_DISABLED;
		auto style = ::GetWindowLongW(edit, GWL_STYLE);
		return (style & (required | rejected)) == required;
	}
	static TrackInfo* find_trackinfo(HWND hwnd)
	{
		if (hwnd == nullptr) return nullptr;
		return find_trackinfo(::GetDlgCtrlID(hwnd), hwnd);
	}
	static TrackInfo* find_trackinfo(uint32_t id, HWND hwnd)
	{
		auto [left, idx] = check_edit_box_id(id);
		if (idx < 0) return nullptr;
		auto& info = (left ? exedit.trackinfo_left : exedit.trackinfo_right)[idx];
		if (hwnd != info.hwnd_label || !check_edit_box_style(hwnd)) return nullptr;
		return &info;
	}
	static TrackInfo* find_trackinfo(POINT pt)
	{
		constexpr int mid_gap = 64;
		constexpr int side_pad = 8;
		constexpr int upper_pad = 0, lower_pad = 4;

		if (!is_valid() || *exedit.SettingDialogObjectIndex < 0) return nullptr;
		const auto track_n = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex].track_n;

		// determine whether the pointer is on the left/right or the middle gap.
		bool left = true;
		if (pt.x < side_pad) return nullptr; // too left.
		if (int wid = [] { RECT rc; ::GetClientRect(get_hwnd(), &rc); return rc.right; }();
			pt.x >= (wid >> 1) - mid_gap / 2) {
			if (pt.x < (wid >> 1) + mid_gap / 2) return nullptr; // in the middle gap.
			if (pt.x >= wid - side_pad) return nullptr; // too right.
			left = false;
		}

		// find the suitable track index for the pointer position.
		auto* info = left ? exedit.trackinfo_left : exedit.trackinfo_right;
		for (auto* const info_end = info + track_n; info < info_end; info++) {
			if (info->is_visible() &&
				info->pos_v + upper_pad <= pt.y &&
				pt.y < info->pos_v + TrackInfo::TrackHeight - lower_pad &&
				TrackLabel::check_edit_box_style(info->hwnd_label))
				return info;
		}
		return nullptr;
	}

	static TrackInfo* first_trackinfo(bool forward)
	{
		if (!is_valid() || *exedit.SettingDialogObjectIndex < 0) return nullptr;
		const auto& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
		const auto track_n = obj.track_n;
		int track_o = 0;

		// needs re-ordering the indices if the final filter is of the certain types.
		switch (const auto& filter_last = obj.filter_param[obj.countFilters() - 1]; filter_last.id) {
			enum id : decltype(filter_last.id) {
				draw_std = 0x0a, // 標準描画
				draw_ext = 0x0b, // 拡張描画
				play_std = 0x0c, // 標準再生
				particle = 0x0d, // パーティクル出力
			};
		case draw_std:
		case draw_ext:
		case play_std:
		case particle:
			track_o = filter_last.track_begin; // alter offset.
			break;
		}

		for (int j = 0; j < track_n; j++) {
			// find the index of the track.
			int const i = ((forward ? j : track_n - 1 - j)
				+ track_o) % track_n;

			// the loop for left/right.
			for (uint_fast8_t k = 0; k < 2; k++) {
				bool const left = (k > 0) ^ forward;

				// get the corresponding info.
				auto& info = (left ? exedit.trackinfo_left : exedit.trackinfo_right)[i];

				// check if it's not disabled.
				if (info.hwnd_label == nullptr || !check_edit_box_style(info.hwnd_label)) continue;
				return &info;
			}
		}
		return nullptr;
	}

private:
	// manipulation of the currently selected trackbar.
	static inline constinit const TrackInfo* info = nullptr;
	static inline constinit wchar_t last_text[15] = L"";

public:
	static const TrackInfo& curr_info() { return *info; }
	static void on_setfocus(const TrackInfo* track_info)
	{
		info = track_info;
		::GetWindowTextW(track_info->hwnd_label, last_text, std::size(last_text));
	}
	static void on_killfocus() { info = nullptr; }
	static bool is_focused() { return info != nullptr; }

	static bool add_number(int delta, bool clamp)
	{
		wchar_t text[std::size(last_text)];
		auto len = ::GetWindowTextW(info->hwnd_label, text, std::size(text));
		if (len >= static_cast<int>(std::size(text)) - 1) return false;

		// parse the string as a number.
		double val;
		if (wchar_t* e; (val = std::wcstod(text, &e)) == HUGE_VAL ||
			(*e != L'\0' && *e != L' ')) return false;

		auto prec = info->precision();
		val += static_cast<double>(delta) / prec;

		// doesn't clamp val into the min/max range by default,
		// as this is an edit control so the user may want to edit it afterward.
		if (clamp) val = std::clamp(val, info->value_min(), info->value_max());

		// get the caret position, relative to the end of the text.
		auto caret = len - get_edit_selection(info->hwnd_label).first;
		if (prec > 1) {
			int dec_places = prec < 100 ? 1 : 2;

			// values have fraction part.
			// preserve the caret position relative to the decimal point.
			if (auto pt = std::wcschr(text, L'.'); pt != nullptr)
				caret -= (text + len) - pt;
			caret += 1 + dec_places;

			// also, we need to prepare the formatting string according to the precision.
			wchar_t fmt[] = L"%.0f"; fmt[2] = L'0' + dec_places;
			len = std::swprintf(text, std::size(text), fmt, val);
		}
		else {
			// values are integers.
			len = std::swprintf(text, std::size(text), L"%d", static_cast<int>(val));
		}
		if (len <= 0) return false;

		// apply those changes.
		::SetWindowTextW(info->hwnd_label, text);

		// restore the caret position preserving the relative position to the decimal point.
		set_edit_caret(info->hwnd_label, std::clamp(len - caret, 0, len));

		return true;
	}

	static bool revert()
	{
		wchar_t text[std::size(last_text) + 1];
		auto len = ::GetWindowTextW(info->hwnd_label, text, std::size(text));

		if (len < static_cast<int>(std::size(text)) - 1 && std::wcscmp(text, last_text) == 0)
			return false; // the content didn't change.

		// worth reverting.
		::SetWindowTextW(info->hwnd_label, last_text);
		set_edit_selection_all(info->hwnd_label);
		return true;
	}

	// ホイール操作時のカーソルのハンドルキャッシュ．
	// 将来的にはユーザ指定のファイルを読み込んでもいいかも．
	static inline constinit struct {
		HCURSOR get()
		{
			if (handle == nullptr)
				handle = ::LoadCursorW(nullptr, reinterpret_cast<PCWSTR>(IDC_HAND));
			return handle;
		}
		//HCURSOR get(const auto& info)
		//{
		//	if (handle == nullptr) {
		//		if (info.file != nullptr) {
		//			handle = ::LoadCursorFromFileW(info.file);
		//			info.id = reinterpret_cast<PCWSTR>(IDC_HAND); // fallback.
		//		}
		//		if (handle == nullptr) {
		//			shared = true;
		//			handle = ::LoadCursorW(nullptr, info.id);
		//		}
		//	}
		//	return handle;
		//}
		void free() {
			//if (handle != nullptr) {
			//	if (!shared) ::DestroyCursor(handle);
			//	handle = nullptr;
			//	shared = false;
			//}
		}

	private:
		HCURSOR handle = nullptr;
		//bool shared = false;
	} cursor;
};

// Combo Box でのキー入力．
struct DropdownList : SettingDlg {
private:
	static inline constinit HWND combo = nullptr;
	static LRESULT CALLBACK mainwindow_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto...)
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
	static uintptr_t hook_uid() { return SettingDlg::hook_uid() + 3; }

public:
	static void on_notify_open(HWND ctrl) {
		combo = ctrl;
		::SetWindowSubclass(exedit.fp->hwnd_parent, mainwindow_hook, hook_uid(), {});
	}
	static void on_notify_close() {
		combo = nullptr;
		::RemoveWindowSubclass(exedit.fp->hwnd_parent, mainwindow_hook, hook_uid());
	}
};


////////////////////////////////
// 設定項目．
////////////////////////////////
inline constinit struct Settings {
	struct modkey_set {
		modkeys keys_decimal, keys_boost;
		bool def_decimal;
		uint16_t rate_boost;

		constexpr static uint16_t min_rate_boost = 1, max_rate_boost = 10000;

		constexpr bool decimal(modkeys keys) const
		{
			return (keys_decimal.has_flags() && keys >= keys_decimal) ^ def_decimal;
		}
		constexpr bool boost(modkeys keys) const
		{
			return keys_boost.has_flags() && keys >= keys_boost;
		}
		constexpr auto calc_rate(modkeys keys, const TrackInfo& info) const
		{
			return (boost(keys) ? rate_boost : 1) * (decimal(keys) ? 1 : info.precision());
		}
	};

	struct {
		struct {
			byte vkey;
			modkeys mkeys;
			constexpr bool match(byte code, modkeys keys) const {
				return vkey != 0 && code == vkey && mkeys == keys;
			}
		} forward{ VK_TAB, modkeys::none }, backward{ VK_TAB, modkeys::shift };
		constexpr bool is_enabled() const { return forward.vkey != 0 || backward.vkey != 0; }
		constexpr bool match(bool& is_forward, auto... args)
		{
			if (forward.match(args...)) is_forward = true;
			else if (backward.match(args...)) is_forward = false;
			else return false;
			return true;
		}
	} textFocus;

	struct {
		bool batch;
		bool hide_cursor;

		int16_t tabstops_text, tabstops_script;
		constexpr bool is_tabstops_enabled() const { return tabstops_text >= 0 || tabstops_script >= 0; }
		constexpr static int16_t min_tabstops = -1, max_tabstops = 256;

		std::unique_ptr<std::wstring> replace_tab_text, replace_tab_script;
		constexpr bool is_replace_tab_enabled() const { return replace_tab_text || replace_tab_script; }
		constexpr static size_t max_replace_tab_length = 256;

		constexpr bool is_enabled() const { return batch || hide_cursor || is_tabstops_enabled() || is_replace_tab_enabled(); }
	} textTweaks{ true, false, -1, -1, nullptr, nullptr };

	struct : modkey_set {
		bool updown, updown_clamp, escape;
		constexpr bool is_enabled() const { return updown || escape; }
		constexpr bool no_wrong_keys(modkeys keys) const
		{
			return keys <= (keys_decimal | keys_boost);
		}
	} trackKbd{
		{ modkeys::shift, modkeys::alt, false, 10, },
		true, false, true,
	};

	struct : modkey_set {
		bool wheel, reverse_wheel, cursor_react;
		modkeys keys_activate;
		constexpr bool is_enabled() const { return wheel; }
		constexpr bool is_activated(modkeys keys) const
		{
			return keys >= keys_activate && no_wrong_keys(keys);
		}
	private:
		constexpr bool no_wrong_keys(modkeys keys) const
		{
			return keys <= (keys_decimal | keys_boost | keys_activate
				| modkeys::ctrl /* don't count ctrl as wrong. */);
		}
	} trackMouse{
		{ modkeys::shift, modkeys::alt, false, 10, },
		true, false, true, modkeys::ctrl,
	};

	struct : modkey_set {
		bool modify;
		constexpr bool is_enabled() const {
			return modify && (keys_decimal.has_flags() || keys_boost.has_flags() || def_decimal == false);
		}
		constexpr bool no_wrong_keys(modkeys keys) const
		{
			return keys <= (keys_decimal | keys_boost | modkeys::ctrl /* don't count ctrl as wrong. */);
		}
	} trackBtn{ modkeys::shift, modkeys::alt, true, 10, true };

	struct {
		bool search;
	} dropdownKbd{ true };

	constexpr bool is_enabled() const
	{
		return textFocus.is_enabled() || textTweaks.is_enabled() ||
			trackKbd.is_enabled() || trackMouse.is_enabled() || trackBtn.is_enabled() ||
			dropdownKbd.search;
	}

	void load(const char* ini_file)
	{
		auto read_raw = [&](auto def, const char* section, const char* key) {
			return static_cast<decltype(def)>(
				::GetPrivateProfileIntA(section, key, static_cast<int32_t>(def), ini_file));
		};
		auto read_modkey = [&](modkeys def, const char* section, const char* key) {
			char str[std::bit_ceil(std::size("ctrl + shift + alt ****"))];
			::GetPrivateProfileStringA(section, key, def.canon_name(), str, std::size(str), ini_file);
			return modkeys{ str, def };
		};
		auto read_string = [&]<size_t max_len>(const char* def, const char* section, const char* key) {
			char str[max_len + 1];
			::GetPrivateProfileStringA(section, key, def, str, std::size(str), ini_file);
			if (std::strcmp(str, def) == 0)
				return std::unique_ptr<std::wstring>{ nullptr };
			return std::make_unique<std::wstring>(Encodes::to_wstring(str));
		};
#define load_gen(head, tgt, section, read, write)	head##tgt = read##(read_raw(write##(head##tgt), section, #tgt))
#define load_int(head, tgt, section)	load_gen(head, tgt, section, /* id */, /* id */)
#define load_bool(head, tgt, section)	load_gen(head, tgt, section, \
			[](auto y) { return y != 0; }, [](auto x) { return x ? 1 : 0; })
#define load_key(head, tgt, section)	head##tgt = read_modkey(head##tgt, section, #tgt)
#define load_str(head, tgt, section, def, max)	head##tgt = read_string.operator()<max>(def, section, #tgt)

		load_int (textFocus., forward.vkey,		"TextBox.Focus");
		load_key (textFocus., forward.mkeys,	"TextBox.Focus");

		load_int (textFocus., backward.vkey,	"TextBox.Focus");
		load_key (textFocus., backward.mkeys,	"TextBox.Focus");

		load_bool(textTweaks., batch,			"TextBox.Tweaks");
		load_bool(textTweaks., hide_cursor,		"TextBox.Tweaks");
		load_gen (textTweaks., tabstops_text,	"TextBox.Tweaks",
			[&](auto x) { return std::clamp(x, textTweaks.min_tabstops, textTweaks.max_tabstops); }, /* id */);
		load_gen (textTweaks., tabstops_script,	"TextBox.Tweaks",
			[&](auto x) { return std::clamp(x, textTweaks.min_tabstops, textTweaks.max_tabstops); }, /* id */);
		load_str (textTweaks., replace_tab_text,	"TextBox.Tweaks", "\t", textTweaks.max_replace_tab_length);
		load_str (textTweaks., replace_tab_script,	"TextBox.Tweaks", "\t", textTweaks.max_replace_tab_length);

		load_bool(trackKbd., updown,			"Track.Keyboard");
		load_bool(trackKbd., updown_clamp,		"Track.Keyboard");
		load_bool(trackKbd., escape,			"Track.Keyboard");
		load_key (trackKbd., keys_decimal,		"Track.Keyboard");
		load_key (trackKbd., keys_boost,		"Track.Keyboard");
		load_bool(trackKbd., def_decimal,		"Track.Keyboard");
		load_gen (trackKbd., rate_boost,		"Track.Keyboard",
			[](auto x) { return std::clamp(x, modkey_set::min_rate_boost, modkey_set::max_rate_boost); }, /* id */);

		load_bool(trackMouse., wheel,			"Track.Mouse");
		load_bool(trackMouse., reverse_wheel,	"Track.Mouse");
		load_bool(trackMouse., cursor_react,	"Track.Mouse");
		load_key (trackMouse., keys_activate,	"Track.Mouse");
		load_key (trackMouse., keys_decimal,	"Track.Mouse");
		load_key (trackMouse., keys_boost,		"Track.Mouse");
		load_bool(trackMouse., def_decimal,		"Track.Mouse");
		load_gen (trackMouse., rate_boost,		"Track.Mouse",
			[](auto x) { return std::clamp(x, modkey_set::min_rate_boost, modkey_set::max_rate_boost); }, /* id */);

		load_bool(trackBtn., modify,			"Track.Button");
		load_key (trackBtn., keys_decimal,		"Track.Button");
		load_key (trackBtn., keys_boost,		"Track.Button");
		load_bool(trackBtn., def_decimal,		"Track.Button");
		load_gen (trackBtn., rate_boost,		"Track.Button",
			[](auto x) { return std::clamp(x, modkey_set::min_rate_boost, modkey_set::max_rate_boost); }, /* id */);

		load_bool(dropdownKbd., search,			"Dropdown.Keyboard");

#undef load_str
#undef load_key
#undef load_bool
#undef load_int
#undef load_gen
	}
} settings;


////////////////////////////////
// 設定ロードセーブ．
////////////////////////////////

// replacing a file extension when it's known.
template<class T, size_t len_max, size_t len_old, size_t len_new>
static void replace_tail(T(&dst)[len_max], size_t len, const T(&tail_old)[len_old], const T(&tail_new)[len_new])
{
	if (len < len_old || len - len_old + len_new > len_max) return;
	std::memcpy(dst + len - len_old, tail_new, len_new * sizeof(T));
}
inline void load_settings(HMODULE h_dll)
{
	char path[MAX_PATH];
	replace_tail(path, ::GetModuleFileNameA(h_dll, path, std::size(path)) + 1, "auf", "ini");

	settings.load(path);
}


////////////////////////////////
// コマンドの定義．
////////////////////////////////
struct Menu {
	enum class ID : unsigned int {
		TextFocusFwd,
		TextFocusBwd,

		TrackFocusFwd,
		TrackFocusBwd,

		Invalid,
	};
	static constexpr bool is_invalid(ID id) { return id >= Invalid; }

	using enum ID;
	static constexpr struct { ID id; const char* name; } Items[] = {
		{ TextFocusFwd,		"テキストにフォーカス移動" },
		{ TextFocusBwd,		"テキストにフォーカス移動(逆順)" },
		{ TrackFocusFwd,	"トラックバーにフォーカス移動" },
		{ TrackFocusBwd,	"トラックバーにフォーカス移動(逆順)" },
	};
};


////////////////////////////////
// 各種コマンド実行の実装．
////////////////////////////////

// メニューのコマンドハンドラ．
inline bool menu_command_handler(Menu::ID id)
{
	constexpr auto fallback = [] {
		// force the focus onto the timeline window if no target is found.
		::SetFocus(exedit.fp->hwnd);
	};

	bool forward = false;
	switch (id) {
	case Menu::TextFocusFwd:
		forward = true;
		[[fallthrough]];
	case Menu::TextFocusBwd:
		if (auto target = TextBox::next_edit_box(nullptr, forward);
			target != nullptr) ::SetFocus(target);
		else fallback();
		break;

	case Menu::TrackFocusFwd:
		forward = true;
		[[fallthrough]];
	case Menu::TrackFocusBwd:
		if (auto target = TrackLabel::first_trackinfo(forward);
			target != nullptr) {
			::SetFocus(target->hwnd_label);
			set_edit_selection_all(target->hwnd_label);
		}
		else fallback();
		break;
	}
	return false;
}

// TAB 文字サイズを変更する機能．
inline void set_tabstops(HWND hwnd)
{
	TextBox::set_tabstops(hwnd, [](auto kind) {
		switch (kind) {
		case TextBox::kind::text: return settings.textTweaks.tabstops_text;
		case TextBox::kind::script: return settings.textTweaks.tabstops_script;
		}
		std::unreachable();
	});
}

// TAB 文字を別の文字列で置き換える機能．
inline std::wstring const* replace_tab(wchar_t ch, HWND edit_box) {
	if (ch != L'\t') return nullptr;

	switch (TextBox::edit_box_kind(edit_box)) {
	case TextBox::kind::text:
		return settings.textTweaks.replace_tab_text.get();
	case TextBox::kind::script:
		return settings.textTweaks.replace_tab_script.get();
	}
	std::unreachable();
}

// 指定キーでフォーカスをテキストボックスから移動する機能．
inline bool focus_from_textbox(byte vkey, HWND edit_box)
{
	// see if the combination including modifier keys matches.
	if (key_pressed_any(VK_LWIN, VK_RWIN)) return false;

	bool forward;
	if (!settings.textFocus.match(forward, vkey, curr_modkeys()))
		return false;

	// find the "next" target for the focus. shift key reverses the direction.
	auto target = TextBox::next_edit_box(edit_box, forward);
	if (target == nullptr) target = exedit.fp->hwnd;

	// non-null seems to mean the focus moved successfully.
	return ::SetFocus(target) != nullptr;
}

// 上下キーでトラックバーの数値をテキストラベル上で増減する機能．
inline bool delta_move_on_label(byte vkey)
{
	if (!TrackLabel::is_focused()) [[unlikely]] return false;

	// only applies to up and down keys.
	if (vkey != VK_UP && vkey != VK_DOWN) return false;

	// combinations of modifier keys that don't make sense shall not take action.
	auto keys = curr_modkeys();
	if (!settings.trackKbd.no_wrong_keys(keys) ||
		key_pressed_any(VK_LWIN, VK_RWIN)) return false;

	// determine the delta value taking the modifier keys in account.
	int delta = vkey == VK_UP ? +1 : -1;
	delta *= settings.trackKbd.calc_rate(keys, TrackLabel::curr_info());

	// then move.
	if (!TrackLabel::add_number(delta, settings.trackKbd.updown_clamp)) {
		// in case of text errors.
		return false;
	}
	return true;
}

// ESC キーで編集前に戻す / フォーカスを外す機能．
inline bool escape_from_label(HWND edit_box)
{
	if (!TrackLabel::is_focused()) [[unlikely]] return false;

	// combinations of any modifier keys shall not take action.
	if (key_pressed_any(VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN)) return false;

	if (TrackLabel::revert()) return true; // reverted the content, the focus stays.
	return ::SetFocus(exedit.fp->hwnd) != nullptr; // no need to revert, the focus moves away.
}

// ホイールでトラックバーの数値をテキストラベル上で増減する機能．
inline bool wheel_on_track(const TrackInfo& info, short wheel, modkeys keys)
{
	// determine the delta value taking the modifier keys in account.
	int delta = (wheel > 0) ^ settings.trackMouse.reverse_wheel ? +1 : -1;
	delta *= settings.trackMouse.calc_rate(keys, info);

	// calculate the resulting value.
	auto val = std::clamp(info.val_int_curr + delta * info.delta, info.val_int_min, info.val_int_max);
	if (val == info.val_int_curr) return false;

	// rescale val to suit with the updown control.
	val /= info.delta;

	// send the new value.
	::SendMessageW(info.hwnd_updown, UDM_SETPOS32, 0, static_cast<LPARAM>(val));
	::SendMessageW(SettingDlg::get_hwnd(), WM_HSCROLL, (val << 16) | SB_THUMBPOSITION, reinterpret_cast<LPARAM>(info.hwnd_updown));
	return true;
}

// スピンコントロールの増減量を修飾キーで調整する機能．
inline void modify_updown_delta(const TrackInfo& info, int& delta)
{
	// combinations of modifier keys that don't make sense shall not take action.
	auto keys = curr_modkeys();
	if (!settings.trackBtn.no_wrong_keys(keys) ||
		key_pressed_any(VK_LWIN, VK_RWIN)) return;

	// modify the delta value taking the modifier keys in account.
	delta *= settings.trackBtn.calc_rate(keys, info);
}


////////////////////////////////
// フックのコールバック関数．
////////////////////////////////
namespace PrvMsg {
	enum : uint32_t {
		// posted to the message-only window associated to this filter,
		// to notify a new window has been created on the dialog box,
		// which might be an edit control to be modified.
		NotifyCreate = WM_USER + 1,

		// posted to the message-only window associated to this filter,
		// to notify that a change was made to certain edit controls.
		NotifyUpdate,
	};
}
LRESULT CALLBACK text_box_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (settings.textFocus.is_enabled() &&
			focus_from_textbox(static_cast<byte>(wparam), hwnd)) {
			// prevent the character that is pressed now to be written to the edit box,
			// by discarding input messages sent to this edit box.
			// (for TAB key, takes effect only when ctrl is not pressed.)
			// for WM_SYSKEYDOWN, it prevents notification sound from playing.
			discard_message(hwnd, message == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR);

			// shouldn't pass to ::DefSubclassProc(). (for TAB key, takes effect only when ctrl is pressed.)
			return 0;
		}
		break;

	case WM_CHAR:
	case WM_IME_COMPOSITION:
		if (settings.textTweaks.hide_cursor && !TextBox::hide_cursor.is_hidden()) {
			// hide the cursor on keyboard inputs.
			POINT pt;
			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);
			TextBox::hide_cursor.on_edit(pt);
		}
		if (message != WM_CHAR) break;

		// replace a TAB with white spaces.
		if (auto* wstr = replace_tab(static_cast<wchar_t>(wparam), hwnd); wstr != nullptr) {
			if (wstr->length() != 1) {
				::DefSubclassProc(hwnd, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(wstr->c_str()));
				return 0;
			}
			// for a single character, just replace a certain parameter,
			// which works a bit better with undo-ing feature.
			wparam = static_cast<WPARAM>((*wstr)[0]);
		}
		break;

	case WM_SETCURSOR:
		// hide the cursor if specified.
		if (settings.textTweaks.hide_cursor && TextBox::hide_cursor.is_hidden()) {
			::SetCursor(nullptr);
			return FALSE; // prioritize the parent window's choice of the cursor shape.
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		// show again the mouse cursor on clicks.
		if (settings.textTweaks.hide_cursor && TextBox::hide_cursor.is_hidden()) {
			TextBox::hide_cursor.reset();
			// since this click initiates dragging, WM_SETCURSOR won't be sent automatically.
			::PostMessageW(hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), HTCLIENT | (message << 16));
		}
		break;
	case WM_MOUSEMOVE:
		// show again the mouse cursor on move.
		if (settings.textTweaks.hide_cursor)
			TextBox::hide_cursor.on_move({ .x = static_cast<int16_t>(lparam & 0xffff), .y = static_cast<int16_t>(lparam >> 16) });
		break;
	case WM_KILLFOCUS:
		::RemoveWindowSubclass(hwnd, text_box_hook, id);
		if (settings.textTweaks.hide_cursor) TextBox::hide_cursor.reset();
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK track_label_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (byte vkey = static_cast<byte>(wparam);
			settings.trackKbd.updown &&
			delta_move_on_label(vkey)) {
			// prevent calling the default window procedure.
			return 0;
		}
		else if (settings.trackKbd.escape && vkey == VK_ESCAPE && escape_from_label(hwnd)) {
			// needs to remove WM_CHAR messages to avoid the notification sound.
			discard_message(hwnd, message == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR);
			return 0;
		}
		break;
	case WM_KILLFOCUS:
		TrackLabel::on_killfocus();
		::RemoveWindowSubclass(hwnd, track_label_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_COMMAND:
		if (auto ctrl = reinterpret_cast<HWND>(lparam); ctrl != nullptr) {
			switch (wparam >> 16) {
			case EN_SETFOCUS:
				if ((settings.textFocus.is_enabled() ||
					settings.textTweaks.hide_cursor || settings.textTweaks.is_replace_tab_enabled()) &&
					TextBox::edit_box_kind(ctrl) != TextBox::kind::unspecified &&
					TextBox::check_edit_box_style(ctrl))
					// hook for shortcut keys to move the focus out of this edit box.
					::SetWindowSubclass(ctrl, text_box_hook, TextBox::hook_uid(), 0);

				else if (const TrackInfo* info;
					settings.trackKbd.is_enabled() &&
					(info = TrackLabel::find_trackinfo(wparam & 0xffff, ctrl)) != nullptr) {
					// hook for manipulating values by arrow keys.
					TrackLabel::on_setfocus(info);
					::SetWindowSubclass(ctrl, track_label_hook, TrackLabel::hook_uid(), 0);
				}
				break;

			case EN_CHANGE:
				if (settings.textTweaks.batch &&
					!TextBox::batch.is_batch_call() &&
					TextBox::edit_box_kind(ctrl) != TextBox::kind::unspecified &&
					// 未確定文字がある状態でフォーカスが移動した場合 WM_KILLFOCUS -> EN_CHANGE の順になるので，その場面に対応するためコメントアウト．
					//::GetFocus() == ctrl &&
					TextBox::check_edit_box_style(ctrl)) {

					//if (*exedit.is_playing == 0) {
					//	// 通常の編集中のユーザの入力と判断，
					//	// メッセージの転送を保留して，次のメッセージループで再送するよう仕掛ける．
					//	TextBox::batch.post(hwnd, message, wparam, lparam,
					//		reinterpret_cast<HWND>(data), PrvMsg::NotifyUpdate);
					//	return 0;
					//}
					//else
					//	// プレビュー再生中のユーザの入力と判断，
					//	// 遅延させずに即座に効果を持たせる（再生が停止する）．遅延待機中の更新通知があるなら破棄．
					//	TextBox::batch.discard(reinterpret_cast<HWND>(data), PrvMsg::NotifyUpdate);
					// NOTE: 再生の停止は一連の入力メッセージが処理された後で起こるため，これは悪手だった．

					// メッセージの転送を保留して，次のメッセージループで再送するよう仕掛ける．
					TextBox::batch.post(hwnd, message, wparam, lparam,
						reinterpret_cast<HWND>(data), PrvMsg::NotifyUpdate);
					return 0;
				}
				break;

				// 開いている Combo Box の追跡．
			case CBN_DROPDOWN:
				if (settings.dropdownKbd.search && check_window_class(ctrl, WC_COMBOBOXW))
					DropdownList::on_notify_open(ctrl);
				break;

			case CBN_CLOSEUP:
				if (settings.dropdownKbd.search && check_window_class(ctrl, WC_COMBOBOXW))
					DropdownList::on_notify_close();
				break;
			}
		}
		break;
	case WM_MOUSEWHEEL:
		if (settings.trackMouse.wheel &&
			*exedit.SettingDialogObjectIndex >= 0) {
			auto keys = modkeys{ (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, key_pressed_any(VK_MENU) };

			// check for the activation key.
			if (!settings.trackMouse.is_activated(keys) ||
				key_pressed_any(VK_LWIN, VK_RWIN)) break;

			// find the pointer position.
			POINT pt = { static_cast<int16_t>(lparam & 0xffff), static_cast<int16_t>(lparam >> 16) };
			::ScreenToClient(hwnd, &pt);

			// if the track is found, make an attempt to move the value.
			if (auto* info = TrackLabel::find_trackinfo(pt); info != nullptr &&
				wheel_on_track(*info, static_cast<int16_t>(wparam >> 16), keys))
				return 0; // successfully moved the value.

			// default behavior otherwise.
		}
		break;
	case WM_SETCURSOR:
		if (settings.trackMouse.wheel && settings.trackMouse.cursor_react &&
			*exedit.SettingDialogObjectIndex >= 0) {
			// check for the activation key.
			if (!settings.trackMouse.is_activated(curr_modkeys()) ||
				key_pressed_any(VK_LWIN, VK_RWIN)) break;

			// find the pointer position.
			POINT pt;
			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);

			// if the track is found, change the cursor to hand.
			if (TrackLabel::find_trackinfo(pt) != nullptr) {
				::SetCursor(TrackLabel::cursor.get());
				return TRUE;
			}
		}
		break;
	case WM_NOTIFY:
		// same strat as updown plugin by nanypoco:
		// https://github.com/nanypoco/updown
		if (settings.trackBtn.is_enabled() &&
			*exedit.SettingDialogObjectIndex >= 0) {
			// check if it's from an UpDown control.
			auto* header = reinterpret_cast<NMHDR*>(lparam);
#pragma warning(suppress: 26454) // overflow warning by the macro UDN_DELTAPOS.
			if (header == nullptr || header->code != UDN_DELTAPOS ||
				header->hwndFrom == nullptr ||
				!check_window_class(header->hwndFrom, UPDOWN_CLASSW)) break;

			// see if it's a part of the trackbars.
			// the previous should be the label, from which I can find the id of the track.
			auto const* info = TrackLabel::find_trackinfo(::GetWindow(header->hwndFrom, GW_HWNDPREV));
			if (info == nullptr || info->hwnd_updown != header->hwndFrom ||
				!info->is_visible()) break;

			// then modify the notification message.
			modify_updown_delta(*info, reinterpret_cast<NMUPDOWN*>(header)->iDelta);
		}
		break;

	case WM_PARENTNOTIFY:
		if (settings.textTweaks.is_tabstops_enabled() &&
			(wparam & 0xffff) == WM_CREATE) {
			// at this moment, we can't tell this is an edit control for text objects or script effects
			// as their handles aren't assinged yet to exedit.hwnd_edit_[text/script].
			// so put a delay by posting a message to the private window.
			::PostMessageW(reinterpret_cast<HWND>(data), PrvMsg::NotifyCreate, wparam, lparam);
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, setting_dlg_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK trackbar_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_MOUSEWHEEL:
		// as trackbars process wheel messages by their own,
		// the message doesn't propagate to the parent. it should be hooked.
		if (settings.trackMouse.is_activated({ (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, key_pressed_any(VK_MENU) }) &&
			!key_pressed_any(VK_LWIN, VK_RWIN))
			return ::SendMessageW(SettingDlg::get_hwnd(), message, wparam, lparam);
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, trackbar_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

// カーソル切り替えのためのキーボード監視用フック．
class KeyboardHook {
	inline constinit static HHOOK handle = nullptr;
	static LRESULT CALLBACK callback(int code, WPARAM wparam, LPARAM lparam)
	{
		if (code == HC_ACTION &&
			(((lparam << 1) ^ lparam) & (1u << 31)) == 0 && // handle only new presses or new releases.
			is_target_key(wparam)) {
			// check if the cursor lies on the setting dialog.
			if (auto dlg = SettingDlg::get_hwnd(); dlg != nullptr) {
				if (auto hwnd = mouse_window(); hwnd != nullptr &&
					(dlg == hwnd || ::IsChild(dlg, hwnd))) {
					// post a message to determine the cursor shape.
					::PostMessageW(hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), HTCLIENT | (WM_MOUSEMOVE << 16));
				}
			}
		}
		return ::CallNextHookEx(handle, code, wparam, lparam);
	}
	static constexpr bool is_target_key(auto vk) {
		switch (vk) {
		case VK_CONTROL:
		case VK_SHIFT:
		case VK_MENU:
		case VK_LWIN:
		case VK_RWIN: return true;
		default: return false;
		}
	}
	static HWND mouse_window() {
		POINT pt;
		::GetCursorPos(&pt);
		return ::WindowFromPoint(pt);
	}

public:
	static void set() {
		if (handle != nullptr) return;
		handle = ::SetWindowsHookExW(WH_KEYBOARD, callback, nullptr, ::GetCurrentThreadId());
	}
	static void unhook() {
		if (handle == nullptr) return;
		::UnhookWindowsHookEx(handle);
		handle = nullptr;
	}
};


////////////////////////////////
// AviUtlに渡す関数の定義．
////////////////////////////////
BOOL func_init(FilterPlugin* fp)
{
	// 情報源確保．
	if (!exedit.init(fp)) {
		::MessageBoxA(fp->hwnd, "拡張編集0.92が見つかりませんでした．",
			fp->name, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// message-only window を作成，登録．これで NoConfig でも AviUtl からメッセージを受け取れる.
	fp->hwnd = ::CreateWindowExW(0, L"AviUtl", L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, nullptr, fp->hinst_parent, nullptr);

	// メニュー登録．
	for (auto [id, name] : Menu::Items)
		fp->exfunc->add_menu_item(fp, name, fp->hwnd, static_cast<int32_t>(id), 0, ExFunc::AddMenuItemFlag::None);

	// 設定ロード．
	load_settings(fp->dll_hinst);

	// 競合確認．
	if (settings.trackBtn.is_enabled() && ::GetModuleHandleW(L"updown.auf") != nullptr) {
		settings.trackBtn.modify = false;
		::MessageBoxA(fp->hwnd, "updown.auf と競合しているため一部機能を無効化しました．\n"
			"次回以降このメッセージを表示させないためには，設定ファイルで以下の設定をしてください:\n\n"
			"[Track.Button]\nmodify=0",
			fp->name, MB_OK | MB_ICONEXCLAMATION);
	}

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, EditHandle* editp, FilterPlugin* fp)
{
	using Message = FilterPlugin::WindowMessage;
	switch (message) {
		// 拡張編集設定ダイアログ等のフック/アンフック．
	case Message::ChangeWindow:
		if (settings.is_enabled() && SettingDlg::is_valid()) {
			::SetWindowSubclass(SettingDlg::get_hwnd(), setting_dlg_hook, SettingDlg::hook_uid(),
				reinterpret_cast<uintptr_t>(hwnd));

			if (settings.trackMouse.wheel) {
				for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++) {
					::SetWindowSubclass(exedit.trackinfo_left[i].hwnd_track, trackbar_hook, TrackLabel::hook_uid(), {});
					::SetWindowSubclass(exedit.trackinfo_right[i].hwnd_track, trackbar_hook, TrackLabel::hook_uid(), {});
				}

				// hooking keyboard inputs to change the cursor shape when certain keys are pressed.
				if (settings.trackMouse.cursor_react &&
					settings.trackMouse.keys_activate.has_flags())
					KeyboardHook::set();
			}
		}
		break;
	case Message::Exit:
		// at this moment, the setting dialog and the timeline window are already destroyed.
		TextBox::batch.discard(hwnd, PrvMsg::NotifyUpdate);
		KeyboardHook::unhook();
		TrackLabel::cursor.free();

		// message-only window を削除．必要ないかもしれないけど．
		fp->hwnd = nullptr; ::DestroyWindow(hwnd);
		break;

	case PrvMsg::NotifyCreate:
		// apply tabstops to edit controls on their creation.
		set_tabstops(reinterpret_cast<HWND>(lparam));
		break;

	case PrvMsg::NotifyUpdate:
		// send a "bundled" notification message from edit controls.
		TextBox::batch();
		break;

		// command handlers.
	case Message::Command:
		if (auto id = static_cast<Menu::ID>(wparam); !Menu::is_invalid(id) &&
			fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp)) {

			return menu_command_handler(id) ? TRUE : FALSE;
		}
		break;
	}
	return FALSE;
}


////////////////////////////////
// Entry point.
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"Reactive Dialog"
#define PLUGIN_VERSION	"v1.21-beta1"
#define PLUGIN_AUTHOR	"sigma-axis"
#define PLUGIN_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define PLUGIN_INFO		PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR)

extern "C" __declspec(dllexport) FilterPluginDLL* __stdcall GetFilterTable(void)
{
	// （フィルタとは名ばかりの）看板．
	using Flag = FilterPlugin::Flag;
	static constinit FilterPluginDLL filter{
		.flag = Flag::NoConfig | Flag::AlwaysActive | Flag::ExInformation,
		.name = PLUGIN_NAME,

		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = PLUGIN_INFO,
	};
	return &filter;
}
