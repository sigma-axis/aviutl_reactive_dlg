/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <tuple>
#include <vector>
#include <span>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>


////////////////////////////////
// トラックバーの変化方法関連．
////////////////////////////////
namespace reactive_dlg::Easings
{
	// easing specifications.
	struct easing_spec {
		bool speed, param, twopoints, loaded;
		constexpr easing_spec(bool speed, bool param, bool twopoints, bool loaded)
			: speed{ speed }
			, param{ param }
			, twopoints{ twopoints }
			, loaded{ loaded } {}
		// makes sense only for script easings.
		constexpr explicit easing_spec(uint8_t flags) : easing_spec{
			(flags & flag_speed) != 0,
			(flags & flag_param) != 0,
			(flags & flag_twopoints) != 0,
			(flags & flag_loaded) != 0 } {}
		explicit easing_spec(ExEdit::Object::TrackMode mode);

		easing_spec() = default;
		easing_spec(easing_spec const&) = default;
		easing_spec(easing_spec&&) = default;
		easing_spec& operator=(easing_spec const&) = default;

		constexpr static uint8_t
			flag_speed		= 1 << 0,
			flag_param		= 1 << 1,
			flag_twopoints	= 1 << 6,
			flag_loaded		= 1 << 7;
	};

	// the pair of name and spec of an easing.
	struct easing_name_spec {
		std::string_view name, // NOTE: `name` may not be null-terminated.
			directory;
		easing_spec spec;

		easing_name_spec(ExEdit::Object::TrackMode mode);
		static size_t script_count();
		constexpr static std::string_view basic_names[]{
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
		constexpr static size_t basic_count = std::size(basic_names);
	};

	// copy-paste formatting, adjusted for displaying.
	struct formatted_valuespan {
		std::span<double const> values;
		int section;
		bool more_l, more_r;

		constexpr int size() const { return static_cast<int>(values.size()); }
		constexpr bool empty() const { return size() == 0; }
		constexpr bool has_section() const { return section > -2; }
		constexpr bool has_section_full() const { return 0 <= section && section + 1 < size(); }
		constexpr void discard_section() { section = -2; }

		// formats a string that lists up the transition of values.
		std::wstring to_string(int prec, bool ellip_l, bool ellip_r, bool zigzag) const;

		formatted_valuespan trim_from_end(int left, int right) const;
		formatted_valuespan trim_from_sect(int left_trail, int right_trail) const {
			if (!has_section()) {
				left_trail = size();
				right_trail = 0;
			}
			return trim_from_end(
				section - left_trail,
				size() - section - 2 - right_trail);
		}
		formatted_valuespan trim_from_sect_l(int trail) const { return trim_from_sect(trail, size()); }
		formatted_valuespan trim_from_sect_r(int trail) const { return trim_from_sect(size(), trail); }
		formatted_valuespan(std::span<double const>&& values, int section)
			: formatted_valuespan{ std::move(values), section, false, false } {}

		formatted_valuespan(formatted_valuespan const&) = default;
		formatted_valuespan(formatted_valuespan&&) = default;
		formatted_valuespan& operator=(formatted_valuespan const&) = default;

	private:
		formatted_valuespan(std::span<double const>&& values, int section, bool more_l, bool more_r)
			: values{ values }, section{ section }, more_l{ more_l }, more_r{ more_r } {}
	};

	// copy-paste formatting. hosts values and current section.
	struct formatted_values {
		std::vector<double> vals;
		int section;

		constexpr int size() const { return static_cast<int>(vals.size()); }
		constexpr bool empty() const { return size() == 0; }
		constexpr bool has_section() const { return section >= 0; }
		formatted_valuespan span() const { return { vals, section }; }
		operator formatted_valuespan() const { return span(); }

		/// collects values of a certain track from a chain of objects.
		/// @param obj the target object. can be non-leading one.
		/// @param idx_track the index of the target track to collect values.
		formatted_values(ExEdit::Object const& obj, size_t idx_track);
		/// parses the formatted values from a string.
		/// @param src the srouce string to be parsed.
		formatted_values(std::wstring src);

		constexpr formatted_values() : vals{}, section{ -1 } {};
		formatted_values(formatted_values const&) = default;
		formatted_values(formatted_values&&) = default;
		formatted_values& operator=(formatted_values const&) = default;
	};

	/// enumerates indices of all objects in the midpoint-chain.
	/// @param obj the focused object in the midpoint-chain.
	/// @return a pair of the position of the object on focus, and the list of indices.
	std::pair<int, std::vector<int>> collect_pos_chain(ExEdit::Object const& obj);

	/// collects internal values of the trackbar in the midpoint-chain.
	/// @param chain the array of indices of the objects.
	/// @param idx_track the index of the trackbar where the values are picked.
	/// @return an array of the collected internal values.
	std::vector<int> collect_int_values(std::vector<int> const& chain, size_t idx_track);

	/// applies internal values of the trackbar in the midpoint-chain.
	/// the caller is responsible to handle undo buffer beforehand.
	/// @param chain the array of indeces of the objects.
	/// @param values the array of the desired internal values.
	/// @param idx_track the index of the trackbar where the values are set.
	void apply_int_values(std::vector<int> const& chain, std::vector<int> const& values, size_t idx_track);

	/// converts a displayed value to an internal value,
	/// rounding and clamping into min-max range.
	int convert_value_disp2int(double val, int denom, int prec, int min, int max);

	/// finds the filter index and relative track index of a given track index.
	/// @param obj the object the given trackbar belongs to.
	/// @param track_index the index of the trackbar, counted from the beginning of the object.
	/// @return the pair `[filter_index, relative_track_index]`,
	/// where `filter_index` is the index of the filter,
	/// and `relative_track_index` is the index of the trackbar counted from the beginning of the filter.
	std::pair<size_t, size_t> find_filter_from_track(ExEdit::Object const& obj, size_t track_index);

	/// composes `ExEdit::ObjectFilterIndex` from object index and filter index.
	/// @param obj_index the index of the object.
	/// @param filter_index the index of the filter.
	/// @return the composed `ExEdit::ObjectFilterIndex`.
	inline ExEdit::ObjectFilterIndex object_filter_index(size_t obj_index, size_t filter_index) {
		return static_cast<ExEdit::ObjectFilterIndex>((obj_index + 1) | (filter_index << 16));
	}
}
