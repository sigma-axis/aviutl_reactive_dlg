/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <algorithm>
#include <bit>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "modkeys.hpp"
#include "str_encodes.hpp"


////////////////////////////////
// ini ファイル操作の補助関数．
////////////////////////////////
namespace sigma_lib::inifile
{
	inline auto read_ini_int(auto def, char const* ini, char const* section, char const* key)
	{
		return static_cast<decltype(def)>(
			::GetPrivateProfileIntA(section, key, static_cast<int32_t>(def), ini));
	}
	inline auto read_ini_int(auto def, char const* ini, char const* section, char const* key,
		int32_t min, int32_t max) {
		return static_cast<decltype(def)>(std::clamp(read_ini_int(static_cast<int32_t>(def), ini, section, key), min, max));
	}
	inline auto read_ini_bool(bool def, char const* ini, char const* section, char const* key)
	{
		return 0 != ::GetPrivateProfileIntA(section, key, def ? 1 : 0, ini);
	}
	inline auto read_ini_modkey(sigma_lib::modifier_keys::modkeys def, char const* ini, char const* section, char const* key)
	{
		char str[std::bit_ceil(std::size("ctrl + shift + alt ****"))];
		::GetPrivateProfileStringA(section, key, def.canon_name(), str, std::size(str), ini);
		return sigma_lib::modifier_keys::modkeys{ str, def };
	}
	inline std::wstring read_ini_string(char8_t const* def, char const* ini, char const* section, char const* key, size_t max_len)
	{
		std::string str(max_len, '\0');
		auto const len = ::GetPrivateProfileStringA(section, key, reinterpret_cast<char const*>(def), str.data(), str.length(), ini);
		str.erase(len);
		auto ret = sigma_lib::string::encode_utf8::to_wide_str(str);
		ret.shrink_to_fit();
		return ret;
	}
}
