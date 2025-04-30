/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <bit>
#include <cmath>
#include <tuple>
#include <string>
#include <set>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "reactive_dlg.hpp"
#include "Filters_ContextMenu.hpp"
#include "inifile_op.hpp"
#include "clipboard.hpp"

using namespace sigma_lib::string;


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// フィルタ効果のメニュー拡張．
////////////////////////////////
using namespace reactive_dlg::Filters::ContextMenu;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static constinit struct {
	constexpr static uint32_t
		filter_name_item = 1110, // the grayed-out menu item that shows the filter name.
		base_id = 13010;
	uint32_t copy_as_lua = 0;

	// make sure the id isn't already used in the menu.
	static void collect_claimed_ids(std::set<uint32_t>& ids, HMENU hmenu)
	{
		std::set<HMENU> track{};
		[&](this auto&& self, HMENU m) -> void {
			if (!track.insert(m).second) return; // cyclic detected.

			for (int i = ::GetMenuItemCount(m); --i >= 0; ) {
				MENUITEMINFOW mii{
					.cbSize = sizeof(mii),
					.fMask = MIIM_SUBMENU | MIIM_FTYPE | MIIM_ID,
				};
				if (::GetMenuItemInfoW(hmenu, i, TRUE, &mii) == FALSE ||
					(mii.fType & MFT_SEPARATOR) != 0) continue;
				else if (mii.hSubMenu != nullptr) self(mii.hSubMenu);
				else ids.insert(mii.wID);
			}
		} (hmenu);
	}
	static uint32_t register_new_id(std::set<uint32_t>& claimed_ids)
	{
		uint32_t id = base_id;
		for (auto i = claimed_ids.lower_bound(id), e = claimed_ids.end();
			i != e && *i == id; i++, id++);
		claimed_ids.insert(id);
		return id;
	}
} menu_id{};

namespace menu_handles
{
	HMENU visual_obj = nullptr;
}

namespace lua_code
{
	// returns empty when escaping is not necessary.
	static inline std::string escape_string(std::string_view const& str)
	{
		constexpr auto should_escape = [](char c) {
			return ('\0' <= c && c <= '\x1f') || c == '\\' || c == '"';
		};

		for (auto& c : str) {
			if (should_escape(c)) goto do_escape;
		}
		return {};

	do_escape:
		constexpr auto append_escaped = [](std::string& s, char c) {
			s.append(1, '\\');
			switch (c) {
			default:
			{
				char buf[4];
				s.append(buf, ::sprintf_s(buf, "%03d", static_cast<byte>(c)));
				return;
			}
			case '\a': c = 'a'; break;
			case '\b': c = 'b'; break;
			case '\f': c = 'f'; break;
			case '\n': c = 'n'; break;
			case '\r': c = 'r'; break;
			case '\t': c = 't'; break;
			case '\v': c = 'v'; break;
			case '\\': c = '\\'; break;
			case '\"': c = '\"'; break;
			}
			s.append(1, c);
		};

		std::string ret{}; ret.reserve(4 * str.size());
		for (auto p = str.begin(), e = str.end(); p != e; p++) {
			if (::IsDBCSLeadByte(*p) != FALSE) {
				if (auto q = p + 1; q == e)
					// illegal character here. let it escape.
					append_escaped(ret, *p);
				else {
					if (should_escape(*q)) {
						// turn the both to escape sequences.
						append_escaped(ret, *p);
						append_escaped(ret, *q);
					}
					else ret.append(&*p, 2); // no need to escape.
					p = q;
				}
			}
			else if (should_escape(*p)) append_escaped(ret, *p);
			else ret.append(1, *p);
		}
		return ret;
	}
}

struct lua_codepiece {
	std::string s{};

	auto& put(int32_t n, int32_t d = 1)
	{
		int const prec = std::lroundf(std::log10f(static_cast<float>(d)));
		char buf[std::bit_ceil(TrackInfo::max_value_len + 1)];
		return append(buf, ::sprintf_s(buf, "%.*f", prec, static_cast<double>(n) / d));
	}

	auto& put(byte const* data, size_t len)
	{
		s.resize_and_overwrite(s.size() + 2 * len + 2, [](char*, size_t sz) { return sz; });
		auto dst = s.end() - (2 * len + 2);

		if (len <= 4) { *(dst++) = '0'; *(dst++) = 'x'; } // by number, add prefix.
		else { *(dst++) = '"'; dst[2 * len] = '"'; } // by string, surround by double quotes.

		// write binary content.
		constexpr auto hex = [](byte b) -> char {
			if (b < 10) return '0' + b;
			else return ('a' - 10) + b;
		};
		for (auto p = data, e = data + len; p < e; p++) {
			*(dst++) = hex((*p) >> 4);
			*(dst++) = hex((*p) & 0x0f);
		}

		return *this;
	}

	auto& put(std::string_view str)
	{
		auto const str2 = lua_code::escape_string(str);
		if (!str2.empty()) str = str2;

		s.reserve(s.size() + str.size() + 2);
		return append("\"").append(str).append("\"");
	}

	constexpr auto& comma() { return append(","); }
	constexpr auto& append(auto... args) { s.append(args...); return *this; }
	constexpr operator std::string() const { return s; }
};

static inline bool can_obj_effect(ExEdit::Filter::Flag flags)
{
	constexpr auto flags_rejected =
		ExEdit::Filter::Flag::Audio |
		ExEdit::Filter::Flag::Input |
		ExEdit::Filter::Flag::Output |
		ExEdit::Filter::Flag::Control,

		flags_required =
		ExEdit::Filter::Flag::Effect;

	return (flags & (flags_rejected | flags_required)) == flags_required;
}

static inline std::string compose_obj_effect(ExEdit::Object const& obj, ExEdit::Object const& leader, ExEdit::Object::FilterParam const& param)
{
	auto const& filter = *exedit.loaded_filter_table[param.id];
	if (!can_obj_effect(filter.flag)) return {};

	lua_codepiece ret{ "obj.effect(" };
	ret.put(filter.name);

	// tracks
	for (int32_t i = 0; i < filter.track_n; i++) {
		ret.comma().put(filter.track_name[i])
			.comma().put(obj.track_value_left[param.track_begin + i],
				filter.track_scale == nullptr ? 1 : std::max(filter.track_scale[i], 1));
	}

	// checks
	for (int32_t i = 0; i < filter.check_n; i++) {
		if (filter.check_default[i] >= 0) // exclude button and combo box.
			ret.comma().put(filter.check_name[i])
				.comma().put(leader.check_value[param.check_begin + i]);
	}

	// exdata
	if (filter.exdata_size > 0) {
		byte const* data = find_exdata<byte>(leader.exdata_offset, param.exdata_offset);
		auto const e = data + filter.exdata_size;
		for (auto const* use = filter.exdata_use;
			data < e && data + use->size <= e; data += use->size, use++) {
			auto const type = use->type;

			// ignore types other than number, binary, or string.
			switch (type) {
			case ExEdit::ExdataUse::Type::Padding:
			default: continue;
			case ExEdit::ExdataUse::Type::Number:
			case ExEdit::ExdataUse::Type::Binary:
			case ExEdit::ExdataUse::Type::String:
				break;
			}

			// write the name.
			ret.comma().put(use->name).comma();

			// write data according to the specified type.
			switch (type) {
			case ExEdit::ExdataUse::Type::Number:
			{
				int32_t n = 0;
				std::memcpy(&n, data, std::min<size_t>(use->size, sizeof(n)));
				ret.put(n);
				break;
			}
			case ExEdit::ExdataUse::Type::Binary:
				ret.put(data, use->size);
				break;
			case ExEdit::ExdataUse::Type::String:
			{
				std::string_view str{
					reinterpret_cast<char const*>(data),
					static_cast<size_t>(use->size)
				};
				if (auto const pos = str.find_first_of('\0'); pos != str.npos)
					str = str.substr(0, pos);
				ret.put(str);
				break;
			}
			default: std::unreachable();
			}
		}
	}

	return ret.append(")");
}

// identifies the object and the filter targeted for the manipulation.
// if not found as valid, returns a tuple of nullptrs.
// entries are: the target object, leader object, and filter param.
static inline std::tuple<ExEdit::Object const*, ExEdit::Object const*, ExEdit::Object::FilterParam const*> find_target_filter()
{
	int const obj_idx = *exedit.SettingDialogObjectIndex;
	if (obj_idx < 0) return { nullptr, nullptr, nullptr };
	int const filter_idx = *exedit.current_filter_index;
	if (filter_idx < 0) return { nullptr, nullptr, nullptr };

	auto const objects = *exedit.ObjectArray_ptr;
	auto const& obj = objects[obj_idx];
	if (filter_idx >= obj.countFilters()) return { nullptr, nullptr, nullptr };

	auto const& leader = obj.index_midpt_leader < 0 ? obj : objects[obj.index_midpt_leader];
	return { &obj, &leader, &leader.filter_param[filter_idx] };
}

static inline bool copy_as_lua()
{
	// check if the situation is valid.
	auto const [o_ptr, l_ptr, f_ptr] = find_target_filter();
	if (o_ptr == nullptr) return false;

	// try to compose the string.
	auto str = compose_obj_effect(*o_ptr, *l_ptr, *f_ptr);
	if (str.empty()) return false; // not a suitable filter for copying.

	// convert to CRLF line breaks, as specified in Win32 API docs.
	for (size_t pos = 0; pos = str.find('\n', pos), pos != str.npos; pos++) {
		if (pos > 0 && str[pos - 1] == '\r') continue;
		str.insert(pos, 1, '\r'); pos++;
	}
	str += "\r\n";

	// send to the clipboard.
	sigma_lib::W32::clipboard::write(
		sigma_lib::string::encode_sys::to_wide_str(str));

	return true;
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK setting_dlg_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto)
{
	switch (message) {
	case WM_INITMENUPOPUP:
	{
		if (reinterpret_cast<HMENU>(wparam) != menu_handles::visual_obj || lparam != 0) break;

		// prepare states of the menu items.
		MENUITEMINFOW mii{
			.cbSize = sizeof(mii),
			.fMask = MIIM_STATE,
		};
		if (auto const [o_ptr, l_ptr, f_ptr] = find_target_filter();
			o_ptr != nullptr &&
			can_obj_effect(exedit.loaded_filter_table[f_ptr->id]->flag))
			mii.fState = MFS_ENABLED;
		else mii.fState = MFS_DISABLED;
		::SetMenuItemInfoW(menu_handles::visual_obj, menu_id.copy_as_lua, FALSE, &mii);
		break;
	}
	case WM_COMMAND:
	{
		if ((wparam >> 16) != 0) break;

		// handle commands.
		if (wparam == menu_id.copy_as_lua) {
			copy_as_lua();
			return 0;
		}

		break;
	}
	case WM_DESTROY:
		::RemoveWindowSubclass(hwnd, &setting_dlg_hook, id);
		break;
	}

	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}

NS_END


////////////////////////////////
// exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Filters::ContextMenu;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (initializing && settings.is_enabled()) {
		menu_handles::visual_obj = ::GetSubMenu(*exedit.hmenu_cxt_visual_obj, 0);

		// find the position to insert menu items.
		size_t insert_pos = ::GetMenuItemCount(menu_handles::visual_obj);
		while (insert_pos > 0) {
			if (::GetMenuItemID(menu_handles::visual_obj, --insert_pos)
				== menu_id.filter_name_item) break;
		}

		// prepare to find suitable menu ids.
		std::set<uint32_t> claimed_ids{};
		menu_id.collect_claimed_ids(claimed_ids, menu_handles::visual_obj);

		// add a separator before adding command items.
		MENUITEMINFOW mii{
			.cbSize = sizeof(mii),
			.fMask = MIIM_FTYPE,
			.fType = MFT_SEPARATOR,
		};
		::InsertMenuItemW(menu_handles::visual_obj, ++insert_pos, TRUE, &mii);
		mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
		mii.fType = MFT_STRING;

		// the command to copy as a lua codepiece.
		if (settings.copy_as_lua) {
			// find a suitable id.
			menu_id.copy_as_lua = menu_id.register_new_id(claimed_ids);

			// add a command item.
			mii.wID = menu_id.copy_as_lua;
			mii.dwTypeData = const_cast<wchar_t*>(L"Luaスクリプトとしてコピー");
			::InsertMenuItemW(menu_handles::visual_obj, ++insert_pos, TRUE, &mii);
		}

		// hook the setting dialog to handle the menu commands.
		::SetWindowSubclass(*exedit.hwnd_setting_dlg, &setting_dlg_hook, hook_uid(), {});
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;

	constexpr auto section = "Filters.ContextMenu";

#define read(func, fld, ...)	fld = read_ini_##func(fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	read(bool,	copy_as_lua);

#undef read
}
