/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <vector>
#include <tuple>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
using byte = uint8_t;
#include <exedit.hpp>

#include "reactive_dlg.hpp"
#include "Easings.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの変化方法関連．
////////////////////////////////
using namespace reactive_dlg::Easings;

// source of script names.
struct script_name {
	std::string_view name, dir;
	script_name(char const*& ptr) {
		std::string_view raw{ ptr };
		ptr += raw.size() + 1;

		// script names are stored in forms of either:
		//   i) "<name1>\x1<directory1>\0"
		//  ii) "<name>\0"
		if (auto pos = raw.find('\x01'); pos != raw.npos) {
			name = raw.substr(0, pos);
			dir = raw.substr(pos + 1);
		}
		else {
			name = raw;
			dir = "";
		}
	}

	static script_name const& from_index(size_t idx)
	{
		collect();
		return names[idx];
	}

	static size_t count() {
		collect();
		return names.size();
	}

private:
#ifndef _DEBUG
	constinit
#endif // !_DEBUG
	static inline std::vector<script_name> names{};
	static void collect()
	{
		if (names.size() > 0) return;

		for (auto* ptr = exedit.track_script_names;
			*ptr != '\0'; names.emplace_back(ptr));
		names.shrink_to_fit();
	}
};

constexpr static easing_spec easing_spec_builtin(size_t basic_idx)
{
	auto const flags = exedit.easing_specs_builtin[basic_idx];
	return {
		(flags & easing_spec::flag_speed) != 0,
		(flags & easing_spec::flag_param) != 0,
		basic_idx >= 4 && basic_idx != 7,
		true,
	};
}

static easing_spec easing_spec_script(size_t script_idx)
{
	auto const& flags = exedit.easing_specs_script[script_idx];
	if ((flags & easing_spec::flag_loaded) == 0)
		// script wasn't loaded yet. try after loading.
		exedit.load_easing_spec(script_idx, 0, 0);
	return easing_spec{ flags };
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings;

expt::easing_spec::easing_spec(ExEdit::Object::TrackMode mode)
{
	size_t const basic_idx = mode.num & 0x0f;
	*this = basic_idx == mode.isScript ?
		easing_spec_script(mode.script_idx) :
		easing_spec_builtin(basic_idx);
}

expt::easing_name_spec::easing_name_spec(ExEdit::Object::TrackMode mode)
{
	if (size_t const basic_idx = mode.num & 0x0f;
		basic_idx == mode.isScript) {
		auto& n = script_name::from_index(mode.script_idx);
		name = n.name;
		directory = n.dir;
		spec = easing_spec_script(mode.script_idx);
	}
	else {
	#pragma warning(suppress : 6385)
		name = basic_names[basic_idx];
		directory = "";
		spec = easing_spec_builtin(basic_idx);
	}
}

size_t expt::easing_name_spec::script_count() { return script_name::count(); }

expt::formatted_values::formatted_values(ExEdit::Object const& obj, size_t idx_track) : vals{}
{
	auto const [pos, chain] = collect_pos_chain(obj); section = pos;
	auto const values = collect_int_values(chain, idx_track);

	if (values.size() == 1) section = -1; // 移動無し
	else if (values.size() == 2) section = 0; // 中間点なし or 中間点無視

	auto const [filter_idx, rel_idx] = find_filter_from_track(obj, idx_track);
	auto const* scale = exedit.loaded_filter_table[obj.filter_param[filter_idx].id]->track_scale;
	double const denom = scale == nullptr ? 1 : std::max(scale[rel_idx], 1);

	vals.reserve(values.size());
	for (int value : values) vals.push_back(value / denom);
}

expt::formatted_values::formatted_values(std::wstring src) : vals{}, section{ -1 }
{
	// helper lambdas.
	constexpr auto is_numeric = [](wchar_t c) {
		constexpr std::wstring_view numerics = L"+-.0123456789";
		return numerics.contains(c);
		//return c == L'+' || c == L'-' || c == L'.' || (L'0' <= c && c <= L'9');
	};
	constexpr auto find_unique = [](std::wstring const& src, wchar_t c) -> wchar_t const* {
		auto pos = src.find(c);
		if (pos == src.npos || src.find(c, pos + 1) != src.npos) return nullptr;
		return src.data() + pos;
	};

	// limit the source to the first line of the string.
	if (auto const pos = src.find_first_of(L"\r\n");
		pos != src.npos) src.erase(pos);

	// identify the bracket positions.
	auto const pos_bra_l = find_unique(src, L'['), pos_bra_r = find_unique(src, L']');
	int idx_bra_l = -1, idx_bra_r = -1;

	// replace all non-numeric characters with white spaces.
	for (auto& c : src) { if (!is_numeric(c)) c = L' '; }

	// parse each number and add it to the list.
	auto pos = src.data(), e = pos;
	for (double val; val = std::wcstod(pos, &e), e != pos; pos = e) {
		if (val == HUGE_VAL || (*e != L'\0' && *e != L' ')) {
			// recognize as a failure.
			vals.clear();
			return;
		}

		// identify the bracket indices.
		if (idx_bra_l < 0 && pos_bra_l < e) idx_bra_l = vals.size();
		if (idx_bra_r < 0 && pos_bra_r < e) idx_bra_r = vals.size();

		// add the parsed number.
		vals.push_back(val);
	}
	if (idx_bra_l < 0) idx_bra_l = vals.size();
	if (idx_bra_r < 0) idx_bra_r = vals.size();

	// verify the bracket index.
	if (pos_bra_l != nullptr && pos_bra_r != nullptr && idx_bra_r == idx_bra_l + 2)
		section = idx_bra_l;
	else if (pos_bra_l == nullptr && pos_bra_r == nullptr && vals.size() == 2)
		// two-long sequence shall implicitly specify the section unless explicitly specified.
		section = 0;
}

formatted_valuespan expt::formatted_valuespan::trim_from_end(int left, int right) const
{
	left = std::max(left, 0); right = std::max(right, 0);
	int const len0 = size(), len1 = len0 - left - right;
	if (len1 <= 0) return { values.subspan(0, 0), -2 };
	return { values.subspan(left, len1),
		section + 1 < left || section >= len0 - right ? -2 : section - left,
		more_l || left > 0, more_r || right > 0,
	};
}

std::wstring expt::formatted_valuespan::to_string(int prec, bool overflow_l, bool overflow_r, to_string_seps const& seps) const
{
	// symbolic tokens.
	constexpr auto arrow = [](double left, double right, to_string_seps const& seps) {
		if (left < right) return seps.up;
		if (left > right) return seps.down;
		return seps.flat;
	};
	constexpr std::wstring_view bra_l = L"[ ", bra_r = L" ]";

	// helper lambda.
	auto append = [n = std::lroundf(std::log10f(static_cast<float>(prec)))](std::wstring& left, double val) {
		wchar_t buf[std::bit_ceil(TrackInfo::max_value_len + 1)];
		static_cast<size_t>(::swprintf_s(buf, L"%.*f", n, val));
		left += buf;
	};

	// handle trivial cases.
	if (values.empty()) return L"";

	// prepare the returning storage.
	std::wstring ret{}; ret.reserve((TrackInfo::max_value_len + 1) * values.size() + 5);

	// construct the string.
	if (-1 == section) ret += bra_l;
	double prev;
	for (int i = 0; i < size(); i++) {
		double curr = values[i];
		if (i == 0) {
			if (overflow_l && more_l) ret += seps.overflow; // heading ellipsis.
		}
		else ret += arrow(prev, curr, seps);

		if (i == section) ret += bra_l;

		append(ret, curr);

		if (i == section + 1) ret += bra_r;

		if (i == size() - 1) {
			if (overflow_r && more_r) ret += seps.overflow; // tail ellipsis.
		}

		prev = curr;
	}
	if (size() == section + 1) ret += bra_r;
	return ret;
}

std::pair<int, std::vector<int>> expt::collect_pos_chain(ExEdit::Object const& obj)
{
	int i = &obj - (*exedit.ObjectArray_ptr);
	if (obj.index_midpt_leader < 0) return { 0, { i } };
	int j = obj.index_midpt_leader;

	int pos_chain = 0;
	std::vector<int> chain{ j };
	while (j = exedit.NextObjectIdxArray[j], j != -1) {
		if (i == j) pos_chain = chain.size();
		chain.push_back(j);
	}
	return { pos_chain, std::move(chain) };
}

std::vector<int> expt::collect_int_values(std::vector<int> const& chain, size_t idx_track)
{
	auto const* const objects = *exedit.ObjectArray_ptr;
	auto const& leading = objects[chain.front()];
	auto const mode = leading.track_mode[idx_track];
	auto const spec = easing_spec{ leading.track_mode[idx_track] };

	std::vector<int> values{ leading.track_value_left[idx_track] };
	if ((mode.num & 0x0f) == 0); // sole value.
	else if (spec.twopoints)
		values.push_back(leading.track_value_right[idx_track]);
	else {
		for (int i : chain)
			values.push_back(objects[i].track_value_right[idx_track]);
	}

	return values;
}

void expt::apply_int_values(std::vector<int> const& chain, std::vector<int> const& values, size_t idx_track)
{
	auto* const objects = *exedit.ObjectArray_ptr;
	auto& leading = objects[chain.front()];
	auto const mode = leading.track_mode[idx_track];
	auto const spec = easing_spec{ leading.track_mode[idx_track] };

	int val_l = values.front();
	if ((mode.num & 0x0f) == 0) {
		// apply uniformly.
		for (int i : chain) {
			auto& o = objects[i];
			o.track_value_left[idx_track] = o.track_value_right[idx_track] = val_l;
		}
	}
	else if (chain.size() == 1 || spec.twopoints) {
		// handle only two points.
		int const val_r = values.back();
		for (int i : chain) {
			auto& o = objects[i];
			o.track_value_left[idx_track] = val_l;
			o.track_value_right[idx_track] = val_r;
		}
	}
	else {
		// common cases.
		for (int pos = 0; pos < static_cast<int>(chain.size()); pos++) {
			int const val_r = values[pos + 1];
			auto& o = objects[chain[pos]];
			o.track_value_left[idx_track] = val_l;
			o.track_value_right[idx_track] = val_r;

			// update val_l.
			val_l = val_r;
		}
	}
}

int expt::convert_value_disp2int(double val, int denom, int prec, int min, int max)
{
	int value = static_cast<int>(std::round(val * denom));
	if (prec < denom) {
		// rounding. an internal value must be a multiple of (denom/prec).
		int const mod = denom / prec, hmod = mod >> 1;
		int r = value % mod;
		if (r < 0) r += mod;
		if (r >= hmod) r -= mod;
		value -= r;
	}
	return std::clamp(value, min, max);
}

std::pair<size_t, size_t> expt::find_filter_from_track(ExEdit::Object const& obj, size_t track_index)
{
	size_t filter_index = 0;
	for (; filter_index < std::size(obj.filter_param) - 1; filter_index++) {
		auto const filter = obj.filter_param[filter_index + 1];
		if (!filter.is_valid() ||
			track_index < static_cast<size_t>(filter.track_begin)) break;
	}
	return { filter_index, track_index - static_cast<size_t>(obj.filter_param[filter_index].track_begin) };
}
