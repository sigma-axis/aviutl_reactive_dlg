/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <vector>
#include <map>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "reactive_dlg.hpp"
#include "Filters_ScriptName.hpp"
#include "inifile_op.hpp"
#include "slim_formatter.hpp"
#include "str_encodes.hpp"

using namespace sigma_lib::string;


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// フィルタ効果のスクリプト名表示．
////////////////////////////////
using namespace reactive_dlg::Filters::ScriptName;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

#ifndef _DEBUG
constinit
#endif // !_DEBUG
static struct {
	// supply this struct with the heading pointer, turning the functionality enabled.
	void init(char const* head) { names.clear(); names.push_back(head); }

	// retrieves the name of the script for the fiter at the given index of the given object.
	// the filter is known to have the specific type of exdata.
	template<class ExDataT>
	char const* get(ExEdit::Object const& leader, ExEdit::Object::FilterParam const& filter_param) {
		ptrdiff_t offset = leader.exdata_offset + filter_param.exdata_offset;
		auto exdata = reinterpret_cast<ExDataT*>((*exedit.exdata_table) + offset + 0x0004);
		return exdata->name[0] != '\0' ? exdata->name : find(exdata->type);
	}

private:
	std::vector<char const*> names{};
	char const* find(size_t idx) {
		if (names.size() <= 1) collect();
		return idx < names.size() - 1 ? names[idx] : nullptr;
	}
	void collect()
	{
		// collect the heading pointers to each name.
		for (auto* head = names.back(); *head != '\0'; names.push_back(head))
			head += std::strlen(head) + 1;
		names.shrink_to_fit();
	}

} basic_anm_names, basic_obj_names, basic_cam_names, basic_scn_names;

// return the script name of the filter at the given index in the object,
// or nullptr if it's not a script-based filter, plus the filter id.
static inline std::pair<char const*, filter_id::id> get_script_name(ExEdit::Object const& obj, int32_t filter_index)
{
	auto& leader = obj.index_midpt_leader < 0 ? obj
		: (*exedit.ObjectArray_ptr)[obj.index_midpt_leader];
	auto& filter_param = leader.filter_param[filter_index];

	if (filter_param.is_valid()) {
		using anm_exdata = ExEdit::Exdata::efAnimationEffect; // shared with camera eff and custom object.
		using scn_exdata = ExEdit::Exdata::efSceneChange;

		switch (filter_param.id) {
		case filter_id::anim_eff:
			return { basic_anm_names.get<anm_exdata>(leader, filter_param), filter_id::anim_eff };

		case filter_id::cust_obj:
			return { basic_obj_names.get<anm_exdata>(leader, filter_param), filter_id::cust_obj };

		case filter_id::cam_eff:
			return { basic_cam_names.get<anm_exdata>(leader, filter_param), filter_id::cam_eff };

		case filter_id::scn_chg:
			return { basic_scn_names.get<scn_exdata>(leader, filter_param), filter_id::scn_chg };
		}
	}
	return { nullptr, filter_id::id{ -1 } };
}

// structure for cached formatted names and its rendered size.
struct name_cache {
	std::wstring caption = L"";
	int width = -1;	// width for the text only.

	constexpr bool is_valid() const { return width >= 0; }
	void init(HWND check_button, std::wstring const& text)
	{
		caption = text;

		// calculate the size for the text.
		HDC dc = ::GetDC(check_button);
		auto old_font = ::SelectObject(dc, reinterpret_cast<HFONT>(
			::SendMessageW(check_button, WM_GETFONT, 0, 0)));
		width = calc_text_width(dc, caption);
		::SelectObject(dc, old_font);
		::ReleaseDC(check_button, dc);
	}
	constexpr int wd_button() const { return width + extra_button_wd; }

	constexpr static int
		extra_button_wd = 18,	// width for check button adds this size.
		button_height = 14,
		gap_between_sep = 5,	// the separator leaves this margin.
		sep_height = 2;			// height for the separator.

	static int calc_text_width(HDC dc, std::wstring const& text)
	{
		RECT rc{};
		::DrawTextW(dc, text.c_str(), text.size(), &rc, DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE);
		return rc.right;
	}
};
// stores and manages caches.
class cache_manager {
	std::map<std::string, name_cache> cache;
	slim_formatter const formatter;
public:
	cache_manager(std::wstring const& fmt) : formatter{ fmt }, cache{} {}
	name_cache const& find_cache(char const* name, size_t idx) {
		auto& ret = cache[name];
		if (!ret.is_valid())
			ret.init(exedit.filter_checkboxes[idx], formatter(encode_sys::to_wide_str(name)));
		return ret;
	}
};
static constinit std::unique_ptr<cache_manager>
	anim_eff{},
	cust_std{}, cust_ext{}, cust_prtcl{},
	cam_eff{},
	scn_change{};

// find or create a cached formatted names at the given filter index of the selected object.
static inline name_cache const* find_script_name_cache(ExEdit::Object const& obj, size_t filter_index)
{
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

static inline size_t filter_index_from_script_combo(size_t id_combo,
	decltype(ExEdit::Object::filter_param) const& filter_param)
{
	// find the index of checks.
	constexpr size_t id_combo_base = 8100;
	size_t const idx_combo = id_combo - id_combo_base;
	if (idx_combo >= ExEdit::Object::MAX_CHECK) return ~0uz;

	// traverse the filter to find the filter index.
	for (size_t filter_index = 0; filter_index < std::size(filter_param); filter_index++) {
		auto const& filter = filter_param[filter_index];
		int const j = idx_combo - filter.check_begin;
		if (!filter.is_valid() || j < 0) break;
		switch (filter.id) {
		case filter_id::scn_chg:
			if (j == 2) break; else continue;
		case filter_id::anim_eff:
		case filter_id::cust_obj:
		case filter_id::cam_eff:
			if (j == 0 || j == 1) break; else continue;
		default: continue;
		}

		// found the desired index.
		return filter_index;
	}

	// no such found.
	return ~0uz;
}

// holding the current states of manipulation of the check buttons and the separators.
static constinit struct {
	size_t filter_index = 0;
	name_cache const* candidate = nullptr;

	constexpr void push(size_t filter_index, name_cache const* candidate) {
		this->filter_index = filter_index;
		this->candidate = candidate;
	}
	constexpr bool is_valid() const { return candidate != nullptr; }
	constexpr void invalidate() { candidate = nullptr; }

	// adjusts positions and sizes of the check button and the separator.
	void adjust_layout()
	{
		// the state must be valid.
		if (!is_valid()) return;

		HWND const dlg = *exedit.hwnd_setting_dlg,
			sep = exedit.filter_separators[filter_index],
			btn = exedit.filter_checkboxes[filter_index];

		// find the positions of the check button and the separator.
		RECT rc; ::GetWindowRect(sep, &rc);
		POINT sep_tl{ rc.left, rc.top }; ::ScreenToClient(dlg, &sep_tl);
		::GetWindowRect(btn, &rc);
		POINT btn_br{ rc.right, rc.bottom }; ::ScreenToClient(dlg, &btn_br);

		// adjust their sizes according to the precalculated width.
		::MoveWindow(btn, btn_br.x - candidate->wd_button(), btn_br.y - candidate->button_height,
			candidate->wd_button(), candidate->button_height, TRUE);
		::MoveWindow(sep, sep_tl.x, sep_tl.y,
			std::max<int>(btn_br.x - candidate->wd_button() - candidate->gap_between_sep - sep_tl.x, 0),
			candidate->sep_height, TRUE);

		// invalidate the state.
		invalidate();
	}
} hook_state;


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	auto ret = ::DefSubclassProc(hwnd, message, wparam, lparam);

	switch (message) {
	case WM_COMMAND:
		if (auto ctrl = reinterpret_cast<HWND>(lparam); ctrl != nullptr) {
			switch (wparam >> 16) {
				// 選択スクリプトの変更を監視．
			case CBN_SELCHANGE:
			{
				if (hook_state.is_valid()) break;
				auto const idx_obj = *exedit.SettingDialogObjectIndex;
				if (idx_obj < 0 || !check_window_class(ctrl, WC_COMBOBOXW)) break;
				auto const& obj = (*exedit.ObjectArray_ptr)[idx_obj];

				// find the filter index.
				auto filter_index = filter_index_from_script_combo(wparam & 0xffff, obj.filter_param);
				if (filter_index >= ExEdit::Object::MAX_FILTER) break;

				// alternative name must be found.
				auto* cache = find_script_name_cache(obj, filter_index);
				if (cache == nullptr) break;

				// then update the filter name and adjust the layout.
				hook_state.push(filter_index, cache);
				::SetWindowTextW(exedit.filter_checkboxes[filter_index], cache->caption.c_str());
				hook_state.adjust_layout();
				break;
			}
			}
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &setting_dlg_hook, id);
		break;
	}
	return ret;
}

// フィルタ効果のセパレータ．
static inline LRESULT CALLBACK filter_sep_hook(HWND hwnd, UINT message, WPARAM w, LPARAM l, auto id, auto data)
{
	switch (message) {
	case WM_SIZE:
	case WM_DESTROY:
	{
		::RemoveWindowSubclass(hwnd, &filter_sep_hook, id);
		if (message != WM_SIZE) break;

		// re-layout at the first call to this function.
		hook_state.adjust_layout();
		break;
	}
	}
	return ::DefSubclassProc(hwnd, message, w, l);
}

// フィルタ効果右上の名前表示チェックボックス．
static inline LRESULT CALLBACK filter_name_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_SETTEXT:
	{
		// wparam: not used, lparam: reinterpret_cast<wchar_t const*>(lparam) is the new text.

		if (hook_state.is_valid()) break;
		auto const idx_obj = *exedit.SettingDialogObjectIndex;
		if (idx_obj < 0) break;
		size_t const filter_idx = static_cast<size_t>(data);

		// alternative name must be found.
		auto* cache = find_script_name_cache((*exedit.ObjectArray_ptr)[idx_obj], filter_idx);
		if (cache == nullptr) break;

		// prepare the hook for the next call of WM_SIZE to the separator.
		hook_state.push(filter_idx, cache);
		::SetWindowSubclass(exedit.filter_separators[filter_idx], &filter_sep_hook, id, data);

		// replace with the new window text.
		lparam = reinterpret_cast<LPARAM>(cache->caption.c_str());
		break;
	}

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &filter_name_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Filters::ScriptName;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.is_enabled()) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			if (settings.anim_eff_fmt)		anim_eff	= std::make_unique<cache_manager>(*settings.anim_eff_fmt);
			if (settings.cust_std_fmt)		cust_std	= std::make_unique<cache_manager>(*settings.cust_std_fmt);
			if (settings.cust_ext_fmt)		cust_ext	= std::make_unique<cache_manager>(*settings.cust_ext_fmt);
			if (settings.cust_particle_fmt)	cust_prtcl	= std::make_unique<cache_manager>(*settings.cust_particle_fmt);
			if (settings.scn_change_fmt)	scn_change	= std::make_unique<cache_manager>(*settings.scn_change_fmt);
			if (settings.cam_eff_fmt)		cam_eff		= std::make_unique<cache_manager>(*settings.cam_eff_fmt);

			basic_anm_names.init(exedit.basic_animation_names);
			basic_obj_names.init(exedit.basic_custom_obj_names);
			basic_cam_names.init(exedit.basic_camera_eff_names);
			basic_scn_names.init(exedit.basic_scene_change_names);

			::SetWindowSubclass(dlg, &setting_dlg_hook, hook_uid(), {});
			for (size_t i = 0; i < ExEdit::Object::MAX_FILTER; i++)
				::SetWindowSubclass(exedit.filter_checkboxes[i], &filter_name_hook,	hook_uid(), { i });
		}
		else {
			::RemoveWindowSubclass(dlg, &setting_dlg_hook, hook_uid());
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Filters.ScriptName";
	constexpr size_t max_str_len = 256;

#define read(fld)	if (auto s = read_ini_string(u8"", ini_file, section, #fld, max_str_len); s != L"") fld = std::make_unique<std::wstring>(std::move(s))

	read(anim_eff_fmt);
	read(cust_std_fmt); read(cust_ext_fmt); read(cust_particle_fmt);
	read(cam_eff_fmt);
	read(scn_change_fmt);

#undef read
}

void expt::Settings::check_conflict(char const* this_plugin_name)
{
	if (is_enabled() && warn_conflict(L"filter_name.auf",
		L"[Filters.ScriptName]\n"
		L"anim_eff_fmt=\"\"\n"
		L"cust_std_fmt=\"\"\n"
		L"cust_ext_fmt=\"\"\n"
		L"cust_particle_fmt=\"\"\n"
		L"cam_eff_fmt=\"\"\n"
		L"scn_change_fmt=\"\"", this_plugin_name)) {
		
		anim_eff_fmt.reset();
		cust_std_fmt.reset();
		cust_ext_fmt.reset();
		cust_particle_fmt.reset();
		cam_eff_fmt.reset();
		scn_change_fmt.reset();
	}
}
