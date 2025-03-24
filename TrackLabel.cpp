/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <tuple>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>

#include "reactive_dlg.hpp"
#include "TrackLabel.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの数値入力ボックス検索．
////////////////////////////////
using namespace reactive_dlg::Track;

static inline bool check_edit_box_style(HWND edit)
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

// returns [left, idx]-pair.
static constexpr std::pair<bool, int32_t> label_id_to_idx(uint32_t id)
{
	bool left = id < id_base::label_right;
	id -= left ? id_base::label_left : id_base::label_right;
	if (id >= ExEdit::Object::MAX_TRACK) return { false, -1 };
	return { left , static_cast<int32_t>(id) };
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Track;

TrackInfo* expt::find_trackinfo(uint32_t id, HWND label)
{
	auto [left, idx] = label_id_to_idx(id);
	if (idx < 0) return nullptr;
	auto& info = (left ? exedit.trackinfo_left : exedit.trackinfo_right)[idx];
	if (label != info.hwnd_label || !check_edit_box_style(label)) return nullptr;
	return &info;
}
TrackInfo* expt::find_trackinfo(POINT dlg_point)
{
	constexpr int mid_gap = 64;
	constexpr int side_pad = 8;
	constexpr int upper_pad = 0, lower_pad = 4;
	constexpr auto client_width = [](HWND hwnd) { RECT rc; ::GetClientRect(hwnd, &rc); return rc.right; };

	HWND const dlg = *exedit.hwnd_setting_dlg;
	if (dlg == nullptr || *exedit.SettingDialogObjectIndex < 0) return nullptr;
	auto const track_n = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex].track_n;

	// determine whether the pointer is on the left/right or the middle gap.
	bool left = true;
	if (dlg_point.x < side_pad) return nullptr; // too left.
	if (int wid = client_width(dlg);
		dlg_point.x >= (wid >> 1) - mid_gap / 2) {
		if (dlg_point.x < (wid >> 1) + mid_gap / 2) return nullptr; // in the middle gap.
		if (dlg_point.x >= wid - side_pad) return nullptr; // too right.
		left = false;
	}

	// find the suitable track index for the pointer position.
	auto* info = left ? exedit.trackinfo_left : exedit.trackinfo_right;
	for (auto* const info_end = info + track_n; info < info_end; info++) {
		if (info->is_visible() &&
			info->pos_v + upper_pad <= dlg_point.y &&
			dlg_point.y < info->pos_v + TrackInfo::TrackHeight - lower_pad &&
			check_edit_box_style(info->hwnd_label))
			return info;
	}
	return nullptr;
}

TrackInfo* expt::first_trackinfo(bool forward)
{
	if (*exedit.SettingDialogObjectIndex < 0) return nullptr;
	auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
	auto const track_n = obj.track_n;
	int track_o = 0;

	// needs re-ordering the indices if the final filter is of the certain type.
	if (auto const& filter_last = obj.filter_param[obj.countFilters() - 1];
		has_flag_or(exedit.loaded_filter_table[filter_last.id]->flag, ExEdit::Filter::Flag::Output))
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
