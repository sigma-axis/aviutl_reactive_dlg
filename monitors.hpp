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
#include <tuple>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

////////////////////////////////
// Windows API 利用の補助関数．
////////////////////////////////
namespace sigma_lib::W32
{
	struct monitor {
		// work area: the portion of the screen excluding the task bar area (and maybe others).
		RECT bd_full, bd_work;
		bool is_primary;

		monitor(int x, int y) : monitor(POINT{ x, y }) {}
		explicit monitor(POINT const& point) : monitor(::MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST)) {}
		monitor(int left, int top, int right, int bottom) : monitor(RECT{ left, top, right, bottom }) {}
		explicit monitor(RECT const& rect) : monitor(::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST)) {}
		explicit monitor(HWND hwnd) : monitor(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)) {}

		monitor(HMONITOR monitor) {
			MONITORINFO mi{ .cbSize = sizeof(mi) };
			::GetMonitorInfoW(monitor, &mi);
			bd_full = mi.rcMonitor;
			bd_work = mi.rcWork;
			is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
		}

		template<bool work>
		constexpr auto clamp(int left, int top, int right, int bottom) const
		{
			if constexpr (work) return clamp(left, top, right, bottom, bd_work);
			else return clamp(left, top, right, bottom, bd_full);
		}
		template<bool work>
		constexpr RECT clamp(RECT const& rect) const {
			auto [l, t, r, b] = clamp<work>(rect.left, rect.top, rect.right, rect.bottom);
			return { l, t, r, b };
		}
		template<bool work>
		constexpr auto clamp(int x, int y) const
		{
			if constexpr (work) return clamp(x, y, bd_work);
			else return clamp(x, y, bd_full);
		}
		template<bool work>
		constexpr POINT clamp(POINT const& point) const {
			auto [x, y] = clamp<work>(point.x, point.y);
			return { x,y };
		}

	private:
		constexpr static auto clamp(int x, int y, RECT const& rc)
		{
			return std::pair{
				std::clamp<int>(x, rc.left, rc.right),
				std::clamp<int>(y, rc.top, rc.bottom)
			};
		}
		constexpr static auto clamp(int left, int top, int right, int bottom, RECT const& rc)
		{
			auto [l, r] = clamp(left, right, rc.left, rc.right);
			auto [t, b] = clamp(top, bottom, rc.top, rc.bottom);
			return std::tuple{ l, t, r, b };
		}
		constexpr static std::pair<int, int> clamp(int lbd, int ubd, int min, int max)
		{
			if (ubd - lbd > max - min)
				return { lbd, ubd }; // leave unchanged.
			else if (lbd < min)
				return { min, ubd + min - lbd };
			else if (ubd > max)
				return { lbd + max - ubd, max };
			else return { lbd, ubd };
		}
	};
}