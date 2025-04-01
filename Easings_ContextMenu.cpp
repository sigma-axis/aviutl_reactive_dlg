/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cmath>
#include <span>
#include <string>
#include <map>

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
// トラックバーの右クリックメニュー．
////////////////////////////////
using namespace reactive_dlg::Easings;
using namespace reactive_dlg::Easings::ContextMenu;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

template<class exdata>
static inline bool scripts_match(ExEdit::Object const& obj1, int filter_index1, ExEdit::Object const& obj2, int filter_index2)
{
	auto& leader1 = obj1.index_midpt_leader < 0 ? obj1 : (*exedit.ObjectArray_ptr)[obj1.index_midpt_leader],
		& leader2 = obj2.index_midpt_leader < 0 ? obj2 : (*exedit.ObjectArray_ptr)[obj2.index_midpt_leader];
	auto data1 = reinterpret_cast<exdata const*>((*exedit.exdata_table) + leader1.filter_param[filter_index1].exdata_offset + 0x0004),
		data2 = reinterpret_cast<exdata const*>((*exedit.exdata_table) + leader2.filter_param[filter_index2].exdata_offset + 0x0004);
	if (data1->name[0] != '\0')
		return std::strcmp(data1->name, data2->name) == 0;
	return data2->name[0] == '\0' && data1->type == data2->type;
}

/// @return `-1` if no matching filter found.
static inline int find_matching_filter(ExEdit::Object const& obj, ExEdit::Object const& src_obj, size_t src_filter_index)
{
	auto& filter_id = src_obj.filter_param[src_filter_index].id;
	size_t filter_index = src_filter_index;

	// for an output filter, match with the output filter of the other object.
	if (size_t count_filters = obj.countFilters();
		has_flag_or(exedit.loaded_filter_table[filter_id]->flag, ExEdit::Filter::Flag::Output))
		filter_index = count_filters - 1;
	else if (src_filter_index >= count_filters) return -1;

	// check the indentities of the two filters.
	if (obj.filter_param[filter_index].id != filter_id) return -1;

	// for filters that handle scripts, those scripts must match.
	switch (filter_id) {
		using anm_exdata = ExEdit::Exdata::efAnimationEffect; // shared with camera eff and custom object.
		using scn_exdata = ExEdit::Exdata::efSceneChange;
	case filter_id::anim_eff:
	case filter_id::cust_obj:
	case filter_id::cam_eff:
		if (!scripts_match<anm_exdata>(obj, filter_index, src_obj, src_filter_index)) return -1;
		break;

	case filter_id::scn_chg:
		if (!scripts_match<scn_exdata>(obj, filter_index, src_obj, src_filter_index)) return -1;
		break;
	}

	// all checks passed.
	return filter_index;
}

struct target_track {
	size_t track_index;
	int midpoints_count;
	bool easing_none, easing_step, easing_twopoints;

	void init(ExEdit::Object const& obj, size_t track_index)
	{
		// record the track index.
		this->track_index = track_index;

		// count up the midpoints.
		midpoints_count = 0;
		if (obj.index_midpt_leader >= 0) {
			for (int i = obj.index_midpt_leader;
				i = exedit.NextObjectIdxArray[i], i >= 0;
				midpoints_count++);
		}

		// determine the easing specs.
		constexpr int
			idx_easing_none = 0,
			idx_easing_step = 3; // 「瞬間移動」

		auto const& mode = obj.track_mode[track_index];
		easing_none = (mode.num & 0x0f) == idx_easing_none;
		easing_step = (mode.num & 0x0f) == idx_easing_step;
		easing_twopoints = easing_name_spec(mode).spec.twopoints;
	}
	constexpr size_t values_count() const {
		return easing_none ? 1 : easing_twopoints ? 2 : midpoints_count + 2;
	}
	constexpr size_t flipping_values_count() const {
		return values_count() - (easing_step ? 1 : 0);
	}
};
struct target_tracks {
	size_t selected_object_index, selected_section;
	struct {
		size_t min, max;
		constexpr bool uniform() const { return min == max; }
		constexpr bool uniformly(size_t v) const { return min == v && uniform(); }
		constexpr void drop(size_t v) {
			if (min > max) min = max = v;
			else if (v < min) min = v;
			else if (v > max) max = v;
		}
	} values_count, flipping_values_count;
	std::map<size_t, target_track> targets;

	// common properties of the target trackbar.
	int track_denom, track_prec, track_min, track_max,
		track_prec_digits;

	std::pair<ExEdit::Object const&, target_track const&> selected_track() const {
		auto const* const objects = *exedit.ObjectArray_ptr;
		auto const& obj = objects[selected_object_index];
		return {
			obj,
			targets.at(obj.index_midpt_leader < 0 ? selected_object_index : obj.index_midpt_leader)
		};
	}
	int to_internal(double val) const {
		return convert_value_disp2int(val, track_denom, track_prec, track_min, track_max);
	}

	size_t count_chains() const { return targets.size(); }
	bool is_valid() const { return count_chains() > 0; }
	operator bool() const { return is_valid(); }
	target_tracks(size_t track_index)
		: selected_object_index{ static_cast<size_t>(*exedit.SettingDialogObjectIndex) }
		, selected_section{ 0 }
		, values_count{ 1, 0 }
		, flipping_values_count{ 1, 0 }
		, targets{}
		, track_denom{ 1 }
		, track_prec{ 1 }
		, track_min{ 0 }
		, track_max{ 0 }
		, track_prec_digits{ 0 }
	{
		auto const* const objects = *exedit.ObjectArray_ptr;
		auto const& obj = objects[selected_object_index];
		size_t const leading_index = obj.index_midpt_leader < 0 ? selected_object_index : obj.index_midpt_leader;

		// identify the selected section.
		for (int i = leading_index; i != selected_object_index; i = exedit.NextObjectIdxArray[i])
			selected_section++;

		// add the first element.
		targets.try_emplace(leading_index);

		// add the other selected objects.
		for (int i : std::span{
			exedit.SelectingObjectIndex,
			static_cast<size_t>(std::max(*exedit.SelectingObjectNum_ptr, 0)) }) {
			auto const& i_leader = objects[i].index_midpt_leader;
			targets.try_emplace(i_leader < 0 ? i : i_leader);
		}

		// eliminate objects that doesn't have a matching trackbar.
		auto [filter_index, filter_track_index] = find_filter_from_track(obj, track_index);
		for (auto e = targets.end(), i = targets.begin(); i != e; ) {
			auto const& obj_i = objects[i->first];
			if (int const filter_index_i = find_matching_filter(obj_i, obj, filter_index);
				filter_index_i >= 0)
				(i++)->second.init(obj_i, // initialize the content.
					obj_i.filter_param[filter_index_i].track_begin + filter_track_index);
			else targets.erase(i++); // eliminate the element.
		}

		// inspect min/max of number of values, as well as flipping ones.
		for (auto const& [_, target] : targets) {
			values_count.drop(target.values_count());
			flipping_values_count.drop(target.flipping_values_count());
		}

		// determine the trackbar specs.
		auto const& trackinfo = exedit.trackinfo_left[track_index];
		track_denom = trackinfo.denominator();
		track_prec = trackinfo.precision();
		track_min = trackinfo.val_int_min;
		track_max = trackinfo.val_int_max;
		track_prec_digits = std::lroundf(std::log10f(static_cast<float>(track_prec)));
	}
	target_tracks& operator=(target_tracks const&) = delete;
	target_tracks(target_tracks const&) = delete;
	target_tracks(target_tracks&&) = default;
};

struct cmd_base {
	target_tracks const& info;

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
	static void push_undo(target_tracks const& info)
	{
		exedit.nextundo();
		for (auto& [i, _] : info.targets)
			exedit.setundo(i, 0x01); // 0x01: the entire chain.
	}

};

struct copy : cmd_base {
	bool append(HMENU menu, uint32_t id)
	{
		append_menu(menu, id, false, L"数値をコピー (&C)");
		return true;
	}

	void execute() const
	{
		auto [obj, track] = info.selected_track();
		sigma_lib::W32::clipboard::write(
			formatted_values{ obj, track.track_index }.span().to_string(info.track_prec, false, false, false));
	}
};

struct modify_base : cmd_base {
	void execute(this auto const& self)
	{
		push_undo(self.info);

		for (auto [i, track] : self.info.targets) {
			auto chain = collect_pos_chain((*exedit.ObjectArray_ptr)[i]).second;
			auto values = collect_int_values(chain, track.track_index);
			self.modify_values(self.info.selected_section, values, track);
			apply_int_values(chain, values, track.track_index);
		}
	}
};
struct paste_base : modify_base {
	constexpr static auto root_title = L"数値を貼り付け";
	formatted_valuespan list;
};
struct paste_unique : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.max > 1) return false;

		list = list.has_section_full() ? list.trim_from_sect_l(0) : list;
		list = list.trim_from_end(0, list.size() - 1);
		list.discard_section();
		append_menu(menu, id, false, L"%s (&V) (%s)", root_title,
			list.to_string(info.track_prec, false, false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set all values uniformly.
		values.front() = convert_value_disp2int(list.values.front(),
			info.track_denom, info.track_prec, info.track_min, info.track_max);
	}
};

struct paste_two : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect(0, 0) : list.trim_from_end(0, list.size() - 2);
		list.discard_section();
		append_menu(menu, id, false, L"区間の左右 (%s)",
			list.to_string(info.track_prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
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
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect_l(0) : list;
		list = list.trim_from_end(0, list.size() - 1);
		list.discard_section();
		append_menu(menu, id, false, L"区間の左だけ (%s)",
			list.to_string(info.track_prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
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
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (!(list.size() <= 2 || list.has_section_full())) return false;
		list = list.has_section_full() ? list.trim_from_sect_r(0) : list;
		list = list.trim_from_end(list.size() - 1, 0);
		list.discard_section();
		append_menu(menu, id, false, L"区間の右だけ (%s)",
			list.to_string(info.track_prec, false, false, false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the value on the right track.
		int val = info.to_internal(list.values.front());
		if (values.size() > 2) values[pos + 1] = val;
		else values.back() = val;
	}
};

struct paste_left_all : paste_base {
	constexpr static auto menu_title = L"区間基準で左半分";
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (!list.has_section_full()) return false;
		if (info.values_count.min >= 3 && info.selected_section > 0 &&
			list.section > 0) {
			// (...%s→%s→[ %s ])
			if (list.has_section_full()) list = list.trim_from_sect_r(-1);
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_end(list.size() - 3, 0)
				.to_string(info.track_prec, true, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set values on the left half.
		for (int p = std::max(pos - list.section, 0); p <= pos; p++)
			values[p] = info.to_internal(list.values[p - pos + list.section]);
	}
};

struct paste_right_all : paste_base {
	constexpr static auto menu_title = L"区間基準で右半分";
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (!list.has_section_full()) return false;
		if (info.values_count.min >= 3 && info.selected_section < info.values_count.min - 2 &&
			list.section + 2 < list.size()) {
			// ([ %s ]→%s→%s...)
			if (list.has_section_full()) list = list.trim_from_sect_l(-1);
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_end(0, list.size() - 3)
				.to_string(info.track_prec, false, true, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set values on the right half.
		for (int p = pos + 1, N = std::min<int>(pos + list.size() - list.section, values.size());
			p < N; p++)
			values[p] = info.to_internal(list.values[p - pos + list.section]);
	}
};

struct paste_all : paste_base {
	constexpr static auto menu_title = L"この区間基準";
	bool append(HMENU menu, uint32_t id)
	{
		if (!info.values_count.uniform()) return false;
		if (!list.has_section_full()) return false;
		if (info.values_count.min >= 3) {
			// (...%s→[ %s→%s ]→%s...)
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.trim_from_sect(1, 1).to_string(info.track_prec, true, true, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
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
		if (info.values_count.max == 1) return false;

		// (%s→%s→%s...)
		if (info.values_count.uniformly(2))
			list = list.trim_from_end(0, list.size() - 2);
		list.discard_section();
		append_menu(menu, id, false, L"%s (%s)", list.size() == 1 ? L"先頭だけ" : L"先頭から順に",
			list.trim_from_end(0, list.size() - 3)
			.to_string(info.track_prec, false, !info.values_count.uniformly(2), false).c_str());

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the values for each interval.
		for (int p = std::min<int>(list.size(), values.size()); --p >= 0;)
			values[p] = info.to_internal(list.values[p]);
	}
};
struct paste_all_tailed : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.max == 1) return false;

		// (...%s→%s→%s)
		if (info.values_count.uniformly(2))
			list = list.trim_from_end(list.size() - 2, 0);
		list.discard_section();
		append_menu(menu, id, false, L"%s (%s)", list.size() == 1 ? L"末尾だけ" : L"末尾から順に",
			list.trim_from_end(list.size() - 3, 0)
			.to_string(info.track_prec, !info.values_count.uniformly(2), false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the values for each interval.
		for (int p = values.size(), q = list.size(); --p >= 0 && --q >= 0;)
			values[p] = info.to_internal(list.values[q]);
	}
};
struct paste_uniform : paste_base {
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.max == 1) return false;
		if (list.size() != 1) return false;

		list.discard_section();
		append_menu(menu, id, false, L"全区間一律に (%s)",
			list.to_string(info.track_prec, false, false, false).c_str());
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : values) v = val;
	}
};
struct paste_uniform_l : paste_base {
	constexpr static auto menu_title = L"区間から左を一律に";
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (list.size() != 1) return false;
		if (info.values_count.min >= 3 && info.selected_section > 0) {
			list.discard_section();
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.to_string(info.track_prec, false, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : std::span{ values }.subspan(0, pos + 1)) v = val;
	}
};
struct paste_uniform_r : paste_base {
	constexpr static auto menu_title = L"区間から右を一律に";
	bool append(HMENU menu, uint32_t id)
	{
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (list.size() != 1) return false;
		if (info.values_count.min >= 3 && info.selected_section < info.values_count.min - 2) {
			list.discard_section();
			append_menu(menu, id, false, L"%s (%s)", menu_title,
				list.to_string(info.track_prec, false, false, false).c_str());
		}
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// set the values for all intervals uniformly.
		auto val = info.to_internal(list.values.front());
		for (auto& v : std::span{ values }.subspan(pos + 1)) v = val;
	}
};

struct write_l2r : modify_base {
	double value; // left value.
	bool append(HMENU menu, uint32_t id, double value_l, double value_r)
	{
		value = value_l;
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		append_menu(menu, id, false, L"上書き: 左 \u25b6 右 (%.*f)", // Black Right-Pointing Triangle
			info.track_prec_digits, value);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// copy the left value to right.
		values[pos + 1] = values[pos];
	}
};

struct write_r2l : modify_base {
	double value; // right value.
	bool append(HMENU menu, uint32_t id, double value_l, double value_r)
	{
		value = value_r;
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		append_menu(menu, id, false, L"上書き: 左 \u25c0 右 (%.*f)", // Black Left-Pointing Triangle
			info.track_prec_digits, value);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// copy the right value to left.
		values[pos] = values[pos + 1];
	}
};

struct swap_left_right : modify_base {
	double value_l, value_r; // original values on the both sides.
	bool append(HMENU menu, uint32_t id, double value_l, double value_r)
	{
		this->value_l = value_l; this->value_r = value_r;
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		append_menu(menu, id, false, L"入替え: 左 \u21c4 右 (%.*f \u21c4 %.*f)", // Rightwards Arrow Over Leftwards Arrow
			info.track_prec_digits, value_l, info.track_prec_digits, value_r);
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// swap the values on left and right.
		std::swap(values[pos], values[pos + 1]);
	}
};

struct write_left_flat : modify_base {
	constexpr static auto menu_title = L"左の区間を一律に";
	double value; // left value of the selected section.
	bool append(HMENU menu, uint32_t id, double value_l, double value_r)
	{
		value = value_l;
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (info.values_count.min >= 3 && info.selected_section > 0)
			append_menu(menu, id, false, L"%s (%.*f)", menu_title, info.track_prec_digits, value);
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// copy the value to the left sections.
		auto val = info.to_internal(value);
		for (int p = 0; p < pos; p++) values[p] = val;
	}
};

struct write_right_flat : modify_base {
	constexpr static auto menu_title = L"右の区間を一律に";
	double value; // right value of the selected section.
	bool append(HMENU menu, uint32_t id, double value_l, double value_r)
	{
		value = value_r;
		if (info.values_count.min == 1 || !info.values_count.uniform()) return false;
		if (info.values_count.min >= 3 && info.selected_section < info.values_count.min - 2)
			append_menu(menu, id, false, L"%s (%.*f)", menu_title, info.track_prec_digits, value);
		else append_menu(menu, id, true, menu_title);

		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// copy the value to the left sections.
		auto val = info.to_internal(value);
		for (int p = pos + 2; p < static_cast<int>(values.size()); p++) values[p] = val;
	}
};

struct translate : modify_base {
	constexpr static auto root_title = L"数値を平行移動";
	bool to_right;
	bool append(HMENU menu, uint32_t id, bool right)
	{
		if (info.values_count.max < 3) return false;

		to_right = right;
		append_menu(menu, id, false, to_right ? L"右へ区間1つ分" : L"左へ区間1つ分");
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		// shift the values to left or right.
		std::memmove(
			values.data() + (to_right ? 1 : 0),
			values.data() + (to_right ? 0 : 1),
			sizeof(int) * (values.size() - 1));
	}
};

struct flip_base : modify_base {
	constexpr static auto root_title = L"数値を前後反転";
protected:
	bool append_core(HMENU menu, uint32_t id, wchar_t const* title,
		bool require_left, bool require_right) const
	{
		if (info.flipping_values_count.max < 3) return false;

		append_menu(menu, id,
			(require_left && !(info.values_count.uniform() && info.selected_section > 0)) ||
			(require_right && !(info.values_count.uniform() && info.selected_section < info.values_count.min - 2)),
			title);
		return true;
	}

	void flip_values(std::vector<int>& values, int flip_center_x2, target_track const& track) const
	{
		if (track.easing_step) flip_center_x2--; // handle stepping easing specially.

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
	bool append(HMENU menu, uint32_t id) const
	{
		if (!info.values_count.uniform() ||
			!append_core(menu, id, L"区間の左を中心", true, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		flip_values(values, 2 * pos, track);
	}
};

struct flip_middle : flip_base {
	bool append(HMENU menu, uint32_t id) const
	{
		if (!info.values_count.uniform() ||
			!append_core(menu, id, L"区間の中央を中心", false, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		flip_values(values, 2 * pos + 1, track);
	}
};

struct flip_right : flip_base {
	bool append(HMENU menu, uint32_t id) const
	{
		if (!info.values_count.uniform() ||
			!append_core(menu, id, L"区間の右を中心", false, true)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		flip_values(values, 2 * pos + 2, track);
	}
};

struct flip_entire : flip_base {
	bool append(HMENU menu, uint32_t id) const
	{
		if (!append_core(menu, id, L"オブジェクトまるごと", false, false)) return false;
		return true;
	}

	void modify_values(int pos, std::vector<int>& values, target_track const& track) const
	{
		flip_values(values, static_cast<int>(values.size() - 1), track);
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

	target_tracks info{ idx };

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

	if (values.empty()) add_gray(menu, paste_base::root_title);
	else if (!paste_unique			.append(menu, menu_id::paste_unique)) {
		bool two_len = info.values_count.uniformly(2);
		HMENU sub = ::CreatePopupMenu();
		if (values.size() == 1) {
			if (two_len) {
				paste_left			.append(sub, menu_id::paste_left);
				paste_two			.append(sub, menu_id::paste_two);
				paste_right			.append(sub, menu_id::paste_right);
			}
			else {
				paste_uniform		.append(sub, menu_id::paste_uniform);

				add_sep(sub);

				paste_all_headed	.append(sub, menu_id::paste_all_headed);
				if (info.values_count.uniform()) {
					paste_uniform_l	.append(sub, menu_id::paste_uniform_l);
					paste_left		.append(sub, menu_id::paste_left);
					paste_two		.append(sub, menu_id::paste_two);
					paste_right		.append(sub, menu_id::paste_right);
					paste_uniform_r	.append(sub, menu_id::paste_uniform_r);
				}
				paste_all_tailed	.append(sub, menu_id::paste_all_tailed);
			}
		}
		else if (values.size() == 2 && two_len) {
			paste_left				.append(sub, menu_id::paste_left);
			paste_two				.append(sub, menu_id::paste_two);
			paste_right				.append(sub, menu_id::paste_right);
		}
		else {
			paste_all_headed		.append(sub, menu_id::paste_all_headed);
			if (!two_len && info.values_count.uniform())
				paste_all			.append(sub, menu_id::paste_all);
			paste_all_tailed		.append(sub, menu_id::paste_all_tailed);

			if (values.has_section() && info.values_count.uniform()) {
				add_sep(sub);

				if (!two_len) paste_left_all.append(sub, menu_id::paste_left_all);
				paste_left			.append(sub, menu_id::paste_left);
				paste_two			.append(sub, menu_id::paste_two);
				paste_right			.append(sub, menu_id::paste_right);
				if (!two_len) paste_right_all.append(sub, menu_id::paste_right_all);
			}
		}

		add_sub(menu, sub, paste_base::root_title);
	}

	// commands for internal copying.
	write_l2r			write_l2r		{ info };
	write_r2l			write_r2l		{ info };
	swap_left_right		swap_left_right	{ info };
	write_left_flat		write_left_flat	{ info };
	write_right_flat	write_right_flat{ info };

	if (info.values_count.min > 1 && info.values_count.uniform()) {
		double value_l = exedit.trackinfo_left[idx].value(),
			value_r = exedit.trackinfo_right[idx].value();

		add_sep(menu);

		write_l2r		.append(menu, menu_id::write_l2r,			value_l, value_r);
		write_r2l		.append(menu, menu_id::write_r2l,			value_l, value_r);
		swap_left_right	.append(menu, menu_id::swap_left_right,		value_l, value_r);
		write_left_flat	.append(menu, menu_id::write_left_flat,		value_l, value_r);
		write_right_flat.append(menu, menu_id::write_right_flat,	value_l, value_r);
	}

	// commands for translation.
	translate		trans_left	{ info };
	translate		trans_right	{ info };

	// commands for flippings.
	flip_entire		flip_entire	{ info };
	flip_left		flip_left	{ info };
	flip_middle		flip_middle	{ info };
	flip_right		flip_right	{ info };

	if (info.flipping_values_count.min >= 3) {
		add_sep(menu);

		// translations as a submenu.
		HMENU sub = ::CreatePopupMenu();
		trans_left		.append(sub, menu_id::trans_left, false);
		trans_right		.append(sub, menu_id::trans_right, true);

		add_sub(menu, sub, translate::root_title);

		// flippings as a submenu.
		sub = ::CreatePopupMenu();
		flip_entire		.append(sub, menu_id::flip_entire);
		if (info.values_count.uniform()) {
			add_sep(sub);

			flip_left	.append(sub, menu_id::flip_left);
			flip_middle	.append(sub, menu_id::flip_middle);
			flip_right	.append(sub, menu_id::flip_right);
		}

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
	case menu_id::copy:				copy			.execute(); return false;

	case menu_id::paste_unique:		paste_unique	.execute(); return true;
	case menu_id::paste_left_all:	paste_left_all	.execute(); return true;
	case menu_id::paste_left:		paste_left		.execute(); return true;
	case menu_id::paste_two:		paste_two		.execute(); return true;
	case menu_id::paste_right:		paste_right		.execute(); return true;
	case menu_id::paste_right_all:	paste_right_all	.execute(); return true;
	case menu_id::paste_all:		paste_all		.execute(); return true;
	case menu_id::paste_all_headed:	paste_all_headed.execute(); return true;
	case menu_id::paste_all_tailed:	paste_all_tailed.execute(); return true;
	case menu_id::paste_uniform:	paste_uniform	.execute(); return true;
	case menu_id::paste_uniform_l:	paste_uniform_l	.execute(); return true;
	case menu_id::paste_uniform_r:	paste_uniform_r	.execute(); return true;

	case menu_id::write_l2r:		write_l2r		.execute(); return true;
	case menu_id::write_r2l:		write_r2l		.execute(); return true;
	case menu_id::swap_left_right:	swap_left_right	.execute(); return true;
	case menu_id::write_left_flat:	write_left_flat	.execute(); return true;
	case menu_id::write_right_flat:	write_right_flat.execute(); return true;

	case menu_id::trans_left:		trans_left		.execute(); return true;
	case menu_id::trans_right:		trans_right		.execute(); return true;

	case menu_id::flip_left:		flip_left		.execute(); return true;
	case menu_id::flip_middle:		flip_middle		.execute(); return true;
	case menu_id::flip_right:		flip_right		.execute(); return true;
	case menu_id::flip_entire:		flip_entire		.execute(); return true;
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
				update_current_frame();
			}
			return 0; // mark handled.
		}
		break;
	}

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &param_button_hook, id);
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
			::SetWindowSubclass(exedit.hwnd_track_buttons[i], &param_button_hook, hook_uid(), { i });
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
