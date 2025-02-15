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
// Windows API 利用の補助関数．
////////////////////////////////
namespace sigma_lib::W32
{
	struct clipboard {
		static bool read(std::wstring& dst)
		{
			// see if the text format is available.
			if (::IsClipboardFormatAvailable(CF_UNICODETEXT) == FALSE ||
				::OpenClipboard(nullptr) == FALSE) return false;

			bool success = false;

			// retrieve the data from clipboard.
			if (auto h = ::GetClipboardData(CF_UNICODETEXT); h != nullptr) {
				if (auto ptr = reinterpret_cast<wchar_t const*>(::GlobalLock(h)); ptr != nullptr) {
					// copy the data from the global memory.
					dst = ptr;
					::GlobalUnlock(h);

					success = true;
				}
			}

			// finalizing.
			::CloseClipboard();

			return success;
		}
		static void write(std::wstring_view const& src)
		{
			// initializing.
			if (::OpenClipboard(nullptr) == FALSE) return;
			::EmptyClipboard();

			// allocate global memory to store the string.
			if (auto h = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (src.size() + 1)); h != nullptr) {
				if (auto ptr = reinterpret_cast<wchar_t*>(::GlobalLock(h)); ptr != nullptr) {
					// copy the string to the global memory.
					src.copy(ptr, src.size()); ptr[src.size()] = L'\0';
					::GlobalUnlock(h);

					// send to the clipboard.
					::SetClipboardData(CF_UNICODETEXT, h);
				}
				else ::GlobalFree(h);
			}

			// finalizing.
			::CloseClipboard();
		}
	};
}
