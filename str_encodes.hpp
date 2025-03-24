/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

////////////////////////////////
// 文字エンコード変換．
////////////////////////////////
namespace sigma_lib::string
{
	template<uint32_t codepage>
	struct Encode {
		constexpr static uint32_t CodePage = codepage;

		// conversion between wide character string.
		static int cnt_wide_str(char const* str, int cnt_str = -1) {
			return to_wide_str(nullptr, 0, str, cnt_str);
		}
		static int to_wide_str(wchar_t* wstr, int cnt_wstr, char const* str, int cnt_str = -1) {
			return ::MultiByteToWideChar(CodePage, 0, str, cnt_str, wstr, cnt_wstr);
		}
		template<size_t cnt_wstr>
		static int to_wide_str(wchar_t(&wstr)[cnt_wstr], char const* str, int cnt_str = -1) {
			return to_wide_str(wstr, int{ cnt_wstr }, str, cnt_str);
		}
		static std::wstring to_wide_str(char const* str, int cnt_str = -1) {
			size_t cntw = cnt_wide_str(str, cnt_str);
			if (cntw == 0) return L"";
			if (cnt_str >= 0 && str[cnt_str - 1] != '\0') cntw++;
			std::wstring ret(cntw - 1, L'\0');
			to_wide_str(ret.data(), cntw, str, cnt_str);
			return ret;
		}
		static std::wstring to_wide_str(std::string const& str) { return to_wide_str(str.c_str(), str.length()); }
		static std::wstring to_wide_str(std::string_view const& str) { return to_wide_str(str.data(), str.length()); }

		static int cnt_narrow_str(wchar_t const* wstr, int cnt_wstr = -1) {
			return from_wide_str(nullptr, 0, wstr, cnt_wstr);
		}
		static int from_wide_str(char* str, int cnt_str, wchar_t const* wstr, int cnt_wstr = -1) {
			return ::WideCharToMultiByte(CodePage, 0, wstr, cnt_wstr, str, cnt_str, nullptr, nullptr);
		}
		template<size_t cnt_str>
		static int from_wide_str(char(&str)[cnt_str], wchar_t const* wstr, int cnt_wstr = -1) {
			return from_wide_str(str, int{ cnt_str }, wstr, cnt_wstr);
		}
		static std::string from_wide_str(wchar_t const* wstr, int cnt_wstr = -1) {
			size_t cnt = cnt_narrow_str(wstr, cnt_wstr);
			if (cnt == 0) return "";
			if (cnt_wstr >= 0 && wstr[cnt_wstr - 1] != L'\0') cnt++;
			std::string ret(cnt - 1, '\0');
			from_wide_str(ret.data(), cnt, wstr, cnt_wstr);
			return ret;
		}
		static std::string from_wide_str(std::wstring const& wstr) { return from_wide_str(wstr.c_str(), wstr.length()); }
		static std::string from_wide_str(std::wstring_view const& wstr) { return from_wide_str(wstr.data(), wstr.length()); }
	};
	using encode_sjis = Encode<932>;
	using encode_utf8 = Encode<CP_UTF8>;
	using encode_sys = Encode<CP_ACP>;
}
