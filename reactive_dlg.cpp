/*
The MIT License (MIT)

Copyright (c) 2024-2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cstring>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>

#include "str_encodes.hpp"

#include "reactive_dlg.hpp"
#include "TextBox.hpp"
#include "Dropdown_Keyboard.hpp"
#include "TrackLabel.hpp"
#include "Track_Keyboard.hpp"
#include "Track_Mouse.hpp"
#include "Track_Button.hpp"
#include "Filters_ScriptName.hpp"
#include "Easings_Misc.hpp"
#include "Easings_Tooltip.hpp"
#include "Easings_ContextMenu.hpp"

using namespace reactive_dlg;


////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
bool ExEdit092::init(AviUtl::FilterPlugin* this_fp)
{
	constexpr char const* info_exedit092 = "拡張編集(exedit) version 0.92 by ＫＥＮくん";

	if (fp != nullptr) return true;
	AviUtl::SysInfo si; this_fp->exfunc->get_sys_info(nullptr, &si);
	for (int i = 0; i < si.filter_n; i++) {
		auto that_fp = this_fp->exfunc->get_filterp(i);
		if (that_fp->information != nullptr &&
			0 == std::strcmp(that_fp->information, info_exedit092)) {
			fp = that_fp;
			init_pointers(fp->dll_hinst, fp->hinst_parent);
			return true;
		}
	}
	return false;
}


////////////////////////////////
// 各機能に共有の関数実装．
////////////////////////////////

// 設定ダイアログのデータ表示を更新．
void update_setting_dialog(int index)
{
	auto hdlg = *exedit.hwnd_setting_dlg;
	::SendMessageW(hdlg, WM_SETREDRAW, FALSE, {});
	exedit.update_setting_dlg(index);
	::SendMessageW(hdlg, WM_SETREDRAW, TRUE, {});
	::UpdateWindow(hdlg);
}

// 編集データの変更でメイン画面を更新．
void update_current_frame()
{
	constexpr UINT msg_update_main = WM_USER + 1;
	::PostMessageW(exedit.fp->hwnd, msg_update_main, 0, 0);
}

// 競合通知メッセージ．
bool warn_conflict(wchar_t const* module_name, wchar_t const* ini_piece, char const* this_plugin_name)
{
	if (::GetModuleHandleW(module_name) == nullptr) return false;
#pragma warning(suppress : 6387)
	::MessageBoxW(exedit.fp->hwnd_parent, (std::wstring{ module_name } +
		L" と競合しているため一部機能を無効化しました．\n"
		"次回以降このメッセージを表示させないためには，設定ファイルで以下の設定をしてください:\n\n" +
		ini_piece).c_str(),
		sigma_lib::string::encode_sys::to_wide_str(this_plugin_name).c_str(), MB_OK | MB_ICONEXCLAMATION);
	return true;
}


////////////////////////////////
// ファイルパス操作．
////////////////////////////////
template<class T, size_t len_max, size_t len_old, size_t len_new>
static void replace_tail(T(&dst)[len_max], size_t len, T const(&tail_old)[len_old], T const(&tail_new)[len_new])
{
	// replaces a file extension when it's known.
	if (len < len_old || len - len_old + len_new > len_max) return;
	std::memcpy(dst + len - len_old, tail_new, len_new * sizeof(T));
}


////////////////////////////////
// 各種コマンド．
////////////////////////////////
namespace Menu
{
	enum class ID : uint32_t {
		TextFocusFwd,
		TextFocusBwd,

		TrackFocusFwd,
		TrackFocusBwd,

		Invalid,
	};
	static constexpr bool is_invalid(ID id) { return id >= ID::Invalid; }

	using enum ID;
	static constexpr struct { ID id; char const* name; } Items[] = {
		{ TextFocusFwd,		"テキストにフォーカス移動" },
		{ TextFocusBwd,		"テキストにフォーカス移動(逆順)" },
		{ TrackFocusFwd,	"トラックバーにフォーカス移動" },
		{ TrackFocusBwd,	"トラックバーにフォーカス移動(逆順)" },
	};
}

inline bool menu_command_handler(Menu::ID id)
{
	constexpr auto fallback = [] {
		// force the focus onto the timeline window if no target is found.
		::SetFocus(exedit.fp->hwnd);
	};

	bool forward = false;
	switch (id) {
	case Menu::TextFocusFwd:
		forward = true;
		[[fallthrough]];
	case Menu::TextFocusBwd:
		if (auto target = TextBox::next_edit_box(nullptr, forward);
			target != nullptr) ::SetFocus(target);
		else fallback();
		break;

	case Menu::TrackFocusFwd:
		forward = true;
		[[fallthrough]];
	case Menu::TrackFocusBwd:
		if (auto target = Track::first_trackinfo(forward);
			target != nullptr) {
			::SetFocus(target->hwnd_label);
			set_edit_selection_all(target->hwnd_label);
		}
		else fallback();
		break;
	}
	return false;
}


////////////////////////////////
// AviUtlに渡す関数の定義．
////////////////////////////////
BOOL func_init(AviUtl::FilterPlugin* fp)
{
	// 情報源確保．
	if (!exedit.init(fp)) {
		::MessageBoxW(fp->hwnd_parent, L"拡張編集0.92が見つかりませんでした．",
			sigma_lib::string::encode_sys::to_wide_str(fp->name).c_str(),
			MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// message-only window を作成，登録．これで NoConfig でも AviUtl からメッセージを受け取れる.
	fp->hwnd = ::CreateWindowExW(0, L"AviUtl", L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, nullptr, fp->hinst_parent, nullptr);

	// メニュー登録．
	for (auto [id, name] : Menu::Items)
		fp->exfunc->add_menu_item(fp, name, fp->hwnd, static_cast<int32_t>(id), 0, AviUtl::ExFunc::AddMenuItemFlag::None);

	// 設定ロード．
	char ini_file[MAX_PATH];
	replace_tail(ini_file, ::GetModuleFileNameA(fp->dll_hinst, ini_file, std::size(ini_file)) + 1, "auf", "ini");
	TextBox::				settings.load(ini_file);
	Dropdown::Keyboard::	settings.load(ini_file);
	Track::Keyboard::		settings.load(ini_file);
	Track::Mouse::			settings.load(ini_file);
	Track::Button::			settings.load(ini_file);
	Filters::ScriptName::	settings.load(ini_file);
	Easings::Misc::			settings.load(ini_file);
	Easings::ContextMenu::	settings.load(ini_file);
	Easings::Tooltip::		settings.load(ini_file);

	// 競合確認．
	Track::Button::	settings.check_conflict(fp->name);
	Filters::ScriptName::settings.check_conflict(fp->name);

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	using Message = AviUtl::FilterPlugin::WindowMessage;
	switch (message) {
		// 仕込みの設定/解除．
	case Message::ChangeWindow:
		TextBox::				setup(hwnd, true);
		Dropdown::Keyboard::	setup(hwnd, true);
		Track::Keyboard::		setup(hwnd, true);
		Track::Mouse::			setup(hwnd, true);
		Track::Button::			setup(hwnd, true);
		Filters::ScriptName::	setup(hwnd, true);
		Easings::Misc::			setup(hwnd, true);
		Easings::ContextMenu::	setup(hwnd, true);
		Easings::Tooltip::		setup(hwnd, true);
		break;
	case Message::Exit:
		// at this moment, the setting dialog is already destroyed.
		Easings::Tooltip::		setup(hwnd, false);
		Easings::ContextMenu::	setup(hwnd, false);
		Easings::Misc::			setup(hwnd, false);
		Filters::ScriptName::	setup(hwnd, false);
		Track::Button::			setup(hwnd, false);
		Track::Mouse::			setup(hwnd, false);
		Track::Keyboard::		setup(hwnd, false);
		Dropdown::Keyboard::	setup(hwnd, false);
		TextBox::				setup(hwnd, false);

		// message-only window を削除．必要ないかもしれないけど．
		fp->hwnd = nullptr; ::DestroyWindow(hwnd);
		break;

	case PrivateMsg::RequestCallback:
		if (auto const callback = reinterpret_cast<PrivateMsg::callback_fnptr>(lparam);
			callback != nullptr) callback(hwnd, wparam);
		break;

		// command handlers.
	case Message::Command:
		if (auto id = static_cast<Menu::ID>(wparam); !Menu::is_invalid(id) &&
			fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp)) {

			return menu_command_handler(id) ? TRUE : FALSE;
		}
		break;
	}
	return FALSE;
}


////////////////////////////////
// Entry point.
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"Reactive Dialog"
#define PLUGIN_VERSION	"v2.00-beta4"
#define PLUGIN_AUTHOR	"sigma-axis"
#define PLUGIN_INFO_FMT(name, ver, author)	(name " " ver " by " author)
#define PLUGIN_INFO		PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR)

extern "C" __declspec(dllexport) AviUtl::FilterPluginDLL* __stdcall GetFilterTable(void)
{
	// （フィルタとは名ばかりの）看板．
	using Flag = AviUtl::FilterPlugin::Flag;
	static constinit AviUtl::FilterPluginDLL filter{
		.flag = Flag::NoConfig | Flag::AlwaysActive | Flag::ExInformation,
		.name = PLUGIN_NAME,

		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = PLUGIN_INFO,
	};
	return &filter;
}
