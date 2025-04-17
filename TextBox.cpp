/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

#include "reactive_dlg.hpp"
#include "TextBox.hpp"
#include "inifile_op.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// テキストボックス検索/操作．
////////////////////////////////
using namespace reactive_dlg::TextBox;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }
static inline bool check_edit_box_style(HWND edit)
{
	// check whether ...
	// - it allows multiple lines,
	// - it accepts return,
	// - it's enabled, and
	// - it's visible.
	constexpr uint32_t
		required = ES_MULTILINE | ES_WANTRETURN | WS_VISIBLE,
		rejected = WS_DISABLED;
	auto style = ::GetWindowLongW(edit, GWL_STYLE);
	return (style & (required | rejected)) == required;
}

enum class kind : uint8_t { unspecified, text, script };
static inline kind edit_box_kind(HWND hwnd) {
	if (hwnd == nullptr) return kind::unspecified; // in case the following two are not created.
	if (hwnd == *exedit.hwnd_edit_text) return kind::text;
	if (hwnd == *exedit.hwnd_edit_script) return kind::script;
	return kind::unspecified;
}

// テキスト入力の画面反映処理を一括する機能．
static constinit struct {
	void operator()()
	{
		// ignore unexpected calls.
		if (!posted) return;
		posted = false;

		calling = true;
		// calling exedit.setting_dlg_wndproc() would be faster,
		// but might ignore hooks from other plugins.
		::SendMessageW(hwnd, message, wparam, lparam);
		calling = false;
	}
	bool is_calling() const { return calling; }
	void post(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, HWND hwnd_host)
	{
		// update message parameters.
		this->hwnd = hwnd;
		this->message = message;
		this->wparam = wparam;
		this->lparam = lparam;

		// post a message if not yet.
		if (posted) return;
		posted = true;

		constexpr PrivateMsg::callback_fnptr callback = [](auto...) { batch(); };
		::PostMessageW(hwnd_host, PrivateMsg::RequestCallback,
			{}, reinterpret_cast<LPARAM>(callback));
	}
	void discard(HWND hwnd_host)
	{
		if (!posted) return;
		posted = false;
		discard_message(hwnd_host, PrivateMsg::RequestCallback);
	}

private:
	// indicates if delaying call was already posted.
	bool posted = false;

	// indicates if it's a delayed batch call.
	bool calling = false;

	// message paramters.
	HWND hwnd{};
	UINT message{};
	WPARAM wparam{};
	LPARAM lparam{};
} batch;


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK text_box_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		// 指定キーでフォーカスをテキストボックスから移動する機能．
		if (settings.focus_forward.is_enabled() || settings.focus_backward.is_enabled()) {
			// see if the combination including modifier keys matches.
			if (key_pressed_any(VK_LWIN, VK_RWIN)) break;

			byte const vkey = static_cast<byte>(wparam);
			bool forward;
			if (auto const curr_keys = curr_modkeys();
				settings.focus_forward.match(vkey, curr_keys)) forward = true;
			else if (settings.focus_backward.match(vkey, curr_keys)) forward = false;
			else break;

			// find the "next" target for the focus.
			auto target = next_edit_box(hwnd, forward);
			if (target == nullptr) target = exedit.fp->hwnd;

			// move the focus. returns non-null if the focus moved successfully.
			if (::SetFocus(target) == nullptr) break;

			// prevent the character that is pressed now to be written to the edit box,
			// by discarding input messages sent to this edit box.
			// (for TAB key, takes effect only when ctrl is not pressed.)
			// for WM_SYSKEYDOWN, it prevents notification sound from playing.
			constexpr int diff = WM_CHAR - WM_KEYDOWN;
			static_assert(diff == WM_SYSCHAR - WM_SYSKEYDOWN);
			discard_message(hwnd, message + diff);

			// shouldn't pass to ::DefSubclassProc(). (for TAB key, takes effect only when ctrl is pressed.)
			return 0;
		}
		break;

	case WM_CHAR:
		// TAB 文字を別の文字列で置き換える機能．
		if (static_cast<wchar_t>(wparam) == L'\t') {
			std::wstring* wstr = nullptr;
			switch (edit_box_kind(hwnd)) {
			case kind::text: wstr = settings.replace_tab_text.get(); break;
			case kind::script: wstr = settings.replace_tab_script.get(); break;
			}
			if (wstr == nullptr) break;

			if (wstr->length() != 1) {
				::DefSubclassProc(hwnd, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(wstr->c_str()));
				return 0;
			}
			// for a single character, just replace a certain parameter,
			// which works a bit better with undo-ing feature.
			wparam = static_cast<WPARAM>((*wstr)[0]);
		}
		break;

	case WM_KILLFOCUS:
		::RemoveWindowSubclass(hwnd, &text_box_hook, id);
		break;
	}

	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

static inline LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	switch (message) {
	case WM_COMMAND:
		if (auto ctrl = reinterpret_cast<HWND>(lparam); ctrl != nullptr) {
			switch (wparam >> 16) {
			case EN_SETFOCUS:
				if ((settings.focus_forward.is_enabled() || settings.focus_backward.is_enabled() ||
					settings.replace_tab_text || settings.replace_tab_script) &&
					edit_box_kind(ctrl) != kind::unspecified && check_edit_box_style(ctrl))
					// hook for shortcut keys to move the focus out of this edit box.
					::SetWindowSubclass(ctrl, &text_box_hook, hook_uid(), 0);
				break;

			case EN_CHANGE:
				if (settings.batch &&
					!batch.is_calling() && edit_box_kind(ctrl) != kind::unspecified &&
					// 未確定文字がある状態でフォーカスが移動した場合 WM_KILLFOCUS -> EN_CHANGE の順になるので，その場面に対応するためコメントアウト．
					//::GetFocus() == ctrl &&
					check_edit_box_style(ctrl)) {

					//if (*exedit.is_playing == 0) {
					//	// 通常の編集中のユーザの入力と判断，
					//	// メッセージの転送を保留して，次のメッセージループで再送するよう仕掛ける．
					//	batch.post(hwnd, message, wparam, lparam, reinterpret_cast<HWND>(data));
					//	return 0;
					//}
					//else
					//	// プレビュー再生中のユーザの入力と判断，
					//	// 遅延させずに即座に効果を持たせる（再生が停止する）．遅延待機中の更新通知があるなら破棄．
					//	batch.discard(reinterpret_cast<HWND>(data));
					// NOTE: 再生の停止は一連の入力メッセージが処理された後で起こるため，これは悪手だった．

					// メッセージの転送を保留して，次のメッセージループで再送するよう仕掛ける．
					batch.post(hwnd, message, wparam, lparam, reinterpret_cast<HWND>(data));
					return 0;
				}
				break;
			}
		}
		break;

	case WM_PARENTNOTIFY:
		if ((settings.tabstops_text != -1 || settings.tabstops_script != -1) &&
			(wparam & 0xffff) == WM_CREATE) {
			// at this moment, we can't tell this is an edit control for text objects or script effects
			// as their handles aren't assinged yet to exedit.hwnd_edit_[text/script].
			// so put a delay by posting a message to the private window.
			constexpr PrivateMsg::callback_fnptr callback = [](HWND, WPARAM wparam) {
				HWND const edit = reinterpret_cast<HWND>(wparam);
				int tabstops = -1;
				switch (edit_box_kind(edit)) {
				case kind::text: tabstops = settings.tabstops_text; break;
				case kind::script: tabstops = settings.tabstops_script; break;
				}
				if (tabstops >= 0)
					::SendMessageW(edit, EM_SETTABSTOPS, { 1 }, reinterpret_cast<LPARAM>(&tabstops));
			};

			::PostMessageW(reinterpret_cast<HWND>(data), PrivateMsg::RequestCallback,
				static_cast<WPARAM>(lparam), reinterpret_cast<LPARAM>(callback));
		}
		break;

	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &setting_dlg_hook, id);
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::TextBox;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.focus_forward.is_enabled() || settings.focus_backward.is_enabled() ||
		settings.tabstops_text >= 0 || settings.tabstops_script >= 0 ||
		settings.replace_tab_text || settings.replace_tab_script ||
		settings.batch) {
		auto const dlg = *exedit.hwnd_setting_dlg;
		if (initializing) {
			::SetWindowSubclass(dlg, &setting_dlg_hook, hook_uid(), reinterpret_cast<DWORD_PTR>(hwnd));
		}
		else {
			::RemoveWindowSubclass(dlg, &setting_dlg_hook, hook_uid());
			batch.discard(hwnd);
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "TextBox";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(int,	focus_forward.vkey);
	read(modkey,focus_forward.mkeys);
	read(int,	focus_backward.vkey);
	read(modkey,focus_backward.mkeys);

	read(bool,	batch);

	constexpr int16_t min_tabstops = -1, max_tabstops = 256;
	read(int,	tabstops_text, min_tabstops, max_tabstops);
	read(int,	tabstops_script, min_tabstops, max_tabstops);

#undef read

	constexpr size_t max_replace_tab_length = 255;
	constexpr auto def_tab = L"\t";
	constexpr auto def_tab_u8 = u8"\t";
	auto s = read_ini_string(def_tab_u8, ini_file, section, "replace_tab_text", max_replace_tab_length);
	if (s != def_tab) replace_tab_text = std::make_unique<std::wstring>(std::move(s));
	s = read_ini_string(def_tab_u8, ini_file, section, "replace_tab_script", max_replace_tab_length);
	if (s != def_tab) replace_tab_script = std::make_unique<std::wstring>(std::move(s));
}

HWND expt::next_edit_box(HWND curr, bool forward)
{
	HWND edits[]{ *exedit.hwnd_edit_text, *exedit.hwnd_edit_script };
	if (!forward) std::reverse(std::begin(edits), std::end(edits));

	auto next = std::end(edits);
	while (--next >= std::begin(edits) && !(*next != nullptr && *next == curr));
	while (++next < std::end(edits)) {
		if (*next != nullptr && check_edit_box_style(*next)) return *next;
	}
	return nullptr;
}
