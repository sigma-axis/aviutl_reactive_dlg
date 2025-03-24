/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <utility>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "reactive_dlg.hpp"


////////////////////////////////
// トラックバーの数値入力ボックス検索．
////////////////////////////////
namespace reactive_dlg::Track
{
	namespace id_base
	{
		constexpr uint32_t
			button = 4000,
			label_left = 4100,
			label_right = 4200;
	}

	/// identifies `TrackInfo` associated to the trackbar label.
	/// @param id the control ID to the trackbar label identified by `label`.
	/// @param label the handle to the edit control in a part of the trackbar.
	/// @return the pointer to the associated `TrackInfo` structure,
	/// or `nullptr` if found no such one.
	TrackInfo* find_trackinfo(uint32_t id, HWND label);
	/// identifies `TrackInfo` located at the certain point in the setting dialog.
	/// @param dlg_point the point where to find the trackbar controls,
	/// relative to the top-left corner of the setting dialog.
	/// @return the pointer to the associated `TrackInfo` structure,
	/// or `nullptr` if found no such one.
	TrackInfo* find_trackinfo(POINT dlg_point);
	/// identifies `TrackInfo` associated to the trackbar label.
	/// internally calls the overload `find_trackinfo(uint32_t, HWND)`.
	/// @param label the handle to the edit control in a part of the trackbar.
	/// @return the pointer to the associated `TrackInfo` structure,
	/// or `nullptr` if found no such one.
	inline TrackInfo* find_trackinfo(HWND label)
	{
		if (label == nullptr) return nullptr;
		return find_trackinfo(::GetDlgCtrlID(label), label);
	}

	/// identifies the first or last valid `TrackInfo`.
	/// @param forward `true` to find the first one, `false` the last one.
	/// @return the pointer to the desired `TrackInfo`,
	/// or `nullptr` if no trackbar is visible.
	TrackInfo* first_trackinfo(bool forward);

	/// identifies the trackbar index associated to the `TrackInfo` structure.
	/// @param ptr_info the pointer to the `TrackInfo` structure.
	/// @return the index among the trackbars.
	inline size_t info_index(TrackInfo const* ptr_info) {
		return std::min<size_t>(ptr_info - exedit.trackinfo_left, ptr_info - exedit.trackinfo_right);
	}

	/// retrieves the control ID of the center button
	/// associated to a trackbar at the given index.
	/// @param idx the index of trackbar the button is associated to.
	/// @return the control ID of the button.
	constexpr uint32_t button_ctrl_id(size_t idx) {
		return idx + id_base::button;
	}
	/// retrieves the control ID of the edit control used as a trackbar label,
	/// associated to a trackbar at the given position and index.
	/// @param left specifies whether the label is located on the left or right.
	/// @param idx the index of trackbar the label is associated to.
	/// @return the control ID of the edit control.
	constexpr uint32_t label_ctrl_id(bool left, size_t idx) {
		return idx + (left ? id_base::label_left : id_base::label_right);
	}
}
