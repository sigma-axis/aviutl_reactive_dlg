/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <tuple>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

#include "inifile_op.hpp"
#include "modkeys.hpp"

#include "reactive_dlg.hpp"
#include "TrackLabel.hpp"
#include "Track_Button.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// トラックバーの周りのボタン操作．
////////////////////////////////
using namespace reactive_dlg::Track;
using namespace reactive_dlg::Track::Button;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_NOTIFY:
		// same strat as updown plugin by nanypoco:
		// https://github.com/nanypoco/updown
		if (*exedit.SettingDialogObjectIndex >= 0) {
			// check if it's from an UpDown control.
			auto* header = reinterpret_cast<NMHDR*>(lparam);
		//#pragma warning(suppress: 26454) // overflow warning by the macro UDN_DELTAPOS.
			if (header == nullptr || header->code != UDN_DELTAPOS ||
				header->hwndFrom == nullptr ||
				!check_window_class(header->hwndFrom, UPDOWN_CLASSW)) break;

			// see if it's a part of the trackbars.
			// the previous should be the label, from which I can find the id of the track.
			auto const* info = find_trackinfo(::GetWindow(header->hwndFrom, GW_HWNDPREV));
			if (info == nullptr || info->hwnd_updown != header->hwndFrom ||
				!info->is_visible()) break;

			// combinations of modifier keys that don't make sense shall not take action.
			auto keys = curr_modkeys();
			if (!settings.no_wrong_keys(keys & ~mk::modkeys::ctrl) || key_pressed_any(VK_LWIN, VK_RWIN)) break;

			// modify the delta value taking the modifier keys in account.
			reinterpret_cast<NMUPDOWN*>(header)->iDelta *= settings.calc_rate(keys, info->precision());
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, setting_dlg_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Track::Button;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.enabled) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			::SetWindowSubclass(dlg, &setting_dlg_hook, hook_uid(), {});
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

	constexpr auto section = "Track.Button";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(modkey,keys_frac);
	read(modkey,keys_boost);
	read(bool,	def_frac);
	read(int,	rate_boost, min_rate_boost, max_rate_boost);
	read(bool,	enabled);

#undef read
}

void expt::Settings::check_conflict(char const* this_plugin_name)
{
	if (enabled && warn_conflict(L"updown.auf", L"[Track.Button]\nmodify=0", this_plugin_name))
		enabled = false;
}
