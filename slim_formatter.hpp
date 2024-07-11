/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <vector>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


////////////////////////////////
// 文字エンコード変換．
////////////////////////////////
struct Encodes {
	template<UINT codepage = CP_UTF8>
	static std::wstring to_wstring(const std::string_view& src) {
		if (src.length() == 0) return L"";

		auto wlen = ::MultiByteToWideChar(codepage, 0, src.data(), src.length(), nullptr, 0);
		std::wstring ret(wlen, L'\0');
		::MultiByteToWideChar(codepage, 0, src.data(), src.length(), ret.data(), wlen);

		return ret;
	}
};


////////////////////////////////
// std::format() の簡易版．
////////////////////////////////
class slim_formatter {
	std::wstring base{};
	std::vector<size_t> holes{}; // the 'holes' where the animation name is placed in.

public:
	constexpr std::wstring operator()(std::wstring const& name) const
	{
		std::wstring ret = base; // copy.
		ret.reserve(ret.size() + name.size() * holes.size());

		// insert the name into the 'holes'.
		for (auto i : holes) ret.insert(i, name);
		ret.shrink_to_fit();
		return ret;
	}
	constexpr std::wstring operator()(char const* name) const { return (*this)(Encodes::to_wstring<CP_ACP>(name)); }
	constexpr void init(std::wstring const& fmt)
	{
		base = fmt;
		holes.clear();

		// find the patern "{+}?" (e.g. "{}", "{{{}", "{{{{{"),
		// and modify it for future use.
		size_t i = 0;
		while ((i = base.find_first_of(L'{', i)) < base.size()) {
			size_t j = base.find_first_not_of(L'{', i);
			bool has_hole = false;

			if (j == base.npos) j = base.size() + 1; // tail.
			else if (base[j] == L'}' && (j - i) % 2 != 0) {
				j++;
				has_hole = true;
			}

			// double beginning braces "{{" turns into a single brace "{",
			// whereas a paired bracket "{}" turns into a 'hole'.
			auto len = j - i, cnt = (len - 1) / 2 + (has_hole ? 0 : 1);
			base.replace(i, len, cnt, L'{');
			i += cnt;

			// add the recognized hole.
			if (has_hole) holes.emplace_back(i);
		}

		std::reverse(holes.begin(), holes.end()); // holes are filled in the reversed order.
		base.shrink_to_fit();
		holes.shrink_to_fit();
	}
};
