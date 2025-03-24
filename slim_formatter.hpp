/*
The MIT License (MIT)

Copyright (c) 2024-2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <vector>
#include <string>

////////////////////////////////
// std::format() の簡易版．
////////////////////////////////
namespace sigma_lib::string
{
	class slim_formatter {
		std::wstring base{};
		std::vector<size_t> holes{}; // the 'holes' where the parameter is placed in.

	public:
		constexpr std::wstring operator()(std::wstring const& param) const
		{
			std::wstring ret(base.size() + param.size() * holes.size(), L'\0');

			// alternatingly copy to `ret` the strings `base` and `param`.
			size_t l = 0; auto p = ret.data();
			for (auto i : holes) {
				base.copy(p, i - l, l);
				p += i - l;

				param.copy(p, param.size());
				p += param.size();

				l = i;
			}

			// and the final tail part.
			base.copy(p, base.size() - l, l);

			return ret;
		}
		constexpr slim_formatter(std::wstring const& fmt)
		{
			base = fmt;

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

			base.shrink_to_fit();
			holes.shrink_to_fit();
		}
	};
}
