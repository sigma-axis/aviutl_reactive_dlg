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
static inline script_name const& script_names(size_t idx) {
#ifndef _DEBUG
	constinit
#endif // !_DEBUG
	static std::vector<script_name> ret{};
	if (ret.empty()) {
		auto* ptr = exedit.track_script_names;
		while (*ptr != '\0') ret.emplace_back(ptr);
		ret.shrink_to_fit();
	}
	return ret[idx];
}

// built-in easing names.
static constexpr std::string_view basic_track_mode_names[]{
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
NS_END



////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings;

expt::easing_name_spec::easing_name_spec(ExEdit::Object::TrackMode const& mode)
{
	size_t const basic_idx = mode.num & 0x0f;
	bool const is_scr = basic_idx == mode.isScript;

	// name of the easing.
#pragma warning(suppress : 6385)
	name = is_scr ?
		script_names(mode.script_idx).name :
		basic_track_mode_names[basic_idx];

	// identify its specification, especially whether it accepts a parameter.
	spec = is_scr ? easing_spec{ exedit.easing_specs_script[mode.script_idx] } :
		easing_spec{ basic_idx, exedit.easing_specs_builtin[basic_idx] };
	if (!spec.loaded) {
		// script wasn't loaded yet. try after loading.
		exedit.load_easing_spec(mode.script_idx, 0, 0);
		spec = { exedit.easing_specs_script[mode.script_idx] };
	}
}

expt::formatted_values::formatted_values(ExEdit::Object const& obj, size_t idx_track) : vals{}
{
	auto [pos, chain] = collect_pos_chain(obj); section = pos;
	auto values = collect_int_values(chain, idx_track);

	if (values.size() == 1) section = -1; // 移動無し
	else if (values.size() == 2) section = 0; // 中間点なし or 中間点無視

	vals.reserve(values.size());
	double const d = exedit.trackinfo_left[idx_track].denominator();
	for (int value : values) vals.push_back(value / d);
}

expt::formatted_values::formatted_values(std::wstring src) : vals{}, section{}
{
	// helper lambdas.
	constexpr auto is_numeric = [](wchar_t c) {
		constexpr std::wstring_view numerics = L"+-.0123456789";
		return numerics.contains(c);
		//return c == L'+' || c == L'-' || c == L'.' || (L'0' <= c && c <= L'9');
	};
	constexpr auto find_unique = [](std::wstring const& src, wchar_t c) -> wchar_t const* {
		auto pos = src.find(c);
		if (pos == std::wstring::npos ||
			src.find(c, pos + 1) != std::wstring::npos) return nullptr;
		return src.data() + pos;
	};

	// limit the source to the first line of the string.
	src = src.erase(std::min(src.size(), src.find_first_of(L"\r\n")));

	// identify the bracket positions.
	auto pos_bra_l = find_unique(src, L'['), pos_bra_r = find_unique(src, L']');
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
		// two-long sequence shall implicitly specify the section unless specified.
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

std::wstring expt::formatted_valuespan::to_string(int prec, bool ellip_l, bool ellip_r, bool zigzag) const
{
	// symbolic tokens.
	constexpr auto arrow = [](double left, double right, bool zigzag) {
		if (zigzag) {
			if (left < right) return L'\u2197'; // North East Arrow.
			if (left > right) return L'\u2198'; // South East Arrow.
		}
		return L'\u2192'; // Rightwards Arrow.
	};
	constexpr wchar_t overflow = L'\u2026'; // Horizontal Ellipsis.
	constexpr std::wstring_view bra_l = L"[ ", bra_r = L" ]";

	// helper lambda.
	auto append = [n = static_cast<int>(0.5f + std::log10f(static_cast<float>(prec)))](std::wstring& left, double val) {
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
			if (ellip_l && more_l) ret += overflow; // heading "...".
		}
		else ret += arrow(prev, curr, zigzag);

		if (i == section) ret += bra_l;

		append(ret, curr);

		if (i == section + 1) ret += bra_r;

		if (i == size() - 1) {
			if (ellip_r && more_r) ret += overflow; // tail "...".
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
	return { pos_chain, chain };
}

std::vector<int> expt::collect_int_values(std::vector<int> const& chain, size_t idx_track)
{
	auto* const objects = *exedit.ObjectArray_ptr;
	auto& leading = objects[chain.front()];
	auto mode = leading.track_mode[idx_track];
	auto spec = easing_name_spec{ leading.track_mode[idx_track] }.spec;

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
	auto mode = leading.track_mode[idx_track];
	auto spec = easing_name_spec{ leading.track_mode[idx_track] }.spec;

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
		int val_r = values.back();
		for (int i : chain) {
			auto& o = objects[i];
			o.track_value_left[idx_track] = val_l;
			o.track_value_right[idx_track] = val_r;
		}
	}
	else {
		// common cases.
		for (int pos = 0; pos < static_cast<int>(chain.size()); pos++) {
			int val_r = values[pos + 1];
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
