/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cmath>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "str_encodes.hpp"
#include "inifile_op.hpp"
#include "monitors.hpp"

#include "reactive_dlg.hpp"
#include "Easings.hpp"
#include "Easings_Tooltip.hpp"


#define NS_BEGIN(...) namespace __VA_ARGS__ {
#define NS_END }
NS_BEGIN()

////////////////////////////////
// 変化方法のツールチップ表示．
////////////////////////////////
using namespace reactive_dlg::Easings;
using namespace reactive_dlg::Easings::Tooltip;

static inline uintptr_t hook_uid() { return reinterpret_cast<uintptr_t>(&settings); }

static constinit HWND tooltip = nullptr;

// formatting a string that describes to the track mode.
static inline std::wstring format_easing(ExEdit::Object::TrackMode const& mode, int32_t param, easing_name_spec const& name_spec)
{
	// get name and specification.
	auto ret = sigma_lib::string::encode_sys::to_wide_str(name_spec.name);

	// integral parameter.
	if (name_spec.spec.param)
		ret += L"\nパラメタ: " + std::to_wstring(param);

	// two more booleans.
	if (bool const acc = (mode.num & mode.isAccelerate) != 0,
		dec = (mode.num & mode.isDecelerate) != 0;
		acc || dec) {
		ret += name_spec.spec.param ? L", " : L"\n";
		if (acc) ret += L"+加速 ";
		if (dec) ret += L"+減速 ";
		ret.pop_back();
	}

	return ret;
}

// multiplies the resolution of logical coordinates for screens with high DPI.
template<size_t scale_num, size_t scale_den = 1>
class rescale_dc {
	HDC dc;
	int mode;
	SIZE sz;

public:
	constexpr static size_t scale_num = scale_num, scale_den = scale_den;
	rescale_dc(HDC dc, std::same_as<int> auto&... coords)
		requires(sizeof...(coords) % 2 == 0) : dc{ dc }, mode{}, sz {}
	{
		constexpr auto num_coords = sizeof...(coords), num_points = num_coords / 2;
		int pts[] = { coords... };
		::LPtoDP(dc, reinterpret_cast<POINT*>(pts), num_points);

		mode = ::SetMapMode(dc, MM_ANISOTROPIC);
		::GetWindowExtEx(dc, &sz);
		::SetWindowExtEx(dc, sz.cx * scale_num / scale_den, sz.cy * scale_num / scale_den, nullptr);

		::DPtoLP(dc, reinterpret_cast<POINT*>(pts), num_points);
		[&]<size_t... I>(std::index_sequence<I...>) {
			((coords = pts[I]), ...);
		}(std::make_index_sequence<num_coords>{});
	}
	~rescale_dc()
	{
		// rewind the scale.
		::SetWindowExtEx(dc, sz.cx, sz.cy, nullptr);
		::SetMapMode(dc, mode);
	}
};

struct section_graph {
	std::vector<std::pair<float, float>> points;
	float min, max, val_left, val_right;

	void clear() { points.clear(); min = max = val_left = val_right = 0; }
	bool empty() const { return points.empty(); }

	inline void plot(ExEdit::Object const& obj, size_t index, int denom);
	inline void draw(HDC dc, int L, int T, int R, int B) const;

private:
	constexpr static int margin_lr = 3, margin_tb = 4;
};

// storage of the tooltip content.
#ifndef _DEBUG
constinit
#endif // !_DEBUG
static struct tooltip_content {
	int width, height;

	std::wstring easing, values;
	section_graph graph;
	int pos_y_values, pos_x_graph;

	inline void measure(size_t index, HDC hdc);
	inline void draw(HDC dc, RECT const& rc) const;

private:
	constexpr static int
		margin_r_easing = 8,
		margin_b_easing = 4, margin_b_graph = 4;
	constexpr static UINT draw_text_options = DT_NOCLIP | DT_HIDEPREFIX;

} content{};

// color conversion.
constexpr auto bgr2rgb(int32_t c) {
	return (std::rotl<uint32_t>(c, 16) & 0x00ff00ff) | (c & 0x0000ff00);
}

static inline struct graph_pen {
	constexpr operator bool() const { return pen != nullptr; }
	operator HPEN() {
		if (pen == nullptr) {
			LOGBRUSH lb{
				.lbStyle = BS_SOLID,
				.lbColor = bgr2rgb(settings.graph.curve_color),
			};
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
				settings.graph.curve_width, &lb, 0, nullptr);
		}
		return pen;
	}
	void delete_object() {
		if (pen != nullptr) {
			::DeleteObject(pen);
			pen = nullptr;
		}
	}
	~graph_pen() { delete_object(); }

private:
	HPEN pen = nullptr;
} graph_pen;


////////////////////////////////
// Tooltip drawings.
////////////////////////////////
inline void section_graph::plot(ExEdit::Object const& obj, size_t index, int denom)
{
	// prepare environment.
	auto const [filter_index, rel_idx] = find_filter_from_track(obj, index);
	auto const ofi = object_filter_index(&obj - *exedit.ObjectArray_ptr, filter_index);
	char* const arg_name = reinterpret_cast<char*>(1 + rel_idx); // represents the trackbar index.

	// select frames.
	size_t const num_sect = settings.graph.plots - 1;
	int const frame_begin = obj.frame_begin, frame_len = obj.frame_end + 1 - frame_begin;
	float const len_f = static_cast<float>(frame_len), denom_f = static_cast<float>(denom);
	std::vector<int> frames{}; frames.reserve(num_sect + 1); frames.push_back(frame_begin);
	for (size_t i = 0; i < num_sect; i++) {
		int const f = frame_begin + (frame_len * (i + 1) + (num_sect >> 1)) / num_sect;
		if (frames.back() < f) frames.push_back(f);
	}

	// store the left and right values.
	val_left = obj.track_value_left[index] / denom_f;
	val_right = obj.track_value_right[index] / denom_f;
	if (val_left < val_right) min = val_left, max = val_right;
	else min = val_right, max = val_left;

	// calculate each point.
	points.clear(); points.reserve(frames.size());
	for (auto f : frames) {
		int val; exedit.calc_trackbar(ofi, f, 0, &val, arg_name);

		// store the point and the statistics.
		float const val_f = val / denom_f;
		points.emplace_back((f - frame_begin) / len_f, val_f);
		if (val_f < min) min = val_f;
		else if (max < val_f) max = val_f;
	}
}

inline void section_graph::draw(HDC dc, int L, int T, int R, int B) const
{
	// re-scale the coordinate.
	rescale_dc<settings.graph.pixel_scale> rescale{ dc, L, T, R, B };

	// prepare coordinates.
	int const
		l = L + margin_lr * rescale.scale_num / rescale.scale_den,
		r = R - margin_lr * rescale.scale_num / rescale.scale_den,
		t = T + margin_tb * rescale.scale_num / rescale.scale_den,
		b = B - margin_tb * rescale.scale_num / rescale.scale_den;
	auto const draw_line = [dc](int x1, int y1, int x2, int y2) {
		POINT pts[] = { {x1, y1}, {x2, y2} };
		::Polyline(dc, pts, std::size(pts));
	};
	auto const line_h = [=](int y) { draw_line(L, y, R, y); };
	auto const line_v = [=](int x) { draw_line(x, T, x, B); };

	auto const func_x = [l, D = r - l](float x) -> int {
		return l + std::lroundf(D * x);
	};
	auto const func_y = [m = min, d = max - min, b, D = t - b](float y) -> int {
		return b + std::lroundf(D * (y - m) / d);
	};

	// draw grid lines.
	auto const old_pen = ::SelectObject(dc, ::GetStockObject(DC_PEN));
	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_3));
	if (min < max) {
		line_h(func_y(0.75f * val_left + 0.25f * val_right));
		line_h(func_y(0.50f * (val_left + val_right)));
		line_h(func_y(0.25f * val_left + 0.75f * val_right));
	}
	else {
		line_h((3 * t + b + 2) >> 2);
		line_h((t + b + 1) >> 1);
		line_h((t + 3 * b + 2) >> 2);
	}
	line_v((3 * l + r + 2) >> 2);
	line_v((l + r + 1) >> 1);
	line_v((l + 3 * r + 2) >> 2);

	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_2));
	if (min < max) {
		line_h(func_y(val_left));
		line_h(func_y(val_right));
	}
	else {
		line_h(t);
		line_h(b);
	}

	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_1));
	line_v(l);
	line_v(r);

	// draw the easing curve.
	::SelectObject(dc, graph_pen);
	if (min < max) {
		std::vector<POINT> pts{}; pts.reserve(points.size());
		for (auto const& [x, y] : points)
			pts.emplace_back(func_x(x), func_y(y));
		::Polyline(dc, pts.data(), std::size(pts));
	}
	else {
		auto const Y = (t + b + 1) >> 1;
		draw_line(l, Y, r, Y);
	}

	// set the pen back.
	::SelectObject(dc, old_pen);
}

inline void tooltip_content::measure(size_t index, HDC hdc)
{
	auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
	auto const& mode = obj.track_mode[index];
	auto const& track_info = exedit.trackinfo_left[index];
	easing_name_spec const name_spec{ mode };

	// the name and desc of the easing.
	if (settings.mode) easing = format_easing(mode, obj.track_param[index], name_spec);

	// values at each midpoint.
	if (settings.values.is_enabled()) {
		if (obj.index_midpt_leader < 0 || // no mid-points.
			name_spec.spec.twopoints)
			values = L"";
		else {
			values = formatted_values{ obj, index }.span()
				.trim_from_sect(settings.values.left, settings.values.right)
				.to_string(track_info.precision(), true, true, settings.values.zigzag);
		}
	}

	// easing graph.
	if (settings.graph.enabled)
		graph.plot(obj, index, track_info.denominator());

	// measure those text.
	RECT rc1{}, rc2{};
	if (!easing.empty())
		::DrawTextW(hdc, easing.c_str(), easing.size(),
			&rc1, DT_CALCRECT | draw_text_options);
	if (!values.empty())
		::DrawTextW(hdc, values.c_str(), values.size(),
			&rc2, DT_CALCRECT | draw_text_options | DT_SINGLELINE);

	// layout the contents and detemine the size.
	int const
		w_ease = rc1.right - rc1.left, h_ease = rc1.bottom - rc1.top,
		w_vals = rc2.right - rc2.left, h_vals = rc2.bottom - rc2.top;
	if (w_ease < w_vals) {
		if (!easing.empty() && !graph.empty()) {
			pos_x_graph = std::min(
				std::max(w_ease + margin_r_easing, w_vals - settings.graph.width),
				w_ease + (settings.graph.width >> 1)); // gap at most half width of the graph.
			width = std::max(w_vals, pos_x_graph + settings.graph.width);
		}
		else {
			pos_x_graph = 0;
			width = std::max(w_vals, graph.empty() ? 0 : settings.graph.width);
		}

		if (easing.empty() && graph.empty()) {
			pos_y_values = 0;
			height = h_vals;
		}
		else {
			pos_y_values = std::max(
				easing.empty() ? 0 : h_ease + margin_b_easing,
				graph.empty() ? 0 : settings.graph.height + margin_b_graph);
			height = pos_y_values + h_vals;
		}
	}
	else {
		if (easing.empty() && values.empty()) {
			pos_x_graph = 0;
			width = settings.graph.width;
		}
		else {
			pos_x_graph = std::max(
				easing.empty() ? 0 : w_ease + margin_r_easing,
				values.empty() ? 0 : w_vals + margin_r_easing);
			width = graph.empty() ? w_ease : pos_x_graph + settings.graph.width;
		}

		if (!easing.empty() && !values.empty()) {
			height = std::max(
				h_ease + margin_b_easing + h_vals,
				graph.empty() ? 0 : settings.graph.height + margin_b_graph);
			pos_y_values = height - margin_b_easing;
		}
		else {
			pos_y_values = 0;
			height = std::max(
				graph.empty() ? 0 : settings.graph.height + margin_b_graph,
				std::max(h_ease, h_vals));
		}
	}
}

inline void tooltip_content::draw(HDC dc, RECT const& rc) const
{
	// change the text color if specified.
	if (settings.text_color >= 0)
		::SetTextColor(dc, bgr2rgb(settings.text_color));

	// actual drawing, using content.easing and content.values.
	if (!easing.empty()) {
		RECT rc_easing = rc;
		::DrawTextW(dc, easing.c_str(), easing.size(),
			&rc_easing, draw_text_options);
	}
	if (!values.empty()) {
		RECT rc_vals = rc;
		rc_vals.top += pos_y_values;
		::DrawTextW(dc, values.c_str(), values.size(),
			&rc_vals, draw_text_options | DT_SINGLELINE);
	}

	if (!graph.empty()) {
		int const L = rc.left + pos_x_graph;
		graph.draw(dc, L, rc.top, L + settings.graph.width, rc.top + settings.graph.height);
	}
}


////////////////////////////////
// Hook callbacks.
////////////////////////////////
static inline LRESULT CALLBACK param_button_hook(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, auto id, auto data)
{
	if (*exedit.SettingDialogObjectIndex < 0) return ::DefSubclassProc(hwnd, message, wparam, lparam);

	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	{
		// relayed messages.
		MSG msg{ .hwnd = hwnd, .message = message, .wParam = wparam, .lParam = lparam };
		::SendMessageW(tooltip, TTM_RELAYEVENT,
			message == WM_MOUSEMOVE ? ::GetMessageExtraInfo() : 0, reinterpret_cast<LPARAM>(&msg));
		break;
	}

	case WM_NOTIFY:
		if (auto const hdr = reinterpret_cast<NMHDR*>(lparam); hdr->hwndFrom == tooltip) {
			switch (hdr->code) {
			case TTN_GETDISPINFOA:
			{
				// compose the tooltip string.
				size_t idx = static_cast<size_t>(data);
				auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
				auto const& mode = obj.track_mode[idx];
				if ((mode.num & 0x0f) == 0) break; // 移動無し
				if (settings.mode ||
					(settings.values.is_enabled() && !(
						obj.index_midpt_leader < 0 || // no mid-points.
						easing_name_spec(mode).spec.twopoints)))
					reinterpret_cast<NMTTDISPINFOA*>(lparam)->lpszText = const_cast<char*>(" ");
				break;
			}
			case TTN_SHOW:
			{
				// adjust the tooltip size to fit with the content.
				RECT rc;
				::GetWindowRect(tooltip, &rc);
				::SendMessageW(tooltip, TTM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rc));
				rc.right = rc.left + content.width + 2; // add slight extra space on the right and bottom.
				rc.bottom = rc.top + content.height + 1;
				::SendMessageW(tooltip, TTM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&rc));

				// adjust the position not to clip edges of screens.
				rc = sigma_lib::W32::monitor<true>{ rc.left, rc.top }
					.expand(-8).clamp(rc); // 8 pixels of padding.
				::SetWindowPos(tooltip, nullptr, rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return TRUE;
			}

			case NM_CUSTOMDRAW:
			{
				auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
				if (::IsWindowVisible(tooltip) == FALSE)
					// prepare the tooltip content.
					content.measure(static_cast<size_t>(data), dhdr->nmcd.hdc);
				else if (dhdr->nmcd.dwDrawStage != CDDS_PREPAINT)
					// draw the content.
					content.draw(dhdr->nmcd.hdc, dhdr->nmcd.rc);
				else return CDRF_NOTIFYPOSTPAINT;
				break;
			}
			}
		}
		break;
	}
	return ::DefSubclassProc(hwnd, message, wparam, lparam);
}
NS_END


////////////////////////////////
// Exported functions.
////////////////////////////////
namespace expt = reactive_dlg::Easings::Tooltip;

bool expt::setup(HWND hwnd, bool initializing)
{
	if (settings.is_enabled()) {
		if (initializing) {
			// create the tooltip window.
			tooltip = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP |
				(settings.animation ? TTS_NOFADE | TTS_NOANIMATE : 0),
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				hwnd, nullptr, exedit.fp->hinst_parent, nullptr);

			::SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			// associate the tooltip with each trackbar button.
			TTTOOLINFOW ti{
				.cbSize = TTTOOLINFOW_V1_SIZE,
				.uFlags = TTF_IDISHWND | TTF_TRANSPARENT,
				.hinst = exedit.fp->hinst_parent,
				.lpszText = LPSTR_TEXTCALLBACKW,
			};
			for (size_t i = 0; i < ExEdit::Object::MAX_TRACK; i++) {
				HWND tgt = exedit.hwnd_track_buttons[i];

				ti.hwnd = tgt;
				ti.uId = reinterpret_cast<uintptr_t>(tgt);

				::SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
				::SetWindowSubclass(tgt, &param_button_hook, hook_uid(), { i });
			}

			// settings of delay time for the tooltip.
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_INITIAL,	0xffff & settings.delay);
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP,	0xffff & settings.duration);
			::SendMessageW(tooltip, TTM_SETDELAYTIME, TTDT_RESHOW,	0xffff & (settings.delay / 5));
			// initial values are... TTDT_INITIAL: 340, TTDT_AUTOPOP: 3400, TTDT_RESHOW: 68.
		}
		else {
			::DestroyWindow(tooltip);
			tooltip = nullptr;
			graph_pen.delete_object();
		}
		return true;
	}
	return false;
}

void expt::Settings::load(char const* ini_file)
{
	using namespace sigma_lib::inifile;
	constexpr int min_color = 0x000000, max_color = 0xffffff;

#define read(func, hd, fld, ...)	hd fld = read_ini_##func(hd fld, ini_file, section, #fld __VA_OPT__(,) __VA_ARGS__)

	{
		constexpr auto section = "Easings.Tooltip";
		constexpr int min_vals = -1, max_vals = 100;
		constexpr int min_time = 0, max_time = 60'000;

		read(bool,,	mode);
		read(int,,	values.left,	min_vals, max_vals);
		read(int,,	values.right,	min_vals, max_vals);
		read(bool,,	values.zigzag);

		read(bool,,	animation);
		read(int,,	delay,		min_time, max_time);
		read(int,,	duration,	min_time, max_time);
		read(int,,	text_color, -1, max_color);
	}
	{
		constexpr auto section = "Easings.Tooltip.Graph";
		constexpr int min_size = 16, max_size = 512;

		read(bool, graph., enabled);

		read(int, graph., width,	min_size, max_size);
		read(int, graph., height,	min_size, max_size);

		read(int, graph., plots,		5, 1025);
		read(int, graph., curve_width,	1, 64 * graph.pixel_scale);

		read(int, graph., curve_color,	min_color, max_color);
		read(int, graph., line_color_1,	min_color, max_color);
		read(int, graph., line_color_2,	min_color, max_color);
		read(int, graph., line_color_3,	min_color, max_color);
	}

#undef read
}
