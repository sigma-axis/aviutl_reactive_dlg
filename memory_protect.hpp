/*
The MIT License (MIT)

Copyright (c) 2024-2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <type_traits>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


////////////////////////////////
// 実行コードなどの書き換え補助．
////////////////////////////////
namespace sigma_lib::memory
{
	class ProtectHelper {
		uintptr_t const addr;
		size_t const sz;
		DWORD old_protect;

	public:
		template<class V> requires(!std::is_pointer_v<V>)
		ProtectHelper(V address, size_t size) : addr{ static_cast<uintptr_t>(address) }, sz{ size } {
			::VirtualProtect(as_ptr(), sz, PAGE_EXECUTE_READWRITE, &old_protect);
		}
		template<class T>
		ProtectHelper(T* address, size_t size = sizeof(T)) :
			ProtectHelper{ reinterpret_cast<uintptr_t>(address), size } {}
		~ProtectHelper() {
			::VirtualProtect(as_ptr(), sz, old_protect, &old_protect);
		}

		template<class T>
		T& as_ref(ptrdiff_t offset = 0) const {
			return *as_ptr<T>(offset);
		}
		template<class T = void>
		T* as_ptr(ptrdiff_t offset = 0) const {
			return reinterpret_cast<T*>(addr + offset);
		}
		size_t size() const { return sz; }

		template<class T, size_t N>
		void copy(T const(&src)[N], ptrdiff_t offset = 0) const {
			copy(&src[0], N, offset);
		}
		template<class T>
		void copy(T const* src, size_t count_elem, ptrdiff_t offset = 0) const {
			if constexpr (!std::is_void_v<T>) count_elem *= sizeof(T);
			std::memcpy(as_ptr(offset), src, count_elem);
		}

		static void write(auto address, auto data) {
			ProtectHelper(address, sizeof(data)).as_ref<decltype(data)>() = data;
		}
		template<class T, size_t N>
		static void copy(auto address, T const(&data)[N]) {
			ProtectHelper(address, sizeof(data)).copy(data);
		}
	};

	// ff 15 xx xx xx xx	// call	dword ptr ds:[xxxxxxxx]
	// v
	// e8 yy yy yy yy		// call	yyyyyyyy
	// 90					// nop
	inline void hook_api_call(auto* address, auto* hook_func)
	{
		auto const
			addr = reinterpret_cast<uintptr_t>(address),
			proc = reinterpret_cast<uintptr_t>(hook_func);

		uint8_t code[6];
		code[0] = 0xe8; // call
		*reinterpret_cast<uintptr_t*>(&code[1]) = proc - (addr + 5);
		code[5] = 0x90; // nop

		ProtectHelper::copy(addr, code);
	}
}

