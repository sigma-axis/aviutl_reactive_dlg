/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <bit>
#include <concepts>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>

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
	constexpr static size_t max_value_len = 9; // assume "-999999.9" is the longest.
};
static_assert(sizeof(TrackInfo) == 40);

struct CheckAndButton {
	int32_t val_check; // 0: unchecked, 1: checked. 0 for button or combo.
	int32_t val_default; // 0: unchecked, 1: checked, -1: button, -2: combo.
	HWND hwnd_check;
	HWND hwnd_static;
	HWND hwnd_button;
	HWND hwnd_combo;
};
static_assert(sizeof(CheckAndButton) == 24);

inline constinit struct ExEdit092 {
	AviUtl::FilterPlugin* fp;
	bool init(AviUtl::FilterPlugin* this_fp);

	//int32_t*	ObjectAllocNum;				// 0x1e0fa0
	ExEdit::Object**	ObjectArray_ptr;	// 0x1e0fa4; Object(*)[].
	int32_t*	NextObjectIdxArray;			// 0x1592d8
	int32_t*	SettingDialogObjectIndex;	// 0x177a10
	int32_t*	SelectingObjectNum_ptr;		// 0x167d88
	int32_t*	SelectingObjectIndex;		// 0x179230
	int32_t*	edit_frame_cursor;			// 0x1a5304
	ExEdit::SceneSetting*	scene_settings;	// 0x177a50

	HWND*		hwnd_setting_dlg;			// 0x1539c8
	//int32_t*	is_playing;					// 0x1a52ec; 0: editing, 1: playing.
	HWND*		hwnd_edit_text;				// 0x236328
	HWND*		hwnd_edit_script;			// 0x230c78
	HWND*		hwnd_track_buttons;			// 0x158f68
	HWND*		filter_checkboxes;			// 0x14d368
	HWND*		filter_separators;			// 0x1790d8
	HWND*		filter_expanders;			// 0x178fcc

	HMENU*		hmenu_cxt_visual_obj;		// 0x158d2c
	/*
			0x158d2c, // 図形やテキストオブジェクト，グループ制御など．
			0x158d24, // 音声系オブジェクト．
			0x167d40, // カメラ制御と時間制御．
			0x158d20, // 画像系フィルタオブジェクト（シーンチェンジなど）．
			0x167d44, // 音声系フィルタオブジェクト．
	*/

	//WNDPROC		setting_dlg_wndproc;		// 0x02cde0
	TrackInfo*	trackinfo_left;				// 0x14d4c8
	TrackInfo*	trackinfo_right;			// 0x14def0
	CheckAndButton*	checks_buttons;			// 0x168a18
	int32_t*	track_label_is_dragging;	// 0x158d30; 0: idle, 1: dragging.
	int32_t*	track_label_start_drag_x;	// 0x179218
	int32_t*	current_filter_index;		// 0x14965c

	uintptr_t*	exdata_table;				// 0x1e0fa8
	char const*	basic_scene_change_names;	// 0x0aef38
	char const*	basic_animation_names;		// 0x0c1f08
	char const* track_script_names;			// 0x0ca010
	char const*	basic_custom_obj_names;		// 0x0ce090
	char const*	basic_camera_eff_names;		// 0x0d20d0
	uint32_t*	easing_specs_builtin;		// 0x0b8390
	uint8_t*	easing_specs_script;		// 0x231d90

	uintptr_t	cmp_shift_state_easing;		// 0x02ca90
	uintptr_t	push_cursor_track_drag;		// 0x02f4c6
	byte*		call_easing_popup_menu;		// 0x02d4b5

	// index: index of the script; zero1, zero2: zero, unknown otherwise.
	void(*load_easing_spec)(int32_t index, int32_t zero1, int32_t zero2); // 0x087940

	ExEdit::Filter**	loaded_filter_table;// 0x187c98; Filter*[].
	//int32_t*	loaded_filter_count;		// 0x146248

	// communicating with the undo buffer.
	void(*nextundo)();						// 0x08d150
	void(*setundo)(uint32_t, uint32_t);		// 0x08d290

	// access to the functions for filter related calculations.
	// same as ExEdit::Filter::exfunc->calc_trackbar.
	BOOL(*calc_trackbar)(ExEdit::ObjectFilterIndex, int32_t, int32_t, int32_t*, char*); // 0x04d040

	// updating the setting dialog.
	void(*update_setting_dlg)(int32_t idx_object); // 0x0305e0

private:
	void init_pointers(HINSTANCE dll_hinst, HINSTANCE hinst_parent)
	{
		auto pick_addr = [exedit_base=reinterpret_cast<uintptr_t>(dll_hinst)]
			<class T>(T& target, ptrdiff_t offset) { target = std::bit_cast<T>(exedit_base + offset); };
		//pick_addr(ObjectAllocNum,			0x1e0fa0);
		pick_addr(ObjectArray_ptr,			0x1e0fa4);
		pick_addr(NextObjectIdxArray,		0x1592d8);
		pick_addr(SettingDialogObjectIndex,	0x177a10);
		pick_addr(SelectingObjectNum_ptr,	0x167d88);
		pick_addr(SelectingObjectIndex,		0x179230);
		pick_addr(edit_frame_cursor,		0x1a5304);
		pick_addr(scene_settings,			0x177a50);

		pick_addr(hwnd_setting_dlg,			0x1539c8);
		//pick_addr(is_playing,				0x1a52ec);
		pick_addr(hwnd_edit_text,			0x236328);
		pick_addr(hwnd_edit_script,			0x230c78);
		pick_addr(hwnd_track_buttons,		0x158f68);
		pick_addr(filter_checkboxes,		0x14d368);
		pick_addr(filter_separators,		0x1790d8);
		pick_addr(filter_expanders,			0x178fcc);

		pick_addr(hmenu_cxt_visual_obj,		0x158d2c);

		//pick_addr(setting_dlg_wndproc,		0x02cde0);
		pick_addr(trackinfo_left,			0x14d4c8);
		pick_addr(trackinfo_right,			0x14def0);
		pick_addr(checks_buttons,			0x168a18);
		pick_addr(track_label_is_dragging,	0x158d30);
		pick_addr(track_label_start_drag_x,	0x179218);
		pick_addr(current_filter_index,		0x14965c);

		pick_addr(exdata_table,				0x1e0fa8);
		pick_addr(basic_scene_change_names,	0x0aef38);
		pick_addr(basic_animation_names,	0x0c1f08);
		pick_addr(track_script_names,		0x0ca010);
		pick_addr(basic_custom_obj_names,	0x0ce090);
		pick_addr(basic_camera_eff_names,	0x0d20d0);
		pick_addr(easing_specs_builtin,		0x0b8390);
		pick_addr(easing_specs_script,		0x231d90);

		pick_addr(cmp_shift_state_easing,	0x02ca90);
		pick_addr(push_cursor_track_drag,	0x02f4c6);
		pick_addr(call_easing_popup_menu,	0x02d4b5);
		pick_addr(load_easing_spec,			0x087940);

		pick_addr(loaded_filter_table,		0x187c98);
		//pick_addr(loaded_filter_count,		0x146248);

		pick_addr(nextundo,					0x08d150);
		pick_addr(setundo,					0x08d290);

		pick_addr(calc_trackbar,			0x04d040);

		pick_addr(update_setting_dlg,		0x0305e0);
	}
} exedit;

// 設定ダイアログのデータ表示を更新．
void update_setting_dialog(int index);

// 編集データの変更でメイン画面を更新．
void update_current_frame();

template<class ExDataT>
inline ExDataT* find_exdata(ptrdiff_t obj_exdata_offset, ptrdiff_t filter_exdata_offset) {
	return reinterpret_cast<ExDataT*>((*exedit.exdata_table)
		+ obj_exdata_offset + filter_exdata_offset + 0x0004);
}

namespace filter_id
{
	enum id : int32_t {
		draw_std = 0x0a, // 標準描画
		draw_ext = 0x0b, // 拡張描画
		play_std = 0x0c, // 標準再生
		particle = 0x0d, // パーティクル出力

		scn_chg = 0x0e, // シーンチェンジ
		anim_eff = 0x4f, // アニメーション効果
		cust_obj = 0x50, // カスタムオブジェクト
		cam_eff = 0x61, // カメラ効果
	};
}


////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////
template<int N>
inline bool check_window_class(HWND hwnd, wchar_t const(&classname)[N])
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

inline sigma_lib::modifier_keys::modkeys curr_modkeys() {
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
// 内部メッセージ．
////////////////////////////////
namespace PrivateMsg
{
	enum : uint32_t {
		// wparam: versatile parameter.
		// lparam: function pointer of the type `void(*)(HWND, WPARAM)`.
		RequestCallback = WM_USER + 1,
	};
	using callback_fnptr = void(__cdecl*)(HWND, WPARAM);
}


////////////////////////////////
// 競合通知メッセージ．
////////////////////////////////
/// @return `true` if the module was found and the confliction message was displayed, `false` otherwise.
bool warn_conflict(wchar_t const* module_name, wchar_t const* ini_piece, char const* this_plugin_name);
