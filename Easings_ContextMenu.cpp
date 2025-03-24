/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cmath>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "inifile_op.hpp"
#include "clipboard.hpp"

#include "reactive_dlg.hpp"
#include "Easings.hpp"
#include "Easings_ContextMenu.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// 変化方法のツールチップ表示．
////////////////////////////////
using namespace reactive_dlg::Easings;
using namespace reactive_dlg::Easings::ContextMenu;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

struct track_info {
	int denom, prec, min, max;
	constexpr bool has_easing() const { return !is_none; }
	constexpr bool is_ease_step() const { return is_step; }
	constexpr bool is_ease_twopoints() const { return is_twopoints; }

private:
	bool is_none, is_step, is_twopoints;
	constexpr static int
		idx_easing_none = 0,
		idx_easing_step = 3; // 「瞬間移動」

public:
	track_info(TrackInfo const& src, ExEdit::Object::TrackMode mode)
		: denom{ src.denominator() }
		, prec{ src.precision() }
		, min{ src.val_int_min }
		, max{ src.val_int_max }
		, is_none{ (mode.num & 0x0f) == idx_easing_none }
		, is_step{ (mode.num & 0x0f) == idx_easing_step }
		, is_twopoints{ easing_name_spec(mode).spec.twopoints } {}
	int to_internal(double val) const {
		return convert_value_disp2int(val, denom, prec, min, max);
	}
};
struct cmd_base {
	track_info const& info;

protected:
	// appends a menu item with a formatted title.
	static void append_menu(HMENU menu, uint32_t id, bool grayed, wchar_t const* fmt, auto... params)
	{
		if constexpr (sizeof...(params) > 0) {
			wchar_t buf[256];
			::swprintf_s(buf, std::size(buf), fmt, params...);
			append_menu(menu, id, grayed, buf);
		}
		else ::AppendMenuW(menu, MF_STRING | (grayed ? MF_GRAYED : 0), id, fmt);
	}
	static void push_undo(ExEdit::Object const& obj)
	{
		exedit.nextundo();
		exedit.setundo(&obj - (*exedit.ObjectArray_ptr), 0x01); // 0x01: the entire chain.
	}

	// quickly determines whether there is a left object in the midpoint-chain.
	static bool has_adjacent_left(ExEdit::Object const& obj) {
		int j = obj.index_midpt_leader;
		return j >= 0 && &obj != &(*exedit.ObjectArray_ptr)[j];
	}
	bool has_valid_left(ExEdit::Object const& obj) const {
		return !info.is_ease_twopoints() && has_adjacent_left(obj);
	}
	// quickly determines whether there is a right object in the midpoint-chain.
	static bool has_adjacent_right(ExEdit::Object const& obj) {
		return obj.index_midpt_leader >= 0 &&
			exedit.NextObjectIdxArray[&obj - *exedit.ObjectArray_ptr] >= 0;
	}
	bool has_valid_right(ExEdit::Object const& obj) const {
		return !info.is_ease_twopoints() && has_adjacent_right(obj);
	}
	// quickly determines whether there is a left or right object in the midpoint-chain.
	static bool has_adjacent_sections(ExEdit::Object const& obj) {
		return obj.index_midpt_leader >= 0;
	}
	bool has_valid_sections(ExEdit::Object const& obj) const {
		return !info.is_ease_twopoints() && has_adjacent_sections(obj);
	}
};

struct copy : cmd_base {
	bool append(HMENU menu, uint32_t id)
	{
		append_menu(menu, id, false, L"数値をコピー (&C)");
		return true;
	}

	void execute(ExEdit::Object& obj, size_t idx_track) const
	{
		sigma_lib::W32::clipboard::write(
			formatted_values{ obj, idx_track }.span().to_string(info.prec, false, false, false));
	}
};

struct modify_base : cmd_base {
	void execute(this auto const& self, ExEdit::Object& obj, size_t idx_track)
	{
		push_undo(obj);

		auto [pos, chain] = collect_pos_chain(obj);
		auto values = collect_int_values(chain, idx_track);
		self.modify_values(pos, values);
		apply_int_values(chain, values, idx_track);
	}
};
struct paste_base : modify_base {
	constexpr static auto root_title = L"数値を貼り付け";
	formatted_valuespan list;
};
struct paste_unique : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (info.has_easing()) return false;

		list = list.has_section_full() ? list.trim_from_sect_l(0) : list;
		list = list.trim_from_end(0, list.size() - 1);
		list.discard_section();
		append_menu(menu, id, false, L"%s (&V) (%s)", root_title,
			list.to_string(info.prec, false, false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set all values uniformly.
		values.front() = convert_value_disp2int(list.values.front(), info.denom, info.prec, info.min, info.max);
	}
};

struct paste_two : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect(0, 0) : list.trim_from_end(0, list.size() - 2);
		list.discard_section();
		append_menu(menu, id, false, L"区間の左右 (%s)",
			list.to_string(info.prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set values on the left and the right.
		int val_l = info.to_internal(list.values.front()),
			val_r = info.to_internal(list.values.back());
		if (values.size() > 2) {
			values[pos] = val_l; values[pos + 1] = val_r;
		}
		else { values.front() = val_l; values.back() = val_r; }
	}
};

struct paste_left : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect_l(0) : list;
		list = list.trim_from_end(0, list.size() - 1);
		list.discard_section();
		append_menu(menu, id, false, L"区間の左だけ (%s)",
			list.to_string(info.prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the value on the left track.
		int val = info.to_internal(list.values.front());
		if (values.size() > 2) values[pos] = val;
		else values.front() = val;
	}
};

struct paste_right : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect_r(0) : list;
		list = list.trim_from_end(list.size() - 1, 0);
		list.discard_section();
		append_menu(menu, id, false, L"区間の右だけ (%s)",
			list.to_string(info.prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the value on the right track.
		int val = info.to_internal(list.values.front());
		if (values.size() > 2) values[pos + 1] = val;
		else values.back() = val;
	}
};

struct paste_left_all : paste_base {
	constexpr static auto menu_title = L"区間基準で左半分";
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj)
	{
		if (!info.has_easing()) return false;
		if (!list.has_section_full()) return false;
		if (has_valid_left(obj) && list.section > 0) {
			// (...%s→%s→[ %s ])
			if (list.has_section_full()) list = list.trim_from_sect_r(-1);
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_end(list.size() - 3, 0)
				.to_string(info.prec, true, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set values on the left half.
		for (int p = std::max(pos - list.section, 0); p <= pos; p++)
			values[p] = info.to_internal(list.values[p - pos + list.section]);
	}
};

struct paste_right_all : paste_base {
	constexpr static auto menu_title = L"区間基準で右半分";
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj)
	{
		if (!info.has_easing()) return false;
		if (!list.has_section_full()) return false;
		if (has_valid_right(obj) && list.section + 2 < list.size()) {
			// ([ %s ]→%s→%s...)
			if (list.has_section_full()) list = list.trim_from_sect_l(-1);
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_end(0, list.size() - 3)
				.to_string(info.prec, false, true, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set values on the right half.
		for (int p = pos + 1, N = std::min<int>(pos + list.size() - list.section, values.size());
			p < N; p++)
			values[p] = info.to_internal(list.values[p - pos + list.section]);
	}
};

struct paste_all : paste_base {
	constexpr static auto menu_title = L"この区間基準";
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj)
	{
		if (!info.has_easing()) return false;
		if (!list.has_section_full()) return false;
		if (has_valid_sections(obj)) {
			// (...%s→[ %s→%s ]→%s...)
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_sect(1, 1).to_string(info.prec, true, true, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set values on the left and the right.
		for (int p = std::max(pos - list.section, 0),
			N = std::min<int>(pos + list.size() - list.section, values.size());
			p < N; p++)
			values[p] = info.to_internal(list.values[p - pos + list.section]);
	}
};

struct paste_all_headed : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;

		// (%s→%s→%s...)
		if (info.is_ease_twopoints())
			list = list.trim_from_end(0, list.size() - 2);
		list.discard_section();
		append_menu(menu, id, false, L"%s (%s)", list.size() == 1 ? L"先頭だけ" : L"先頭から順に",
			list.trim_from_end(0, list.size() - 3)
			.to_string(info.prec, false, !info.is_ease_twopoints(), false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the values for each interval.
		for (int p = std::min<int>(list.size(), values.size()); --p >= 0;)
			values[p] = info.to_internal(list.values[p]);
	}
};
struct paste_all_tailed : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;

		// (...%s→%s→%s)
		if (info.is_ease_twopoints())
			list = list.trim_from_end(list.size() - 2, 0);
		list.discard_section();
		append_menu(menu, id, false, L"%s (%s)", list.size() == 1 ? L"末尾だけ" : L"末尾から順に",
			list.trim_from_end(list.size() - 3, 0)
			.to_string(info.prec, !info.is_ease_twopoints(), false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the values for each interval.
		for (int p = values.size(), q = list.size(); --p >= 0 && --q >= 0;)
			values[p] = info.to_internal(list.values[q]);
	}
};
struct paste_uniform : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.has_easing()) return false;
		if (list.size() != 1) return false;

		list.discard_section();
		append_menu(menu, id, false, L"全区間一律に (%s)",
			list.to_string(info.prec, false, false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : values) v = val;
	}
};
struct paste_uniform_l : paste_base {
	constexpr static auto menu_title = L"区間から左を一律に";
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj)
	{
		if (!info.has_easing()) return false;
		if (list.size() != 1) return false;
		if (has_valid_left(obj)) {
			list.discard_section();
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.to_string(info.prec, false, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : std::span{ values }.subspan(0, pos + 1)) v = val;
	}
};
struct paste_uniform_r : paste_base {
	constexpr static auto menu_title = L"区間から右を一律に";
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj)
	{
		if (!info.has_easing()) return false;
		if (list.size() != 1) return false;
		if (has_valid_right(obj)) {
			list.discard_section();
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.to_string(info.prec, false, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : std::span{ values }.subspan(pos + 1)) v = val;
	}
};

struct write_l2r : modify_base {
	double value; // left value.
	bool append(HMENU menu, uint32_t id, TrackInfo const& info_l, TrackInfo const& info_r)
	{
		if (!info.has_easing()) return false;

		value = info_l.value();
		append_menu(menu, id, false, L"上書き: 左 \u25b6 右 (%.*f)", // Black Right-Pointing Triangle
			static_cast<int>(0.5f + std::log10f(static_cast<float>(info.prec))), value);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// copy the left value to right.
		values[pos + 1] = values[pos];
	}
};

struct write_r2l : modify_base {
	double value; // right value.
	bool append(HMENU menu, uint32_t id, TrackInfo const& info_l, TrackInfo const& info_r)
	{
		if (!info.has_easing()) return false;

		value = info_r.value();
		append_menu(menu, id, false, L"上書き: 左 \u25c0 右 (%.*f)", // Black Left-Pointing Triangle
			static_cast<int>(0.5f + std::log10f(static_cast<float>(info.prec))), value);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// copy the right value to left.
		values[pos] = values[pos + 1];
	}
};

struct swap_left_right : modify_base {
	double value_l, value_r; // original values on the both sides.
	bool append(HMENU menu, uint32_t id, TrackInfo const& info_l, TrackInfo const& info_r)
	{
		if (!info.has_easing()) return false;

		value_l = info_l.value(); value_r = info_r.value();
		int prec_len = static_cast<int>(0.5f + std::log10f(static_cast<float>(info.prec)));
		append_menu(menu, id, false, L"入替え: 左 \u21c4 右 (%.*f \u21c4 %.*f)", // Rightwards Arrow Over Leftwards Arrow
			prec_len, value_l, prec_len, value_r);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// swap the values on left and right.
		std::swap(values[pos], values[pos + 1]);
	}
};

struct write_left_flat : modify_base {
	constexpr static auto menu_title = L"左の区間を一律に";
	double value; // left value of the selected section.
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj, TrackInfo const& info_l)
	{
		if (!info.has_easing()) return false;
		if (has_valid_left(obj)) {
			value = info_l.value();
			append_menu(menu, id, false, L"%s (%.*f)", menu_title,
				static_cast<int>(0.5f + std::log10f(static_cast<float>(info.prec))), value);
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// copy the value to the left sections.
		auto val = info.to_internal(value);
		for (int p = 0; p < pos; p++) values[p] = val;
	}
};

struct write_right_flat : modify_base {
	constexpr static auto menu_title = L"右の区間を一律に";
	double value; // right value of the selected section.
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj, TrackInfo const& info_r)
	{
		if (!info.has_easing()) return false;
		if (has_valid_right(obj)) {
			value = info_r.value();
			append_menu(menu, id, false, L"%s (%.*f)", menu_title,
				static_cast<int>(0.5f + std::log10f(static_cast<float>(info.prec))), value);
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// copy the value to the left sections.
		auto val = info.to_internal(value);
		for (int p = pos + 2; p < static_cast<int>(values.size()); p++) values[p] = val;
	}
};

struct translate : modify_base {
	constexpr static auto root_title = L"数値を平行移動";
	bool to_right;
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj, bool right)
	{
		if (!info.has_easing()) return false;
		if (!has_valid_sections(obj)) return false;

		to_right = right;
		append_menu(menu, id, false, to_right ? L"右へ区間1つ分" : L"左へ区間1つ分");
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		// shift the values to left or right.
		std::memmove(values.data() + (to_right ? 1 : 0), values.data() + (to_right ? 0 : 1), sizeof(int) * (values.size() - 1));
	}
};

struct flip_base : modify_base {
	constexpr static auto root_title = L"数値を前後反転";
protected:
	bool append_core(HMENU menu, uint32_t id, wchar_t const* title,
		ExEdit::Object const& obj, bool require_left, bool require_right) const
	{
		if (!info.has_easing()) return false;
		if (!has_valid_sections(obj)) return false;

		append_menu(menu, id,
			(require_left && !has_adjacent_left(obj)) ||
			(require_right && !has_adjacent_right(obj)), title);
		return true;
	}

	void flip_values(std::vector<int>& values, int flip_center_x2) const
	{
		if (info.is_ease_step()) flip_center_x2--; // handle stepping easing specially.

		// reverse the values. values at off-range are filled from the nearest.
		for (size_t p = flip_center_x2 + 1; p < values.size(); p++)
			values[p] = values.front();
		for (size_t p = 0, q = flip_center_x2; p < q; p++, q--) {
			if (q >= values.size()) values[p] = values.back();
			else std::swap(values[p], values[q]);
		}
	}
};

struct flip_left : flip_base {
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj) const
	{
		if (!append_core(menu, id, L"区間の左を中心", obj, true, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		flip_values(values, 2 * pos);
	}
};

struct flip_middle : flip_base {
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj) const
	{
		if (!append_core(menu, id, L"区間の中央を中心", obj, false, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		flip_values(values, 2 * pos + 1);
	}
};

struct flip_right : flip_base {
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj) const
	{
		if (!append_core(menu, id, L"区間の右を中心", obj, false, true)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		flip_values(values, 2 * pos + 2);
	}
};

struct flip_entire : flip_base {
	bool append(HMENU menu, uint32_t id, ExEdit::Object const& obj) const
	{
		if (!append_core(menu, id, L"オブジェクトまるごと", obj, false, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values) const
	{
		flip_values(values, static_cast<int>(values.size() - 1));
	}
};

namespace menu_id
{
	enum id : uint32_t {
		dismissed = 0,
		copy,

		paste_unique,
		paste_left_all,
		paste_left,
		paste_two,
		paste_right,
		paste_right_all,
		paste_all,
		paste_all_headed,
		paste_all_tailed,
		paste_uniform,
		paste_uniform_l,
		paste_uniform_r,

		write_l2r,
		write_r2l,
		swap_left_right,
		write_left_flat,
		write_right_flat,

		trans_left,
		trans_right,

		flip_left,
		flip_middle,
		flip_right,
		flip_entire,
	};
}

static inline bool on_context_menu(HWND hwnd, size_t idx)
{
	constexpr auto add_sep = [](HMENU menu) {
		return ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
	};
	constexpr auto add_sub = [](HMENU menu, HMENU sub, wchar_t const* name) {
		return ::AppendMenuW(menu, MF_POPUP, reinterpret_cast<uintptr_t>(sub), name);
	};
	constexpr auto add_gray = [](HMENU menu, wchar_t const* name) {
		return ::AppendMenuW(menu, MF_GRAYED, menu_id::dismissed, name);
	};
	constexpr auto pop_tail = [](HMENU menu) {
		return ::RemoveMenu(menu, ::GetMenuItemCount(menu) - 1, MF_BYPOSITION);
	};

	auto& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
	track_info info{ exedit.trackinfo_left[idx], obj.track_mode[idx] };

	// prepare the context menu.
	auto menu = ::CreatePopupMenu();

	// add menuitems.
	// copying to clipboard.
	copy copy{ info };
	copy.append(menu, menu_id::copy);

	// parse the clipboard string for pasting.
	formatted_values values{};
	if (std::wstring str; sigma_lib::W32::clipboard::read(str) && !str.empty())
		values = { std::move(str) };

	paste_unique		paste_unique	{ info, values };
	paste_uniform		paste_uniform	{ info, values };
	paste_uniform_l		paste_uniform_l	{ info, values };
	paste_uniform_r		paste_uniform_r	{ info, values };
	paste_all_headed	paste_all_headed{ info, values };
	paste_all			paste_all		{ info, values };
	paste_all_tailed	paste_all_tailed{ info, values };
	paste_left_all		paste_left_all	{ info, values };
	paste_left			paste_left		{ info, values };
	paste_two			paste_two		{ info, values };
	paste_right			paste_right		{ info, values };
	paste_right_all		paste_right_all	{ info, values };

	if (values.size() > 0) {
		if (!paste_unique	.append(menu, menu_id::paste_unique)) {
			bool two_len = info.is_ease_twopoints() || obj.index_midpt_leader < 0;
			HMENU sub = ::CreatePopupMenu();
			if (values.size() == 1) {
				if (two_len) {
					paste_left		.append(sub, menu_id::paste_left);
					paste_two		.append(sub, menu_id::paste_two);
					paste_right		.append(sub, menu_id::paste_right);
				}
				else {
					paste_uniform	.append(sub, menu_id::paste_uniform);

					add_sep(sub);

					paste_all_headed.append(sub, menu_id::paste_all_headed);
					paste_uniform_l	.append(sub, menu_id::paste_uniform_l, obj);
					paste_left		.append(sub, menu_id::paste_left);
					paste_two		.append(sub, menu_id::paste_two);
					paste_right		.append(sub, menu_id::paste_right);
					paste_uniform_r	.append(sub, menu_id::paste_uniform_r, obj);
					paste_all_tailed.append(sub, menu_id::paste_all_tailed);
				}
			}
			else if (values.size() == 2 && two_len) {
				paste_left	.append(sub, menu_id::paste_left);
				paste_two	.append(sub, menu_id::paste_two);
				paste_right	.append(sub, menu_id::paste_right);
			}
			else {
				paste_all_headed.append(sub, menu_id::paste_all_headed);
				if (!two_len) paste_all.append(sub, menu_id::paste_all, obj);
				paste_all_tailed.append(sub, menu_id::paste_all_tailed);

				if (values.has_section()) {
					add_sep(sub);

					if (!two_len) paste_left_all.append(sub, menu_id::paste_left_all, obj);
					paste_left	.append(sub, menu_id::paste_left);
					paste_two	.append(sub, menu_id::paste_two);
					paste_right	.append(sub, menu_id::paste_right);
					if (!two_len) paste_right_all.append(sub, menu_id::paste_right_all, obj);
				}
			}

			add_sub(menu, sub, paste_base::root_title);
		}
	}
	else add_gray(menu, paste_base::root_title);

	// commands for internal copying.
	write_l2r			write_l2r		{ info };
	write_r2l			write_r2l		{ info };
	swap_left_right		swap_left_right	{ info };
	write_left_flat		write_left_flat	{ info };
	write_right_flat	write_right_flat{ info };

	if (info.has_easing()) {
		add_sep(menu);

		write_l2r		.append(menu, menu_id::write_l2r, exedit.trackinfo_left[idx], exedit.trackinfo_right[idx]);
		write_r2l		.append(menu, menu_id::write_r2l, exedit.trackinfo_left[idx], exedit.trackinfo_right[idx]);
		swap_left_right	.append(menu, menu_id::swap_left_right, exedit.trackinfo_left[idx], exedit.trackinfo_right[idx]);
		write_left_flat	.append(menu, menu_id::write_left_flat, obj, exedit.trackinfo_left[idx]);
		write_right_flat.append(menu, menu_id::write_right_flat, obj, exedit.trackinfo_right[idx]);
	}

	// commands for translation.
	translate		trans_left	{ info };
	translate		trans_right	{ info };

	// commands for flippings.
	flip_entire		flip_entire	{ info };
	flip_left		flip_left	{ info };
	flip_middle		flip_middle	{ info };
	flip_right		flip_right	{ info };

	if (info.has_easing() && !info.is_ease_twopoints() && obj.index_midpt_leader >= 0) {
		add_sep(menu);

		// translations as a submenu.
		HMENU sub = ::CreatePopupMenu();
		trans_left	.append(sub, menu_id::trans_left, obj, false);
		trans_right	.append(sub, menu_id::trans_right, obj, true);

		add_sub(menu, sub, translate::root_title);

		// flippings as a submenu.
		sub = ::CreatePopupMenu();
		flip_entire	.append(sub, menu_id::flip_entire,	obj);
		add_sep(sub);
		flip_left	.append(sub, menu_id::flip_left,	obj);
		flip_middle	.append(sub, menu_id::flip_middle,	obj);
		flip_right	.append(sub, menu_id::flip_right,	obj);

		add_sub(menu, sub, flip_base::root_title);
	}

	// show a context menu
	TPMPARAMS tp{ .cbSize = sizeof(tp) }; ::GetWindowRect(hwnd, &tp.rcExclude);
	POINT pt; ::GetCursorPos(&pt);
	uint32_t id = ::TrackPopupMenuEx(menu,
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_VERTICAL,
		pt.x, pt.y, hwnd, &tp);
	::DestroyMenu(menu);
	if (id == menu_id::dismissed) return false;

	// handle the selected item w.r.t. the returned id.
	switch (id) {
	case menu_id::copy:				copy			.execute(obj, idx); return false;

	case menu_id::paste_unique:		paste_unique	.execute(obj, idx); return true;
	case menu_id::paste_left_all:	paste_left_all	.execute(obj, idx); return true;
	case menu_id::paste_left:		paste_left		.execute(obj, idx); return true;
	case menu_id::paste_two:		paste_two		.execute(obj, idx); return true;
	case menu_id::paste_right:		paste_right		.execute(obj, idx); return true;
	case menu_id::paste_right_all:	paste_right_all	.execute(obj, idx); return true;
	case menu_id::paste_all:		paste_all		.execute(obj, idx); return true;
	case menu_id::paste_all_headed:	paste_all_headed.execute(obj, idx); return true;
	case menu_id::paste_all_tailed:	paste_all_tailed.execute(obj, idx); return true;
	case menu_id::paste_uniform:	paste_uniform	.execute(obj, idx); return true;
	case menu_id::paste_uniform_l:	paste_uniform_l	.execute(obj, idx); return true;
	case menu_id::paste_uniform_r:	paste_uniform_r	.execute(obj, idx); return true;

	case menu_id::write_l2r:		write_l2r		.execute(obj, idx); return true;
	case menu_id::write_r2l:		write_r2l		.execute(obj, idx); return true;
	case menu_id::swap_left_right:	swap_left_right	.execute(obj, idx); return true;
	case menu_id::write_left_flat:	write_left_flat	.execute(obj, idx); return true;
	case menu_id::write_right_flat:	write_right_flat.execute(obj, idx); return true;

	case menu_id::trans_left:		trans_left		.execute(obj, idx); return true;
	case menu_id::trans_right:		trans_right		.execute(obj, idx); return true;

	case menu_id::flip_left:		flip_left		.execute(obj, idx); return true;
	case menu_id::flip_middle:		flip_middle		.execute(obj, idx); return true;
	case menu_id::flip_right:		flip_right		.execute(obj, idx); return true;
	case menu_id::flip_entire:		flip_entire		.execute(obj, idx); return true;
	}
	return false;
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK param_button_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_RBUTTONDOWN:
	{
		if (settings.context_menu && *exedit.SettingDialogObjectIndex >= 0) {
			auto const idx = static_cast<size_t>(data);
			if (on_context_menu(hwnd, idx)) {
				// update the setting dialog and the main window.
				update_setting_dialog(*exedit.SettingDialogObjectIndex);
				update_current_frame(idx);
			}
			return 0; // mark handled.
		}
		break;
	}

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, param_button_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings::ContextMenu;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.context_menu && initializing) {
		for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++)
			::SetWindowSubclass(exedit.hwnd_track_buttons[i], param_button_hook, hook_uid(), { i });
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Easings";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool,	context_menu);

#undef read
}
