/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <map>
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
#include "slim_formatter.hpp"
#include "memory_protect.hpp"


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
	constexpr static char const* info_exedit092 = "拡張編集(exedit) version 0.92 by ＫＥＮくん";
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
	int32_t*	track_label_is_dragging;	// 0x158d30; 0: idle, 1: dragging.
	int32_t*	track_label_start_drag_x;	// 0x179218
	HWND*		hwnd_track_buttons;			// 0x158f68

	//WNDPROC		setting_dlg_wndproc;		// 0x02cde0

	HWND*		filter_checkboxes;			// 0x14d368
	HWND*		filter_separators;			// 0x1790d8
	uintptr_t*	exdata_table;				// 0x1e0fa8
	char const*	basic_scene_change_names;	// 0x0aef38
	char const*	basic_animation_names;		// 0x0c1f08
	char const* track_script_names;			// 0x0ca010
	char const*	basic_custom_obj_names;		// 0x0ce090
	char const*	basic_camera_eff_names;		// 0x0d20d0
	uint32_t*	easing_specs_builtin;		// 0x0b8390
	uint8_t*	easing_specs_script;		// 0x231d90

	uintptr_t	cmp_shift_state_easing;		// 0x02ca90
	byte*		call_easing_popup_menu;		// 0x02d4b5

	// index: index of the script; zero1, zero2: zero, unknown otherwise.
	void(*load_easing_spec)(int32_t index, int32_t zero1, int32_t zero2); // 0x087940

	Filter**	loaded_filter_table;		// 0x187c98
	//int32_t*	loaded_filter_count;		// 0x146248

private:
	void init_pointers()
	{
		auto pick_addr = [exedit_base=reinterpret_cast<uintptr_t>(fp->dll_hinst)]
			<class T>(T& target, ptrdiff_t offset) { target = std::bit_cast<T>(exedit_base + offset); };
		pick_addr(ObjectArray_ptr,			0x1e0fa4);
		pick_addr(SettingDialogObjectIndex,	0x177a10);
		pick_addr(hwnd_setting_dlg,			0x1539c8);
		//pick_addr(is_playing,				0x1a52ec);
		pick_addr(hwnd_edit_text,			0x236328);
		pick_addr(hwnd_edit_script,			0x230C78);
		pick_addr(trackinfo_left,			0x14d4c8);
		pick_addr(trackinfo_right,			0x14def0);
		pick_addr(track_label_is_dragging,	0x158d30);
		pick_addr(track_label_start_drag_x,	0x179218);
		pick_addr(hwnd_track_buttons,		0x158f68);

		//pick_addr(setting_dlg_wndproc,		0x02cde0);

		pick_addr(filter_checkboxes,		0x14d368);
		pick_addr(filter_separators,		0x1790d8);
		pick_addr(exdata_table,				0x1e0fa8);
		pick_addr(basic_scene_change_names,	0x0aef38);
		pick_addr(basic_animation_names,	0x0c1f08);
		pick_addr(track_script_names,		0x0ca010);
		pick_addr(basic_custom_obj_names,	0x0ce090);
		pick_addr(basic_camera_eff_names,	0x0d20d0);
		pick_addr(easing_specs_builtin,		0x0b8390);
		pick_addr(easing_specs_script,		0x231d90);

		pick_addr(cmp_shift_state_easing,	0x02ca90);
		pick_addr(call_easing_popup_menu,	0x02d4b5);
		pick_addr(load_easing_spec,			0x087940);

		pick_addr(loaded_filter_table,		0x187c98);
		//pick_addr(loaded_filter_count,		0x146248);
	}
} exedit;

namespace filter_id
{
	enum id : int32_t {
		draw_std = 0x0a, // 標準描画
		draw_ext = 0x0b, // 拡張描画
		play_std = 0x0c, // 標準再生
		particle = 0x0d, // パーティクル出力

		scn_chg  = 0x0e, // シーンチェンジ
		anim_eff = 0x4f, // アニメーション効果
		cust_obj = 0x50, // カスタムオブジェクト
		cam_eff  = 0x61, // カメラ効果
	};
}


////////////////////////////////
// 文字エンコード変換．
////////////////////////////////
struct Encodes {
	template<UINT codepage = CP_UTF8>
	static std::wstring to_wstring(std::string_view const& src) {
		if (src.length() == 0) return L"";

		auto wlen = ::MultiByteToWideChar(codepage, 0, src.data(), src.length(), nullptr, 0);
		std::wstring ret(wlen, L'\0');
		::MultiByteToWideChar(codepage, 0, src.data(), src.length(), ret.data(), wlen);

		return ret;
	}
};


////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////
template<int N>
bool check_window_class(HWND hwnd, wchar_t const(&classname)[N])
{
	wchar_t buf[N + 1];
	return ::GetClassNameW(hwnd, buf, N + 1) == N - 1
		&& std::wmemcmp(buf, classname, N - 1) == 0;
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

inline byte set_key_state(byte vkey, byte state)
{
	byte states[0x100];
	std::ignore = ::GetKeyboardState(states);
	std::swap(state, states[vkey]);
	::SetKeyboardState(states);
	return state;
}

inline void discard_message(HWND hwnd, UINT message) {
	MSG msg;
	while (::PeekMessageW(&msg, hwnd, message, message, PM_REMOVE));
}


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
	static void set_tabstops(HWND hwnd, auto const& choose)
	{
		auto kind = edit_box_kind(hwnd);
		if (kind == kind::unspecified) return;
		if (auto value = choose(kind); value >= 0)
			::SendMessageW(hwnd, EM_SETTABSTOPS, { 1 }, reinterpret_cast<LPARAM>(&value));
	}

	// テキスト入力の画面反映処理を一括する機能．
	static inline constinit struct {
		void operator()()
		{
			// ignore unexpected calls.
			if (!posted) return;
			posted = false;

			calling = true;
			// calling exedit.setting_dlg_wndproc() would be faster,
			// but might ignore hooks from other plugins.
			::SendMessageW(hwnd, message, wparam, lparam);
			calling = false;
		}
		bool is_calling() const { return calling; }
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
		bool calling = false;

		// message paramters.
		HWND hwnd{};
		UINT message{};
		WPARAM wparam{};
		LPARAM lparam{};
	} batch;

	// テキスト入力中にカーソルを隠す機能．
	static inline constinit struct {
	private:
		static constexpr auto dist(POINT const& p1, POINT const& p2) {
			// so-called L^\infty-distance.
			return std::max(std::abs(p1.x - p2.x), std::abs(p1.y - p2.y));
		}
		bool hide = false;
		POINT pos{};

	public:
		bool is_hidden() const { return hide; }
		void reset() { hide = false; }
		void on_edit(POINT const& pt) { hide = true; pos = pt; }
		void on_move(POINT const& pt) {
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
	constexpr static uint32_t id_button_base = 4000,
		id_label_left_base = 4100, id_label_right_base = 4200;

public:
	static uintptr_t hook_uid() { return SettingDlg::hook_uid() + 2; }

	static constexpr int8_t check_button_id(uint32_t id)
	{
		auto idx = id - id_button_base;
		if (idx >= Object::MAX_TRACK) return -1;
		return static_cast<int8_t>(idx);
	}
	static constexpr uint32_t button_id(size_t idx) {
		return idx + id_button_base;
	}
	static constexpr std::pair<bool, int8_t> check_edit_box_id(uint32_t id)
	{
		bool left = id < id_label_right_base;
		id -= left ? id_label_left_base : id_label_right_base;
		if (id >= Object::MAX_TRACK) return { false, -1 };
		return { left , static_cast<int8_t>(id) };
	}
	static constexpr uint32_t label_id(bool left, size_t idx) {
		return idx + (left ? id_label_left_base : id_label_right_base);
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
		auto const track_n = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex].track_n;

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
		auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
		auto const track_n = obj.track_n;
		int track_o = 0;

		// needs re-ordering the indices if the final filter is of the certain type.
		if (auto const& filter_last = obj.filter_param[obj.countFilters() - 1];
			has_flag_or(exedit.loaded_filter_table[filter_last.id]->flag, Filter::Flag::Output))
			track_o = filter_last.track_begin; // alter offset.

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
	static inline constinit TrackInfo const* info = nullptr;
	static inline constinit wchar_t last_text[16] = L"";

public:
	static inline POINT mouse_pos_on_focused{};
	static inline struct {
		uint16_t idx = ~0;
		byte alt_state = 0;
		constexpr bool is_active() const { return idx < Object::MAX_TRACK; }
		constexpr void activate(size_t idx) { this->idx = static_cast<decltype(this->idx)>(idx); }
		constexpr void deactivate() { idx = ~0; alt_state = 0; }
	} easing_menu;
	static TrackInfo const& curr_info() { return *info; }
	constexpr static size_t max_num_len = std::size(last_text);
	static size_t find_info_index(TrackInfo const* ptr_info) {
		return std::min<size_t>(ptr_info - exedit.trackinfo_left, ptr_info - exedit.trackinfo_right);
	}
	static void on_setfocus(TrackInfo const* track_info)
	{
		info = track_info;
		if (!easing_menu.is_active()) {
			::GetCursorPos(&mouse_pos_on_focused);
			::GetWindowTextW(track_info->hwnd_label, last_text, std::size(last_text));
		}
	}
	static void on_killfocus() { info = nullptr; }
	static bool is_focused() { return info != nullptr; }

	static bool add_number(int delta, bool clamp)
	{
		return modify_number([&](double val) {
			return val + static_cast<double>(delta) / info->precision();
		}, clamp);
	}
	static bool mult_number(double factor, bool clamp)
	{
		return modify_number([&](double val) {
			return val * factor;
		}, clamp);
	}
	static bool modify_number(auto&& modify, bool clamp)
	{
		wchar_t text[std::size(last_text)];
		auto len = ::GetWindowTextW(info->hwnd_label, text, std::size(text));
		if (len >= static_cast<int>(std::size(text)) - 1) return false;

		// parse the string as a number.
		double val;
		if (wchar_t* e; (val = std::wcstod(text, &e)) == HUGE_VAL ||
			(*e != L'\0' && *e != L' ')) return false;

		// apply the modification.
		val = modify(val);

		// doesn't clamp val into the min/max range by default,
		// as this is an edit control so the user may want to edit it afterward.
		if (clamp) val = std::clamp(val, info->value_min(), info->value_max());

		// track the position of the decimal point.
		int dec_diff = len;

		if (auto prec = info->precision(); prec > 1) {
			int dec_places = static_cast<int>(0.5 + std::log10(prec));

			// values have fraction part.
			if (auto pt = std::wcschr(text, L'.'); pt != nullptr)
				dec_diff = pt - text;
			dec_diff += 1 + dec_places;

			len = std::swprintf(text, std::size(text), L"%.*f", dec_places, val);
		}
		else {
			// values are integers.
			len = std::swprintf(text, std::size(text), L"%d", static_cast<int>(val));
		}
		if (len <= 0) return false;

		dec_diff -= len;

		// get the current selection, and adjust it
		// so the relative position to the decimal point is preserved.
		auto [sel_l, sel_r] = get_edit_selection(info->hwnd_label);
		sel_l -= dec_diff; sel_r -= dec_diff;

		// apply those changes.
		::SetWindowTextW(info->hwnd_label, text);

		// restore the selection.
		set_edit_selection(info->hwnd_label, std::clamp(sel_l, 0, len), std::clamp(sel_r, 0, len));

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
		//HCURSOR get(auto const& info)
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
// フィルタ効果のスクリプト名表示．
////////////////////////////////
struct FilterName : SettingDlg {
private:
#ifndef _DEBUG
	constinit // somehow it causes an error for debug build.
#endif
	// names of build-in animation effects.
	static inline class {
		std::vector<char const*> names{};
		char const* find(size_t idx) {
			// dynamically construct index -> name mapping.
			while (idx >= names.size()) {
				auto str = names.back();
				str += std::strlen(str) + 1;
				if (str[0] == '\0') return nullptr;
				names.push_back(str);
			}
			return names[idx];
		}
	public:
		// supply this struct with the heading pointer, turning the functionality enabled.
		void init(char const* head) {
			names.clear();
			names.push_back(head);
		}

		// retrieves the name of the script for the fiter at the given index of the given object.
		// the filter is known to have the specific type of exdata.
		template<class ExDataT>
		char const* get(ExEdit::Object const& leader, ExEdit::Object::FilterParam const& filter_param) {
			ptrdiff_t offset = leader.exdata_offset + filter_param.exdata_offset;
			auto exdata = reinterpret_cast<ExDataT*>((*exedit.exdata_table) + offset + 0x0004);
			return exdata->name[0] != '\0' ? exdata->name : find(exdata->type);
		}
	} basic_anim_names{}, basic_cust_obj_names{}, basic_cam_eff_names{}, basic_scn_change_names{};

	// the exdata structure for filters based on scripts.
	using anim_exdata = ExEdit::Exdata::efAnimationEffect; // also applies to camera eff and custom object.
	using scene_exdata = ExEdit::Exdata::efSceneChange;

	// exdata is only available for "mid-point-leading" objects.
	static ExEdit::Object& leading_object(ExEdit::Object& obj) {
		if (obj.index_midpt_leader < 0) return obj;
		else return (*exedit.ObjectArray_ptr)[obj.index_midpt_leader];
	}
	// return the script name of the filter at the given index in the object,
	// or nullptr if it's not a script-based filter, plus the filter id.
	static std::pair<char const*, filter_id::id> get_script_name(ExEdit::Object& obj, int32_t filter_index)
	{
		auto& leader = leading_object(obj);
		auto& filter_param = leader.filter_param[filter_index];

		if (filter_param.is_valid()) {
			switch (filter_param.id) {
			case filter_id::anim_eff:
				return { basic_anim_names.get<anim_exdata>(leader, filter_param), filter_id::anim_eff };

			case filter_id::cust_obj:
				return { basic_cust_obj_names.get<anim_exdata>(leader, filter_param), filter_id::cust_obj };

			case filter_id::cam_eff:
				return { basic_cam_eff_names.get<anim_exdata>(leader, filter_param), filter_id::cam_eff };

			case filter_id::scn_chg:
				return { basic_scn_change_names.get<scene_exdata>(leader, filter_param), filter_id::scn_chg };
			}
		}
		return { nullptr, filter_id::id{-1} };
	}

	// structure for cached formatted names and its rendered size.
	struct filter_name_cache {
		std::wstring caption = L"";
		int width = -1;	// width for the text only.

		bool is_valid() const { return width >= 0; }
		void init(HWND check_button, std::wstring const& text)
		{
			caption = text;

			// calculate the size for the text.
			RECT rc{};
			HDC dc = ::GetDC(check_button);
			auto old_font = ::SelectObject(dc, reinterpret_cast<HFONT>(
				::SendMessageW(check_button, WM_GETFONT, 0, 0)));
			::DrawTextW(dc, caption.c_str(), caption.size(), &rc, DT_CALCRECT);
			::SelectObject(dc, old_font);
			::ReleaseDC(check_button, dc);
			width = rc.right;
		}
		int wd_button() const { return width + extra_button_wd; }

		constexpr static int
			extra_button_wd = 18,	// width for check button adds this size.
			button_height = 14,
			gap_between_sep = 5,	// the separator leaves this margin.
			sep_height = 2;			// height for the separator.

	};
	// stores and manages caches.
	class cache_manager {
		std::map<std::string, filter_name_cache> cache;
		slim_formatter const formatter;
	public:
		cache_manager(std::wstring const& fmt) : formatter{ fmt }, cache{} {}
		filter_name_cache const& find_cache(char const* name, size_t idx) {
			auto& ret = cache[name];
			if (!ret.is_valid())
				ret.init(exedit.filter_checkboxes[idx], formatter(Encodes::to_wstring<CP_ACP>(name)));
			return ret;
		}
	};
	static inline constinit std::unique_ptr<cache_manager>
		anim_eff{},
		cust_std{}, cust_ext{}, cust_prtcl{},
		cam_eff{},
		scn_change{};

	// find or create a cached formatted names at the given filter index of the selected object.
	static filter_name_cache const* find_script_name_cache(size_t filter_index)
	{
		if (!is_valid()) return nullptr;

		auto idx = *exedit.SettingDialogObjectIndex;
		if (idx < 0) return nullptr;

		auto& obj = (*exedit.ObjectArray_ptr)[idx];
		auto [name, id] = get_script_name(obj, filter_index);
		if (name == nullptr) return nullptr;

		cache_manager* cache_source;
		switch (id) {
		case filter_id::anim_eff:
			cache_source = anim_eff.get(); break;
		case filter_id::cust_obj:
		{
			auto& draw_filter = obj.filter_param[obj.countFilters() - 1];
			switch (draw_filter.id) {
			case filter_id::draw_std:
				cache_source = cust_std.get(); break;
			case filter_id::draw_ext:
				cache_source = cust_ext.get(); break;
			case filter_id::particle:
				cache_source = cust_prtcl.get(); break;
			default: return nullptr;
			}
			break;
		}
		case filter_id::cam_eff:
			cache_source = cam_eff.get(); break;
		case filter_id::scn_chg:
			cache_source = scn_change.get(); break;
		default:
			std::unreachable();
		}
		return cache_source == nullptr ? nullptr :
			&cache_source->find_cache(name, filter_index);
	}

	// holding the current states of manipulation of the check buttons and the separators.
	static inline constinit struct hook_state {
		enum State : uint32_t {
			idle,
			renew,	// the filter itself is changing.
			manip,	// manipulating intentionally with this dll.
		} state = idle;

		filter_name_cache const* candidate = nullptr;
	} hook_states[ExEdit::Object::MAX_FILTER]{};

	// adjusts positions and sizes of the check button and the separator.
	// `hook_states[filter_index].state` must be set to `manip` before this call.
	static void adjust_layout(size_t filter_index)
	{
		auto cache = hook_states[filter_index].candidate;
		if (cache == nullptr) return; // won't occur.

		HWND const
			sep = exedit.filter_separators[filter_index],
			btn = exedit.filter_checkboxes[filter_index];

		// find the positions of the check button and the separator.
		RECT rc; ::GetWindowRect(sep, &rc);
		POINT sep_tl{ rc.left, rc.top }; ::ScreenToClient(get_hwnd(), &sep_tl);
		::GetWindowRect(btn, &rc);
		POINT btn_br{ rc.right, rc.bottom }; ::ScreenToClient(get_hwnd(), &btn_br);

		// adjust their sizes according to the precalculated width.
		::MoveWindow(btn, btn_br.x - cache->wd_button(), btn_br.y - cache->button_height,
			cache->wd_button(), cache->button_height, TRUE);
		::MoveWindow(sep, sep_tl.x, sep_tl.y,
			std::max<int>(btn_br.x - cache->wd_button() - cache->gap_between_sep - sep_tl.x, 0),
			cache->sep_height, TRUE);
	}

public:
	// should be called on WM_SETTEXT of the check button,
	// and replace lparam with the returned value if it's not nullptr.
	static wchar_t const* on_set_text(size_t filter_index)
	{
		// current state must be `idle`.
		auto& state = hook_states[filter_index];
		if (state.state != hook_state::idle) return nullptr;

		// it should be a supported filter.
		auto* cache = find_script_name_cache(filter_index);
		if (cache == nullptr) return nullptr;

		// then prepare the hook for the next call of WM_SIZE to the separator.
		state.state = hook_state::renew;
		state.candidate = cache;

		// return the tweaked caption.
		return cache->caption.c_str();
	}
	// should be called on WM_SIZE of the separator.
	static void on_sep_resized(size_t filter_index)
	{
		auto& state = hook_states[filter_index];
		if (state.state == hook_state::renew && state.candidate != nullptr) {
			state.state = hook_state::manip;
			adjust_layout(filter_index);
		}
		state.state = hook_state::idle;
	}

	// returns the index of the filter that handles the specified combo box,
	// which might be the one that selects animation types.
	static size_t idx_filter_from_combo_id(int id_combo)
	{
		constexpr auto invalid_return = std::numeric_limits<size_t>::max();

		// the setting dialog must be alive.
		if (!is_valid()) return invalid_return;

		// an object must be selected.
		auto const idx_obj = *exedit.SettingDialogObjectIndex;
		if (idx_obj < 0) return invalid_return;

		auto const& filter_param = (*exedit.ObjectArray_ptr)[idx_obj].filter_param;

		// find the index of checks.
		constexpr int id_combo_base = 8100;
		int const idx_combo = id_combo - id_combo_base;
		if (idx_combo < 0 || idx_combo >= ExEdit::Object::MAX_CHECK) return invalid_return;

		// traverse the filter to find the index.
		for (size_t i = 0; i < std::size(filter_param); i++) {
			auto const& filter = filter_param[i];
			int j = idx_combo - filter.check_begin;
			if (!filter.is_valid() || j < 0) break;
			switch (filter.id) {
			case filter_id::scn_chg:	if (j == 2) break; else continue;
			case filter_id::anim_eff:	if (j == 0 || j == 1) break; else continue;
			case filter_id::cust_obj:	if (j == 0 || j == 1) break; else continue;
			case filter_id::cam_eff:	if (j == 0 || j == 1) break; else continue;
			default: continue;
			}
			return i;
		}
		return invalid_return;
	}

	// should be called when a combo box changed it's selection,
	// after the default procedure was processed.
	// makes updates to the filter names if necessary.
	static void on_script_changed(size_t filter_index)
	{
		// the state must be `idle`
		auto& state = hook_states[filter_index];
		if (state.state != hook_state::idle) return;

		// it must be a supported filter.
		auto* cache = find_script_name_cache(filter_index);
		if (cache == nullptr) return;

		// then update the filter name and adjust the layout.
		state.state = hook_state::manip;

		state.candidate = cache;
		::SetWindowTextW(exedit.filter_checkboxes[filter_index], cache->caption.c_str());
		adjust_layout(filter_index);

		state.state = hook_state::idle;
	}

	// should be called to acquire the formatting string.
	static void init(std::unique_ptr<std::wstring> const& anim_eff_fmt,
		std::unique_ptr<std::wstring> const& cust_std_fmt, std::unique_ptr<std::wstring> const& cust_ext_fmt, std::unique_ptr<std::wstring> const& cust_prtcl_fmt,
		std::unique_ptr<std::wstring> const& cam_eff_fmt, std::unique_ptr<std::wstring> const& scn_change_fmt) {

		if (anim_eff_fmt)	anim_eff	= std::make_unique<cache_manager>(*anim_eff_fmt);
		if (cust_std_fmt)	cust_std	= std::make_unique<cache_manager>(*cust_std_fmt);
		if (cust_ext_fmt)	cust_ext	= std::make_unique<cache_manager>(*cust_ext_fmt);
		if (cust_prtcl_fmt)	cust_prtcl	= std::make_unique<cache_manager>(*cust_prtcl_fmt);
		if (scn_change_fmt)	scn_change	= std::make_unique<cache_manager>(*scn_change_fmt);
		if (cam_eff_fmt)	cam_eff		= std::make_unique<cache_manager>(*cam_eff_fmt);

		basic_anim_names.init(exedit.basic_animation_names);
		basic_cust_obj_names.init(exedit.basic_custom_obj_names);
		basic_cam_eff_names.init(exedit.basic_camera_eff_names);
		basic_scn_change_names.init(exedit.basic_scene_change_names);
	}
};


////////////////////////////////
// トラックバーの変化方法の調整．
////////////////////////////////
struct Easings : SettingDlg {
private:
	// source of script names.
	struct script_name {
		std::string_view name, dir;
		script_name(char const*& ptr) {
			std::string_view raw{ ptr };
			ptr += raw.size() + 1;

			// script names are stored in forms of either:
			//   i) "<name1>\x1<directory1>\0"
			//  ii) "<name>\0"
			if (auto pos = raw.find('\x01'); pos != std::string_view::npos) {
				name = raw.substr(0, pos);
				dir = raw.substr(pos + 1);
			}
			else {
				name = raw;
				dir = "";
			}
		}
	};
	static inline std::vector<script_name> script_names{};
	static void init_script_names() {
		if (script_names.empty()) {
			auto* ptr = exedit.track_script_names;
			while (*ptr != '\0') script_names.emplace_back(ptr);
			script_names.shrink_to_fit();
		}
	}
	// built-in easing names.
	constexpr static std::string_view const basic_track_mode_names[]{
		"移動無し",
		"直線移動",
		"曲線移動",
		"瞬間移動",
		"中間点無視",
		"移動量指定",
		"ランダム移動",
		"加減速移動",
		"反復移動",
	};
	struct easing_spec {
		bool speed, param, twopoints, loaded;
		constexpr easing_spec(uint32_t val)
			: speed		{ (val & flag_speed) != 0 }
			, param		{ (val & flag_param) != 0 }
			, twopoints	{ val >= 4 && val != 7 }
			, loaded	{ true } {}
		constexpr easing_spec(uint8_t val)
			: speed		{ (val & flag_speed) != 0 }
			, param		{ (val & flag_param) != 0 }
			, twopoints	{ (val & flag_twopoints) != 0 }
			, loaded	{ (val & flag_loaded) != 0 } {}

		constexpr static uint8_t
			flag_speed		= 1 << 0,
			flag_param		= 1 << 1,
			flag_twopoints	= 1 << 6,
			flag_loaded		= 1 << 7;
	};

	// formatting the tooltip text according to the track mode.
	static std::string format_tooltip(ExEdit::Object::TrackMode const& mode, int32_t param)
	{
		size_t const basic_idx = mode.num & 0x0f;
		bool const acc = (mode.num & mode.isAccelerate) != 0,
			dec = (mode.num & mode.isDecelerate) != 0,
			is_scr = basic_idx == mode.isScript;

		// name of the easing.
		std::string ret{ is_scr ?
			script_names[mode.script_idx].name :
			basic_track_mode_names[basic_idx] };

		// identify its specification, especially whether it accepts a parameter.
		easing_spec spec = is_scr ? easing_spec{ exedit.easing_specs_script[mode.script_idx] } :
			easing_spec{ exedit.easing_specs_builtin[basic_idx] };
		if (!spec.loaded) {
			// script wasn't loaded yet. try after loading.
			exedit.load_easing_spec(mode.script_idx, 0, 0);
			spec = { exedit.easing_specs_script[mode.script_idx] };
		}

		// integral parameter.
		if (spec.param)
			ret += "\nパラメタ: " + std::to_string(param);

		// two more booleans.
		if (acc || dec) {
			ret += spec.param ? ", " : "\n";
			if (acc) ret += "+加速 ";
			if (dec) ret += "+減速 ";
			ret.pop_back();
		}

		return ret;
	}

	static inline constinit HWND tooltip = nullptr;

public:
	// should be called when each trackbar button recieves messages,
	// and handles/updates tooltip states.
	static void relay_tooltip_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
		size_t idx, int32_t text_color)
	{
		if (tooltip == nullptr || *exedit.SettingDialogObjectIndex < 0) return;

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
					auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
					auto const& mode = obj.track_mode[idx];
					if (mode.num == 0) break; // 移動無し
					auto const param = obj.track_param[idx];

					// cached previous states.
					static constinit Object::TrackMode prev_mode = { -1, -1 };
					static constinit int32_t prev_param = 0;
				#ifndef _DEBUG
					constinit // somehow it causes an error for debug build.
				#endif
					static std::string curr_tooltip_text{};

					// store the string and return it.
					if (std::bit_cast<int32_t>(prev_mode) != std::bit_cast<int32_t>(mode) ||
						prev_param != param) {
						prev_mode = mode; prev_param = param;
						curr_tooltip_text = format_tooltip(mode, param);
					}
					reinterpret_cast<NMTTDISPINFOA*>(lparam)->lpszText = curr_tooltip_text.data();
					break;
				}

				case NM_CUSTOMDRAW:
				{
					// change the text color if specified.
					if (text_color < 0) break;

					auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
					if ((dhdr->nmcd.dwDrawStage & CDDS_PREPAINT) != 0)
						::SetTextColor(dhdr->nmcd.hdc,
							(std::rotl<uint32_t>(text_color, 16) & 0x00ff00ff) | (text_color & 0x0000ff00));
					break;
				}
				}
			}
			break;
		}
	}

	// creates a tooltip and associates it with relevant buttons.
	static void setup_tooltip(HWND hwnd, bool anim, uint16_t delay, uint16_t duration)
	{
		if (tooltip != nullptr) return;

		// initialize the name table of easing scripts.
		init_script_names();

		// create the tooltip window.
		tooltip = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | (anim ? TTS_NOFADE | TTS_NOANIMATE : 0),
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
		}

		// required for multi-line tooltips.
		::SendMessageW(tooltip, TTM_SETMAXTIPWIDTH, 0, (1 << 15) - 1);

		// settings of delay time for the tooltip.
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_INITIAL,	0xffff & delay);
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP,	0xffff & duration);
		::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_RESHOW,	0xffff & (delay / 5));
		// initial values are... TTDT_INITIAL: 340, TTDT_AUTOPOP: 3400, TTDT_RESHOW: 68.
	}
	static void term_tooltip()
	{
		if (tooltip != nullptr) {
			::DestroyWindow(tooltip);
			tooltip = nullptr;
		}
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
		constexpr auto calc_rate(modkeys keys, TrackInfo const& info) const
		{
			return (boost(keys) ? rate_boost : 1) * (decimal(keys) ? 1 : info.precision());
		}
	};
	struct shortcut_key {
		byte vkey;
		modkeys mkeys;
		constexpr bool match(byte code, modkeys keys) const {
			return is_enabled() && code == vkey && mkeys == keys;
		}
		constexpr bool is_enabled() const { return vkey != 0; }
	};

	struct {
		shortcut_key forward{ VK_TAB, modkeys::none }, backward{ VK_TAB, modkeys::shift };
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
		shortcut_key easing_menu;
		struct : shortcut_key {
			bool clamp;
		} negate;
		constexpr bool is_enabled() const { return updown || escape || easing_menu.is_enabled() || negate.is_enabled(); }
		constexpr bool no_wrong_keys(modkeys keys) const
		{
			return keys <= (keys_decimal | keys_boost);
		}
	} trackKbd{
		{ modkeys::shift, modkeys::alt, false, 10, },
		true, false, true, { 0, modkeys::none }, { 0, modkeys::none, false },
	};

	struct : modkey_set {
		bool wheel, reverse_wheel, cursor_react, fixed_drag;
		modkeys keys_activate;
		constexpr bool is_enabled() const { return wheel || fixed_drag; }
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
		true, false, true, false, modkeys::ctrl,
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

	struct {
		std::unique_ptr<std::wstring> anim_eff_fmt,
			cust_std_fmt, cust_ext_fmt, cust_particle_fmt,
			cam_eff_fmt, scn_change_fmt;
		constexpr static size_t max_fmt_length = 256;
		bool is_enabled() const {
			return anim_eff_fmt ||
				cust_std_fmt || cust_ext_fmt || cust_particle_fmt ||
				cam_eff_fmt || scn_change_fmt;
		}
	} filterName{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	struct {
		bool linked_track_invert_shift, wheel_click, tooltip, tip_anim;
		uint16_t tip_delay, tip_duration;
		int32_t tip_text_color; // 0xrrggbb or < 0 (not spcified).

		constexpr static uint16_t min_time = 0, max_time = 60'000;
		bool is_enabled() const { return wheel_click || tooltip; };
	} easings{ false, true, false, true, 0, 10'000, -1 };

	constexpr bool hooks_dialog() const
	{
		return textFocus.is_enabled() || textTweaks.is_enabled() ||
			trackKbd.is_enabled() || trackMouse.is_enabled() || trackBtn.is_enabled() ||
			dropdownKbd.search || filterName.is_enabled();
	}

	void load(char const* ini_file)
	{
		auto read_raw = [&](auto def, char const* section, char const* key) {
			return static_cast<decltype(def)>(
				::GetPrivateProfileIntA(section, key, static_cast<int32_t>(def), ini_file));
		};
		auto read_modkey = [&](modkeys def, char const* section, char const* key) {
			char str[std::bit_ceil(std::size("ctrl + shift + alt ****"))];
			::GetPrivateProfileStringA(section, key, def.canon_name(), str, std::size(str), ini_file);
			return modkeys{ str, def };
		};
		auto read_string = [&]<size_t max_len>(char8_t const* def, char const* section, char const* key, char8_t const* nil) {
			char str[max_len + 1];
			::GetPrivateProfileStringA(section, key, reinterpret_cast<char const*>(def), str, std::size(str), ini_file);
			if (std::strcmp(str, reinterpret_cast<char const*>(nil)) == 0)
				return std::unique_ptr<std::wstring>{ nullptr };
			return std::make_unique<std::wstring>(Encodes::to_wstring(str));
		};
	#define load_gen(head, tgt, section, read, write)	head##tgt = read##(read_raw(write##(head##tgt), section, #tgt))
	#define load_int(head, tgt, section)	load_gen(head, tgt, section, /* id */, /* id */)
	#define load_bool(head, tgt, section)	load_gen(head, tgt, section, \
				[](auto y) { return y != 0; }, [](auto x) { return x ? 1 : 0; })
	#define load_key(head, tgt, section)	head##tgt = read_modkey(head##tgt, section, #tgt)
	#define load_str0(head, tgt, section, def, max, nil)	head##tgt = read_string.operator()<max>(def, section, #tgt, nil)
	#define load_str(head, tgt, section, def, max)	load_str0(head, tgt, section, def, max, def)

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
		load_str (textTweaks., replace_tab_text,	"TextBox.Tweaks", u8"\t", textTweaks.max_replace_tab_length);
		load_str (textTweaks., replace_tab_script,	"TextBox.Tweaks", u8"\t", textTweaks.max_replace_tab_length);

		load_bool(trackKbd., updown,			"Track.Keyboard");
		load_bool(trackKbd., updown_clamp,		"Track.Keyboard");
		load_bool(trackKbd., escape,			"Track.Keyboard");
		load_key (trackKbd., keys_decimal,		"Track.Keyboard");
		load_key (trackKbd., keys_boost,		"Track.Keyboard");
		load_bool(trackKbd., def_decimal,		"Track.Keyboard");
		load_gen (trackKbd., rate_boost,		"Track.Keyboard",
			[](auto x) { return std::clamp(x, modkey_set::min_rate_boost, modkey_set::max_rate_boost); }, /* id */);
		load_int (trackKbd., easing_menu.vkey,	"Track.Keyboard");
		load_key (trackKbd., easing_menu.mkeys,	"Track.Keyboard");
		load_int (trackKbd., negate.vkey,		"Track.Keyboard");
		load_key (trackKbd., negate.mkeys,		"Track.Keyboard");
		load_bool(trackKbd., negate.clamp,		"Track.Keyboard");

		load_bool(trackMouse., wheel,			"Track.Mouse");
		load_bool(trackMouse., reverse_wheel,	"Track.Mouse");
		load_bool(trackMouse., cursor_react,	"Track.Mouse");
		load_bool(trackMouse., fixed_drag,		"Track.Mouse");
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

		load_str0(filterName., anim_eff_fmt,	"FilterName", u8"{}(アニメーション効果)", filterName.max_fmt_length, u8"");
		load_str0(filterName., cust_std_fmt,	"FilterName", u8"{}(カスタムオブジェクト)[標準描画]", filterName.max_fmt_length, u8"");
		load_str0(filterName., cust_ext_fmt,	"FilterName", u8"{}(カスタムオブジェクト)[拡張描画]", filterName.max_fmt_length, u8"");
		load_str0(filterName., cust_particle_fmt, "FilterName", u8"{}(カスタムオブジェクト)[パーティクル出力]", filterName.max_fmt_length, u8"");
		load_str0(filterName., cam_eff_fmt,		"FilterName", u8"{}(カメラ効果)", filterName.max_fmt_length, u8"");
		load_str0(filterName., scn_change_fmt,	"FilterName", u8"{}(シーンチェンジ)", filterName.max_fmt_length, u8"");

		load_bool(easings.,	linked_track_invert_shift,	"Easings");
		load_bool(easings.,	wheel_click,		"Easings");
		load_bool(easings.,	tooltip,			"Easings");
		load_bool(easings.,	tip_anim,			"Easings");
		load_gen (easings.,	tip_delay,			"Easings",
			[](auto x) { return std::clamp<int>(x, decltype(easings)::min_time, decltype(easings)::max_time); }, /* id */);
		load_gen (easings.,	tip_duration,		"Easings",
			[](auto x) { return std::clamp<int>(x, decltype(easings)::min_time, decltype(easings)::max_time); }, /* id */);
		load_int(easings.,	tip_text_color,		"Easings");

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
static void replace_tail(T(&dst)[len_max], size_t len, T const(&tail_old)[len_old], T const(&tail_new)[len_new])
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
	static constexpr struct { ID id; char const* name; } Items[] = {
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

// 指定キーでトラックバーの数値を符号反転する機能．
inline bool flip_on_label(byte vkey)
{
	if (!settings.trackKbd.negate.match(vkey, curr_modkeys()) ||
		key_pressed_any(VK_LWIN, VK_RWIN)) return false;

	if (!TrackLabel::is_focused()) [[unlikely]] return false;

	if (!TrackLabel::mult_number(-1, settings.trackKbd.negate.clamp)) {
		// in case of text errors.
		return false;
	}
	return true;
}

// ESC キーで編集前に戻す / フォーカスを外す機能．
inline bool escape_from_label()
{
	if (!TrackLabel::is_focused()) [[unlikely]] return false;

	// combinations of any modifier keys shall not take action.
	if (key_pressed_any(VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN)) return false;

	if (TrackLabel::revert()) return true; // reverted the content, the focus stays.
	return ::SetFocus(exedit.fp->hwnd) != nullptr; // no need to revert, the focus moves away.
}

// トラックバー変化方法の選択メニューを表示する機能．
inline bool popup_easing_menu(byte vkey)
{
	if (!settings.trackKbd.easing_menu.match(vkey, curr_modkeys()) ||
		key_pressed_any(VK_LWIN, VK_RWIN)) return false;

	if (!TrackLabel::is_focused()) [[unlikely]] return false;

	auto& info = TrackLabel::curr_info();
	auto const idx = TrackLabel::find_info_index(&info);
	if (idx >= Object::MAX_TRACK) return false;

	// suppress notification sound by discarding certain messages.
	discard_message(info.hwnd_label, settings.trackKbd.easing_menu.mkeys
		.has_flags(modkeys::alt) ? WM_SYSCHAR : WM_CHAR);

	// save the index so it the hook is enabled.
	TrackLabel::easing_menu.activate(idx);

	// move off the focus form this edit box,
	// otherwise it would swallow the input of Enter key.
	auto [l_pos, r_pos] = get_edit_selection(info.hwnd_label);
	wchar_t last_text[TrackLabel::max_num_len];
	::GetWindowTextW(info.hwnd_label, last_text, std::size(last_text));
	::SetFocus(*exedit.hwnd_setting_dlg);

	if (settings.trackKbd.easing_menu.mkeys.has_flags(modkeys::alt))
		// as alt+clicking the button has a different functionality,
		// make the key recognized as released.
		TrackLabel::easing_menu.alt_state = set_key_state(VK_MENU, 0);
	// send the button-click notification.
	::SendMessageW(*exedit.hwnd_setting_dlg, WM_COMMAND,
		(BN_CLICKED << 16) | TrackLabel::button_id(idx),
		reinterpret_cast<LPARAM>(exedit.hwnd_track_buttons[idx]));

	// rewind the focus and other states.
	if ((::GetWindowLongW(info.hwnd_label, GWL_STYLE) & WS_DISABLED) == 0) {
		::SetFocus(info.hwnd_label);
		::SetWindowTextW(info.hwnd_label, last_text);
		set_edit_selection(info.hwnd_label, l_pos, r_pos);

		// turn out of the hooking state.
		TrackLabel::easing_menu.deactivate();
	}
	else {
		// if it's right side and disabled now, focus on left instead.
		auto left_one = exedit.trackinfo_left[idx].hwnd_label;

		TrackLabel::easing_menu.deactivate(); // no more hooks.
		::SetFocus(left_one);
		set_edit_selection_all(left_one);
	}

	return true;
}

// ホイールでトラックバーの数値をテキストラベル上で増減する機能．
inline bool wheel_on_track(TrackInfo const& info, short wheel, modkeys keys)
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
inline void modify_updown_delta(TrackInfo const& info, int& delta)
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
			constexpr int diff = WM_CHAR - WM_KEYDOWN;
			static_assert(diff == WM_SYSCHAR - WM_SYSKEYDOWN);
			discard_message(hwnd, message + diff);

			// shouldn't pass to ::DefSubclassProc(). (for TAB key, takes effect only when ctrl is pressed.)
			return 0;
		}
		break;

	case WM_CHAR:
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

	case WM_KILLFOCUS:
		::RemoveWindowSubclass(hwnd, text_box_hook, id);
		break;
	}

	if (settings.textTweaks.hide_cursor) {
		switch (message) {
		case WM_SETCURSOR:
			// hide the cursor if specified.
			if (settings.textTweaks.hide_cursor && TextBox::hide_cursor.is_hidden()) {
				::SetCursor(nullptr);
				return FALSE; // prioritize the parent window's choice of the cursor shape.
			}
			break;

		case WM_CHAR:
		case WM_IME_COMPOSITION:
			if (!TextBox::hide_cursor.is_hidden()) {
				// hide the cursor on keyboard inputs.
				POINT pt;
				::GetCursorPos(&pt);
				::ScreenToClient(hwnd, &pt);
				TextBox::hide_cursor.on_edit(pt);
				if (pt.x >= 0 && pt.y >= 0) {
					RECT rc;
					::GetClientRect(hwnd, &rc);
					if (pt.x < rc.right && pt.y < rc.bottom)
						::SendMessageW(hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), HTCLIENT | (message << 16));
				}
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_KILLFOCUS:
			// show again the mouse cursor on clicks and losing focus.
			TextBox::hide_cursor.reset();
			::SendMessageW(hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), HTCLIENT | (message << 16));
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			// show again the mouse cursor on buttons up too.
			TextBox::hide_cursor.reset();
			break;

		case WM_MOUSEMOVE:
			// show again the mouse cursor on move.
			TextBox::hide_cursor.on_move({ .x = static_cast<int16_t>(lparam & 0xffff), .y = static_cast<int16_t>(lparam >> 16) });
			break;
		}

	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK track_label_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (byte vkey = static_cast<byte>(wparam);
			settings.trackKbd.updown && delta_move_on_label(vkey)) {
			// prevent calling the default window procedure.
			return 0;
		}
		else if ((settings.trackKbd.escape && vkey == VK_ESCAPE && escape_from_label())
			|| (settings.trackKbd.negate.is_enabled() && flip_on_label(vkey))) {
			// needs to remove WM_CHAR messages to avoid notification sounds or unnecessary inputs.
			constexpr int diff = WM_CHAR - WM_KEYDOWN;
			static_assert(diff == WM_SYSCHAR - WM_SYSKEYDOWN);
			discard_message(hwnd, message + diff);
			return 0;
		}
		else if (settings.trackKbd.easing_menu.is_enabled() && popup_easing_menu(vkey))
			return 0;
		break;
	case WM_MOUSEMOVE:
		if (settings.trackMouse.fixed_drag &&
			*exedit.track_label_is_dragging != 0 && ::GetCapture() == hwnd) {
			POINT pt; ::GetCursorPos(&pt);
			if (pt.x != TrackLabel::mouse_pos_on_focused.x || pt.y != TrackLabel::mouse_pos_on_focused.y) {
				// rewind the cursor position.
				::SetCursorPos(TrackLabel::mouse_pos_on_focused.x, TrackLabel::mouse_pos_on_focused.y);

				// "fake" the starting point of the drag.
				*exedit.track_label_start_drag_x -= pt.x - TrackLabel::mouse_pos_on_focused.x;
			}

			// hide the cursor.
			::SetCursor(nullptr);
		}
		break;
	case WM_KILLFOCUS:
		TrackLabel::on_killfocus();
		::RemoveWindowSubclass(hwnd, track_label_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

// トラックバー変化方法のメニューをショートカットキーで表示させたときの座標調整．
BOOL WINAPI popup_easing_menu_hook(HMENU menu, UINT flags, int x, int y, int, HWND hwnd, RECT const*)
{
	if (TrackLabel::easing_menu.is_active()) {
		auto const btn = exedit.hwnd_track_buttons[TrackLabel::easing_menu.idx];

		// Alt の状態を差し戻す．
		set_key_state(VK_MENU, TrackLabel::easing_menu.alt_state);

		// メニューの位置をボタンの矩形に合わせて調整．
		TPMPARAMS p{ .cbSize = sizeof(p) };
		::GetWindowRect(btn, &p.rcExclude);

		// メニュー表示．
		return ::TrackPopupMenuEx(menu, flags | TPM_VERTICAL,
			p.rcExclude.left, p.rcExclude.top, hwnd, &p);
	}

	// 条件を満たさない場合は未介入のデフォルト処理．
	return ::TrackPopupMenuEx(menu, flags, x, y, hwnd, nullptr);
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

				else if (TrackInfo const* info;
					(settings.trackKbd.is_enabled() || settings.trackMouse.fixed_drag) &&
					(info = TrackLabel::find_trackinfo(wparam & 0xffff, ctrl)) != nullptr) {
					// hook for manipulating values by arrow keys.
					TrackLabel::on_setfocus(info);
					::SetWindowSubclass(ctrl, track_label_hook, TrackLabel::hook_uid(), 0);
				}
				break;

			case EN_CHANGE:
				if (settings.textTweaks.batch &&
					!TextBox::batch.is_calling() &&
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

				// 選択スクリプトの変更を監視．
			case CBN_SELCHANGE:
				if (settings.filterName.is_enabled() && check_window_class(ctrl, WC_COMBOBOXW)) {
					if (auto idx = FilterName::idx_filter_from_combo_id(wparam & 0xffff);
						idx < ExEdit::Object::MAX_FILTER) {

						auto ret = ::DefSubclassProc(hwnd, message, wparam, lparam);
						FilterName::on_script_changed(idx);
						return ret;
					}
				}
				break;
			}
		}
		break;
	case WM_MOUSEWHEEL:
		if (settings.trackMouse.wheel &&
			*exedit.SettingDialogObjectIndex >= 0) {
			modkeys keys{ (wparam & MK_CONTROL) != 0, (wparam & MK_SHIFT) != 0, key_pressed_any(VK_MENU) };

			// check for the activation key.
			if (!settings.trackMouse.is_activated(keys) ||
				key_pressed_any(VK_LWIN, VK_RWIN)) break;

			// find the pointer position.
			POINT pt = { static_cast<int16_t>(lparam & 0xffff), static_cast<int16_t>(lparam >> 16) };
			::ScreenToClient(hwnd, &pt);

			// if the track is found, make an attempt to move the value.
			if (auto* info = (TrackLabel::is_focused() && *exedit.track_label_is_dragging != 0 && ::GetCapture() != nullptr) ?
				&TrackLabel::curr_info() : TrackLabel::find_trackinfo(pt);
				info != nullptr &&
				wheel_on_track(*info, static_cast<int16_t>(wparam >> 16), keys))
				return 0; // successfully moved the value.

			// default behavior otherwise.
		}
		break;
	case WM_SETCURSOR:
		if (settings.trackMouse.wheel && settings.trackMouse.cursor_react &&
			*exedit.SettingDialogObjectIndex >= 0) {
			// check if it's not in a dragging state.
			if (*exedit.track_label_is_dragging != 0 && ::GetCapture() != nullptr)
				return TRUE;

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

LRESULT CALLBACK trackbar_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
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

// アニメーション効果のスクリプト名表示．
LRESULT CALLBACK filter_name_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_SETTEXT:
		// wparam: not used, lparam: reinterpret_cast<wchar_t const*>(lparam) is the new text.
		if (size_t idx = static_cast<int>(data); idx < ExEdit::Object::MAX_FILTER) {
			if (auto new_name = FilterName::on_set_text(idx); new_name != nullptr)
				lparam = reinterpret_cast<LPARAM>(new_name);
		}
		break;
	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, trackbar_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
// アニメーション効果のスクリプト名表示によるレイアウト変更．
LRESULT CALLBACK filter_separator_hook(HWND hwnd, UINT message, WPARAM w, LPARAM l, auto id, auto data)
{
	switch (message) {
	case WM_SIZE:
		if (size_t idx = static_cast<int>(data); idx < ExEdit::Object::MAX_FILTER)
			FilterName::on_sep_resized(idx);
		break;
	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, filter_separator_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, w, l);
}
// トラックバー変化方法指定のボタンのカスタム動作．
LRESULT CALLBACK param_button_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	auto const idx = static_cast<size_t>(data);

	// handle tooltip if exists.
	if (settings.easings.tooltip)
		Easings::relay_tooltip_message(hwnd, message, wparam, lparam, idx, settings.easings.tip_text_color);

	switch (message) {
	case WM_MBUTTONUP:
		// customized behavior of wheel click.
		if (settings.easings.wheel_click && *exedit.SettingDialogObjectIndex >= 0) {
			// send a command to edit the easing parameter.
			constexpr uint16_t param_set_command = 1122;
			::PostMessageW(*exedit.hwnd_setting_dlg, WM_COMMAND, param_set_command, static_cast<LPARAM>(idx));
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, param_button_hook, id);
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
	auto conflict_warning = [&](std::string auf, char const* ini) {
		::MessageBoxA(fp->hwnd, (auf + " と競合しているため一部機能を無効化しました．\n"
			"次回以降このメッセージを表示させないためには，設定ファイルで以下の設定をしてください:\n\n"
			+ ini).c_str(), fp->name, MB_OK | MB_ICONEXCLAMATION);
	};
	if (settings.trackBtn.is_enabled() && ::GetModuleHandleW(L"updown.auf") != nullptr) {
		settings.trackBtn.modify = false;
		conflict_warning("updown.auf", "[Track.Button]\nmodify=0");
	}
	if (settings.filterName.is_enabled() && ::GetModuleHandleW(L"filter_name.auf") != nullptr) {
		settings.filterName.anim_eff_fmt.reset();
		settings.filterName.cust_std_fmt.reset();
		settings.filterName.cust_ext_fmt.reset();
		settings.filterName.cust_particle_fmt.reset();
		settings.filterName.cam_eff_fmt.reset();
		settings.filterName.scn_change_fmt.reset();
		conflict_warning("filter_name.auf",
			"[FilterName]\n"
			"anim_eff_fmt=\"\"\n"
			"cust_std_fmt=\"\"\n"
			"cust_ext_fmt=\"\"\n"
			"cust_particle_fmt=\"\"\n"
			"cam_eff_fmt=\"\"\n"
			"scn_change_fmt=\"\"");
	}

	// トラックバー変化方法のメニューをショートカットキーで表示させたときの座標調整．
	if (settings.trackKbd.easing_menu.is_enabled())
		// ff 15 48 a3 ff 2f	call	dword ptr ds:[TrackPopupMenu]
		// V
		// e8 yy yy yy yy		call	yyyyyyyy
		// 90					nop
		memory::hook_api_call(exedit.call_easing_popup_menu, popup_easing_menu_hook);

	// トラックバーの変化方法の調整．
	if (settings.easings.linked_track_invert_shift)
		// 0f 8c 87 00 00 00	jl
		// V
		// 0f 8d 87 00 00 00	jge
		memory::ProtectHelper::write(exedit.cmp_shift_state_easing + 1, byte{ 0x8d });

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, EditHandle* editp, FilterPlugin* fp)
{
	using Message = FilterPlugin::WindowMessage;
	switch (message) {
		// 拡張編集設定ダイアログ等のフック/アンフック．
	case Message::ChangeWindow:
		if (settings.hooks_dialog() && SettingDlg::is_valid()) {
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

			if (settings.filterName.is_enabled()) {
				FilterName::init(settings.filterName.anim_eff_fmt,
					settings.filterName.cust_std_fmt, settings.filterName.cust_ext_fmt, settings.filterName.cust_particle_fmt,
					settings.filterName.cam_eff_fmt, settings.filterName.scn_change_fmt);
				for (size_t i = 0; i < ExEdit::Object::MAX_FILTER; i++) {
					::SetWindowSubclass(exedit.filter_checkboxes[i], filter_name_hook, FilterName::hook_uid(), { i });
					::SetWindowSubclass(exedit.filter_separators[i], filter_separator_hook, FilterName::hook_uid(), { i });
				}
			}
		}
		if (settings.easings.is_enabled()) {
			for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++)
				::SetWindowSubclass(exedit.hwnd_track_buttons[i], param_button_hook, Easings::hook_uid(), { i });
			if (settings.easings.tooltip)
				Easings::setup_tooltip(hwnd, settings.easings.tip_anim,
					settings.easings.tip_delay, settings.easings.tip_duration);
		}
		break;
	case Message::Exit:
		// at this moment, the setting dialog is already destroyed.
		TextBox::batch.discard(hwnd, PrvMsg::NotifyUpdate);
		KeyboardHook::unhook();
		TrackLabel::cursor.free();
		if (settings.easings.tooltip) Easings::term_tooltip();

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
#define PLUGIN_VERSION	"v1.82-beta3"
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
