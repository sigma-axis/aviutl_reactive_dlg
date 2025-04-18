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
#include <bit>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "inifile_op.hpp"
#include "str_encodes.hpp"
#include "monitors.hpp"

#include "reactive_dlg.hpp"
#include "Tooltip.hpp"
#include "Easings.hpp"
#include "Filters_Tooltip.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// フィルタのツールチップ表示．
////////////////////////////////
using namespace reactive_dlg::Filters::Tooltip;
namespace common = reactive_dlg::Tooltip;
using common::tooltip, common::bgr2rgb, reactive_dlg::Easings::easing_name_spec;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static inline std::wstring button_text(HWND hwnd)
{
	std::wstring ret{};
	ret.resize_and_overwrite(::SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0), [&](wchar_t* ptr, size_t sz)
	{
		return ::SendMessageW(hwnd, WM_GETTEXT, sz + 1, reinterpret_cast<LPARAM>(ptr));
	});
	return ret;
}

static inline std::wstring format_index(size_t filter_index, ExEdit::Object const& obj, ExEdit::Filter const* filter, ExEdit::Filter const* out_filter)
{
	wchar_t buf[8];
	return { buf , static_cast<size_t>(::swprintf_s(buf, L"%d/%d", filter_index + 1,
		obj.countFilters() - (out_filter != nullptr ? 1 : 0))) };
}

static inline std::wstring format_trackbars(size_t filter_index, ExEdit::Object const& obj, ExEdit::Filter const* filter)
{
	using sigma_lib::string::encode_sys;

	wchar_t buf[std::bit_ceil(TrackInfo::max_value_len + 3)];

	std::wstring ret = L"";
	for (int rel_idx = 0; rel_idx < filter->track_n; rel_idx++) {
		size_t const index = rel_idx + obj.filter_param[filter_index].track_begin;
		auto const& mode = obj.track_mode[index];

		// skip stationary track if specified.
		bool const stationary = (mode.num & 0x0f) == 0;
		if (settings.trackbars == Settings::trackbar_level::moving && stationary) continue;

		auto const& track_info = exedit.trackinfo_left[index];
		auto const digits = std::lround(std::log10f(static_cast<float>(track_info.precision())));

		// write the left value.
		ret.append(button_text(exedit.hwnd_track_buttons[index]));
		ret.append(buf, ::swprintf_s(buf, L": %.*f", digits,
			track_info.calc_value(obj.track_value_left[index])));

		if (!stationary) {
			// append the right value.
			ret.append(L" → ");
			ret.append(buf, ::swprintf_s(buf, L"%.*f; ", digits,
				track_info.calc_value(obj.track_value_right[index])));

			// append the easing name.
			ret.append(encode_sys::to_wide_str(easing_name_spec{ mode }.name));
		}
		ret.append(1, L'\n');
	}

	if (!ret.empty()) ret.pop_back(); // pop the trailing line break.
	return ret;
}

static inline std::wstring format_checks(size_t filter_index, ExEdit::Object const& obj, ExEdit::Filter const* filter)
{
	using sigma_lib::string::encode_sys;

	ExEdit::Object const& leader = obj.index_midpt_leader < 0 ? obj : (*exedit.ObjectArray_ptr)[obj.index_midpt_leader];

	std::wstring ret = L"";
	for (int rel_idx = 0; rel_idx < filter->check_n; rel_idx++) {
		if (filter->check_default[rel_idx] < 0) continue; // skip buttons and combo boxes.

		size_t const index = rel_idx + obj.filter_param[filter_index].check_begin;

		// append the check name and state.
		ret.append(encode_sys::to_wide_str(filter->check_name[rel_idx]));
		ret.append(leader.check_value[index] == 0 ? L": OFF\n" : L": ON\n");
	}

	if (!ret.empty()) ret.pop_back(); // pop the trailing line break.
	return ret;
}

static inline bool next_exdata(ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain) {
	// move the given pointers.
	data += use->size;
	size_remain -= use->size;
	use++;
	return true;
}

static inline bool parse_as_file(std::wstring& s, ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain)
{
	using Type = ExEdit::ExdataUse::Type;
	using sigma_lib::string::encode_sys;
	constexpr static std::string_view token_file = "file";
	constexpr static std::wstring_view path_delimiter = L"/\\", path_ellipsis = L"...";

	if (use->name == token_file &&
		use->size >= 0x100 &&
		use->type == Type::String) {

		// append a line.
		s.append(L"ファイル: ");

		auto str = reinterpret_cast<char const*>(data);
		if (str[0] == '\0') s.append(L"指定なし");
		else {
			auto path = encode_sys::to_wide_str(str);

			// trim the path upto the parent directory.
			if (auto pos = path.find_last_of(path_delimiter);
				pos != path.npos && pos > 0) {
				pos = path.find_last_of(path_delimiter, pos - 1);
				if (pos != path.npos && pos > 0)
					path.replace(0, pos - 1, path_ellipsis);
			}
			s.append(path);
		}

		return next_exdata(use, data, size_remain);
	}
	return false;
}

static inline bool parse_as_color(std::wstring& s, ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain)
{
	using Type = ExEdit::ExdataUse::Type;
	using sigma_lib::string::encode_sys;
	constexpr static std::string_view token_color = "color",
		token_no_color = "no_color", char_digits = "0123456789";

	std::string_view const use_name = use->name;
	if (use_name.starts_with(token_color) &&
		use_name.find_first_not_of(char_digits, token_color.size()) == use_name.npos &&
		use->size == 3 &&
		use->type == Type::Binary) {

		// the index of the color in the form of string.
		std::string_view const color_idx = use_name.substr(token_color.size());

		// find no_color flag.
		int8_t no_color = -1; // -1: no such flag, 0: flase, 1: true.
		if (size_remain > 3 && use[1].name != nullptr) {
			std::string_view use1_name = use[1].name;
			if (use1_name.starts_with(token_no_color) &&
				color_idx == use1_name.substr(token_no_color.size()) &&
				use[1].size == 1 &&
				use[1].type == Type::Number)
				no_color = data[3] == 0 ? 0 : 1;
		}

		// append a line.
		s.append(L"色" + encode_sys::to_wide_str(color_idx) + L": ");

		if (no_color > 0) s.append(L"指定なし");
		else {
			wchar_t buf[8];
			s.append(buf, ::swprintf_s(buf, L"#%02x%02x%02x", data[0], data[1], data[2]));
		}

		// move the given pointers.
		if (no_color >= 0) // move twice.
			next_exdata(use, data, size_remain);
		return next_exdata(use, data, size_remain);
	}
	return false;
}

static inline bool parse_as_blend(std::wstring& s, ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain)
{
	using Type = ExEdit::ExdataUse::Type;
	using sigma_lib::string::encode_sys;
	constexpr static std::string_view token_blend = "blend";

	if (use->name == token_blend &&
		use->size == 4 &&
		use->type == Type::Number) {

		s.append(L"合成モード: ");

		// take the name from the filter "standard drawing".
		std::string_view name_src = exedit.loaded_filter_table[filter_id::draw_std]->check_name[0];
		for (auto idx = *reinterpret_cast<int const*>(data); idx > 0; idx--) {
			if (name_src.size() == 0) return false; // might not be the blend mode in the context.
			name_src = name_src.data() + name_src.size() + 1;
		}
		s.append(encode_sys::to_wide_str(name_src));

		return next_exdata(use, data, size_remain);
	}
	return false;
}

static inline bool parse_as_text(std::wstring& s, ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain)
{
	using Type = ExEdit::ExdataUse::Type;
	constexpr static std::string_view token_text = "text";
	constexpr static std::wstring_view text_ellipsis = L"...", line_breaks = L"\r\n";
	constexpr static size_t min_size_text = 256, max_heading_chars = 64;

	if (use->name == token_text &&
		use->size >= min_size_text && use->size % sizeof(wchar_t) == 0 &&
		use->type == Type::Binary) {

		std::wstring_view text{ reinterpret_cast<wchar_t const*>(data), use->size / sizeof(wchar_t) };
		text = text.substr(0, text.find_first_of(L'\0'));

		// trim the line breaks.
		if (auto pos = text.find_first_not_of(line_breaks); pos != text.npos)
			text = text.substr(pos);
		if (auto pos = text.find_last_not_of(line_breaks); pos != text.npos)
			text = text.substr(0, pos + 1);

		// leave only non-empty text.
		if (!text.empty()) {
			s.append(text.substr(0, max_heading_chars));
			if (text.size() > max_heading_chars) s.append(text_ellipsis);

			return next_exdata(use, data, size_remain);
		}
	}
	return false;
}

static inline bool parse_as_scene(std::wstring& s, ExEdit::ExdataUse const*& use, byte const*& data, int& size_remain)
{
	using Type = ExEdit::ExdataUse::Type;
	using sigma_lib::string::encode_sys;
	constexpr static std::string_view token_scene = "scene";
	constexpr static int max_scenes = 50;

	if (use->name == token_scene &&
		use->size == 4 &&
		use->type == Type::Number) {

		int n = *reinterpret_cast<int const*>(data);
		s.append(L"シーン: ");

		// compose the scene name.
		std::wstring scene_name{};
		if (0 <= n && n < max_scenes) {
			if (n == 0) scene_name = L"Root";
			else {
				wchar_t buf[16];
				scene_name = { buf, static_cast<size_t>(::swprintf_s(buf, L"Scene %d", n)) };
			}

			// if it has a custom name, place it at the head.
			if (auto const* name = exedit.scene_settings[n].name;
				name != nullptr && name[0] != '\0')
				scene_name = encode_sys::to_wide_str(name)
					+ L" (" + scene_name + L")";
		}
		else scene_name = L"(指定なし)";
		s.append(scene_name);

		return next_exdata(use, data, size_remain);
	}
	return false;
}

static inline std::wstring format_exdata(size_t filter_index, ExEdit::Object const& obj, ExEdit::Filter const* filter)
{
	using sigma_lib::string::encode_sys;

	ExEdit::Object const& leader = obj.index_midpt_leader < 0 ? obj : (*exedit.ObjectArray_ptr)[obj.index_midpt_leader];
	byte const* data = find_exdata<byte>(leader.exdata_offset, leader.filter_param[filter_index].exdata_offset);
	auto const* use = filter->exdata_use;
	int size_remain = filter->exdata_size;

	std::wstring ret = L"";
	while (size_remain > 0 && use->size <= size_remain) {
		if (use->name != nullptr && use->type != ExEdit::ExdataUse::Type::Padding) {
			if (parse_as_file(ret, use, data, size_remain) ||
				parse_as_color(ret, use, data, size_remain) ||
				parse_as_blend(ret, use, data, size_remain) ||
				parse_as_text(ret, use, data, size_remain) ||
				parse_as_scene(ret, use, data, size_remain)) {
				ret.append(1, L'\n');
				continue;
			}
		}

		// move to the next data.
		next_exdata(use, data, size_remain);
	}

	if (!ret.empty()) ret.pop_back(); // pop the trailing line break.
	return ret;
}

// storage of the tooltip content.
#ifndef _DEBUG
constinit
#endif // !_DEBUG
static struct tooltip_content {
	int width, height;

	std::wstring name, index, trackbars, checks, exdata;
	int pos_x_index, pos_y_tracks, pos_y_checks, pos_y_exdata;

	inline void measure(size_t filter_index, HDC dc);
	inline void draw(HDC dc, RECT const& rc) const;
	constexpr bool is_valid() const { return width > 0 && height > 0; }
	void invalidate() { width = height = 0; }

private:
	constexpr static int gap_cols = 12, gap_header = 4, gap_rows = 2;
	constexpr static UINT draw_text_options = DT_NOCLIP | DT_NOPREFIX;

} content{};


////////////////////////////////
// Tooltip drawings.
////////////////////////////////
void tooltip_content::measure(size_t filter_index, HDC dc)
{
	if (is_valid()) return;

	constexpr auto concat = [](std::wstring const& l, std::wstring const& r) -> std::wstring {
		if (l.empty()) return r;
		else if (r.empty()) return l;
		return l + L'\n' + r;
	};

	int const obj_index = *exedit.SettingDialogObjectIndex;
	auto const* const objects = *exedit.ObjectArray_ptr;
	auto const& obj = objects[obj_index];
	auto const* filter = exedit.loaded_filter_table[obj.filter_param[filter_index].id];
	size_t const count_filters = obj.countFilters();
	auto const* out_filter = exedit.loaded_filter_table[obj.filter_param[count_filters - 1].id];
	if (!has_flag_or(out_filter->flag, ExEdit::Filter::Flag::Output))
		out_filter = nullptr;

	// format each element.
	name = button_text(exedit.filter_checkboxes[filter_index]);
	index = format_index(filter_index, obj, filter, out_filter);

	if (settings.trackbars != Settings::trackbar_level::none) {
		trackbars = format_trackbars(filter_index, obj, filter);
		if (filter_index == 0 && out_filter != nullptr)
			trackbars = concat(format_trackbars(count_filters - 1, obj, out_filter), trackbars);
	}

	if (settings.checks) {
		checks = format_checks(filter_index, obj, filter);
		if (filter_index == 0 && out_filter != nullptr)
			checks = concat(format_checks(count_filters - 1, obj, out_filter), checks);
	}

	if (settings.exdata) {
		exdata = format_exdata(filter_index, obj, filter);
		if (filter_index == 0 && out_filter != nullptr)
			exdata = concat(format_exdata(count_filters - 1, obj, out_filter), exdata);
	}

	// measure those text.
	RECT rc_nm{}, rc_idx{}, rc_tr{}, rc_chk{}, rc_ex{};
	::DrawTextW(dc, name.c_str(), name.size(),
		&rc_nm, DT_CALCRECT | draw_text_options | DT_SINGLELINE);
	::DrawTextW(dc, index.c_str(), index.size(),
		&rc_idx, DT_CALCRECT | draw_text_options | DT_SINGLELINE);
	if (!trackbars.empty())
		::DrawTextW(dc, trackbars.c_str(), trackbars.size(),
			&rc_tr, DT_CALCRECT | draw_text_options);
	if (!checks.empty())
		::DrawTextW(dc, checks.c_str(), checks.size(),
			&rc_chk, DT_CALCRECT | draw_text_options);
	if (!exdata.empty())
		::DrawTextW(dc, exdata.c_str(), exdata.size(),
			&rc_ex, DT_CALCRECT | draw_text_options);

	// calculate the layout.
	int w_nm = rc_nm.right - rc_nm.left, h_nm = rc_nm.bottom - rc_nm.top,
		w_idx = rc_idx.right - rc_idx.left, h_idx = rc_idx.bottom - rc_idx.top,
		w_tr = rc_tr.right - rc_tr.left, h_tr = rc_tr.bottom - rc_tr.top,
		w_chk = rc_chk.right - rc_chk.left, h_chk = rc_chk.bottom - rc_chk.top,
		w_ex = rc_ex.right - rc_ex.left, h_ex = rc_ex.bottom - rc_ex.top;

	width = std::max({ w_nm + gap_cols + w_idx, w_tr, w_chk, w_ex });
	pos_x_index = width - w_idx;

	height = h_nm; int gap = gap_header;
	if (h_tr > 0) pos_y_tracks = height + gap, height = pos_y_tracks + h_tr, gap = gap_rows;
	if (h_chk > 0) pos_y_checks = height + gap, height = pos_y_checks + h_chk, gap = gap_rows;
	if (h_ex > 0) pos_y_exdata = height + gap, height = pos_y_exdata + h_ex, gap = gap_rows;
}

void tooltip_content::draw(HDC dc, RECT const& rc) const
{
	if (!is_valid()) return;

	// actual drawing, using content.easing and content.values.
	{
		RECT rc2 = rc;
		::DrawTextW(dc, name.c_str(), name.size(),
			&rc2, draw_text_options | DT_SINGLELINE);
		rc2.left = pos_x_index;
		::DrawTextW(dc, index.c_str(), index.size(),
			&rc2, draw_text_options | DT_SINGLELINE);
	}
	if (!trackbars.empty()) {
		RECT rc2 = rc;
		rc2.top = pos_y_tracks;
		::DrawTextW(dc, trackbars.c_str(), trackbars.size(),
			&rc2, draw_text_options);
	}
	if (!checks.empty()) {
		RECT rc2 = rc;
		rc2.top = pos_y_checks;
		::DrawTextW(dc, checks.c_str(), checks.size(),
			&rc2, draw_text_options);
	}
	if (!exdata.empty()) {
		RECT rc2 = rc;
		rc2.top = pos_y_exdata;
		::DrawTextW(dc, exdata.c_str(), exdata.size(),
			&rc2, draw_text_options);
	}
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK filter_header_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	if (*exedit.SettingDialogObjectIndex < 0) return ::DefSubclassProc(hwnd, message, wparam, lparam);

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
		// relayed messages.
		MSG msg{ .hwnd = hwnd, .message = message, .wParam = wparam, .lParam = lparam };
		::SendMessageW(tooltip, TTM_RELAYEVENT,
			message == WM_MOUSEMOVE ? ::GetMessageExtraInfo() : 0, reinterpret_cast<LPARAM>(&msg));
		break;
	}

	case WM_NOTIFY:
		if (auto const hdr = reinterpret_cast<NMHDR*>(lparam); hdr->hwndFrom == tooltip) {
			constexpr auto dummy_text_a = "\t";
			constexpr auto dummy_text_w = L"\t";

			switch (hdr->code) {
			case TTN_GETDISPINFOA:
			{
				// supply the content string, which is this time a dummy,
				// or nothing if tooltip has no use for the current state.
				size_t idx = static_cast<size_t>(data);
				auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
				if (idx < static_cast<size_t>(obj.countFilters()) &&
					has_flag_or(obj.filter_status[idx], ExEdit::Object::FilterStatus::Folding)) {
					reinterpret_cast<NMTTDISPINFOA*>(lparam)
						->lpszText = const_cast<char*>(dummy_text_a);
					content.invalidate();
				}
				return {};
			}
			case TTN_SHOW:
			{
				// adjust the tooltip size to fit with the content.
				RECT rc;
				::GetWindowRect(tooltip, &rc);
				::SendMessageW(tooltip, TTM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rc));
				rc.right = rc.left + content.width + 2; // add slight extra space on the right and bottom.
				rc.bottom = rc.top + content.height + 1;
				::SendMessageW(tooltip, TTM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&rc));

				// adjust the position not to clip edges of screens.
				rc = sigma_lib::W32::monitor<true>{ rc.left, rc.top }
				.expand(-8).clamp(rc); // 8 pixels of padding.
				::SetWindowPos(tooltip, nullptr, rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return TRUE;
			}

			case NM_CUSTOMDRAW:
			{
				auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
				switch (dhdr->nmcd.dwDrawStage) {
				case CDDS_PREPAINT:
				{
					// prepare the tooltip content.
					content.measure(static_cast<size_t>(data), dhdr->nmcd.hdc);

					// actual drawing is done on the CDDS_POSTPAINT notification.
					if (::IsWindowVisible(tooltip) != FALSE)
						return CDRF_NOTIFYPOSTPAINT;
					break;
				}
				case CDDS_POSTPAINT:
				{
					// change the text color if specified.
					if (common::settings.text_color >= 0)
						::SetTextColor(dc, bgr2rgb(common::settings.text_color));

					// draw the content.
					content.draw(dhdr->nmcd.hdc, dhdr->nmcd.rc);
					break;
				}
				}
				return CDRF_DODEFAULT;
			}
			}
		}
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Filters::Tooltip;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.enabled) {
		common::setup(hwnd, initializing, hook_uid());
		if (initializing) {
			// associate the tooltip with each filter check box and expander.
			TTTOOLINFOW ti{
				.cbSize = TTTOOLINFOW_V1_SIZE,
				.uFlags = TTF_IDISHWND | TTF_TRANSPARENT,
				.hinst = exedit.fp->hinst_parent,
				.lpszText = LPSTR_TEXTCALLBACKW,
			};
			for (size_t i = 0; i < ExEdit::Object::MAX_FILTER; i++) {
				for (auto tgt : { exedit.filter_checkboxes[i], exedit.filter_expanders[i] }) {
					ti.uId = reinterpret_cast<uintptr_t>(ti.hwnd = tgt);
					::SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
					::SetWindowSubclass(tgt, &filter_header_hook, hook_uid(), { i });
				}
			}
		}
		else {
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Filters.Tooltip";
	constexpr size_t max_str_len = 31;

#define read(func, hd, fld, ...)	hd fld = read_ini_##func(hd fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)
#define read_s(fld)	if (auto s = read_ini_string(u8"", ini_file, section, #fld, max_str_len); s != L"") fld = std::make_unique<std::wstring>(std::move(s))

	read(bool,,	enabled);
	read(int,,	trackbars);
	read(bool,,	checks);
	read(bool,,	exdata);

#undef read_s
#undef read
}
