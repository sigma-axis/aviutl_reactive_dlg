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
#include <bit>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <exedit.hpp>

#include "str_encodes.hpp"
#include "inifile_op.hpp"

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
static inline std::wstring format_easing(ExEdit::Object::TrackMode mode, int32_t param, easing_name_spec const& name_spec)
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
		::ScaleWindowExtEx(dc, scale_num, scale_den, scale_num, scale_den, &sz);

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
	float val_left, val_right, curr;

	void clear() { points.clear(); val_left = val_right = 0; curr = -1; }
	bool empty() const { return points.empty(); }

	inline void plot(ExEdit::Object const& obj, size_t index, int denom);
	inline void draw(HDC dc, int L, int T, int R, int B) const;

private:
	constexpr static int margin_lr = 3, margin_tb = 3;
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
	constexpr bool is_valid() const { return width > 0 && height > 0; }
	void invalidate() { width = height = 0; }

private:
	constexpr static int
		margin_r_easing = 8,
		margin_b_easing = 4, margin_b_graph = 4;
	constexpr static UINT draw_text_options = DT_NOCLIP | DT_NOPREFIX;

} content{};

// color conversion.
constexpr auto bgr2rgb(int32_t c) {
	return (std::rotl<uint32_t>(c, 16) & 0x00ff00ff) | (c & 0x0000ff00);
}

static inline constinit struct graph_pen {
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
	size_t const obj_index = &obj - *exedit.ObjectArray_ptr;
	auto const ofi = object_filter_index(obj_index, filter_index);
	char* const arg_name = reinterpret_cast<char*>(1 + rel_idx); // represents the trackbar index.

	size_t const num_sect = settings.graph.polls - 1;
	int const frame_begin = obj.frame_begin,
		frame_len = obj.frame_end - frame_begin
		+ (obj.index_midpt_leader >= 0 && exedit.NextObjectIdxArray[obj_index] >= 0 ? 1 : 0);
	float const len_f = static_cast<float>(std::max(frame_len, 1));

	// select frames.
	std::vector<int> frames{}; frames.reserve(num_sect + 1); frames.push_back(frame_begin);
	for (size_t i = 0; i < num_sect; i++) {
		int const f = frame_begin + (frame_len * (i + 1) + (num_sect >> 1)) / num_sect;
		if (frames.back() < f) frames.push_back(f);
	}

	// store the current frame position.
	if (int const curr_rel = *exedit.edit_frame_cursor - frame_begin;
		0 <= curr_rel && curr_rel <= frame_len)
		curr = curr_rel / len_f;
	else curr = -1;

	// retrieve the left and right values.
	int const val_l = obj.track_value_left[index],
		val_r = obj.track_value_right[index];
	int min, max;
	if (val_l < val_r) min = val_l, max = val_r;
	else min = val_r, max = val_l;

	// calculate each point.
	points.clear(); points.reserve(frames.size());
	for (auto f : frames) {
		int val; exedit.calc_trackbar(ofi, f, 0, &val, arg_name);

		// store the point and the statistics.
		points.emplace_back((f - frame_begin) / len_f, std::bit_cast<float>(val));
		if (val < min) min = val;
		else if (max < val) max = val;
	}

	// handling the single-frame interval.
	if (points.size() < 2)
		points.emplace_back(1.0f, points.back().second);

	// normalize the calculated values within the [0, 1] range.
	if (min == max) {
		min--; max++;

		// discard intermediate points.
		points[1] = points.back();
		points.resize(2);
	}
	float const range = static_cast<float>(max - min);
	val_left = (val_l - min) / range; val_right = (val_r - min) / range;
	for (auto& [_, y] : points)
		y = (std::bit_cast<int>(y) - min) / range;
}

inline void section_graph::draw(HDC dc, int L, int T, int R, int B) const
{
	// re-scale the coordinate.
	int X0 = L + margin_lr, X1 = R - margin_lr, Y1 = T + margin_tb, Y0 = B - margin_tb;
	rescale_dc<settings.graph.pixel_scale> rescale{ dc, L, T, R, B, X0, Y1, X1, Y0 };

	// prepare lambdas.
	auto const draw_line = [dc](int X1, int Y1, int X2, int Y2) {
		POINT pts[] = { {X1, Y1}, {X2, Y2} };
		::Polyline(dc, pts, std::size(pts));
	};
	auto const line_h = [=](int Y) { draw_line(L, Y, R, Y); };
	auto const line_v = [=](int X) { draw_line(X, T, X, B); };

	auto const func_x = [X0, D = X1 - X0](float x) -> int {
		return X0 + std::lroundf(D * x);
	};
	auto const func_y = [Y0, D = Y1 - Y0](float y) -> int {
		return Y0 + std::lroundf(D * y);
	};

	// draw grid lines.
	auto const dc_pen = ::GetStockObject(DC_PEN);
	auto const old_pen = ::SelectObject(dc, dc_pen);
	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_3));
	line_h(func_y(0.75f * val_left + 0.25f * val_right));
	line_h(func_y(0.50f * (val_left + val_right)));
	line_h(func_y(0.25f * val_left + 0.75f * val_right));
	line_v((3 * X0 + X1 + 2) >> 2);
	line_v((X0 + X1 + 1) >> 1);
	line_v((X0 + 3 * X1 + 2) >> 2);

	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_2));
	line_h(func_y(val_left));
	line_h(func_y(val_right));

	::SetDCPenColor(dc, bgr2rgb(settings.graph.line_color_1));
	line_v(X0);
	line_v(X1);

	// draw the easing curve.
	::SelectObject(dc, graph_pen);
	std::vector<POINT> pts{}; pts.reserve(points.size());
	for (auto const& [x, y] : points)
		pts.emplace_back(func_x(x), func_y(y));
	::Polyline(dc, pts.data(), std::size(pts));

	// draw the current frame.
	if (curr >= 0) {
		::SelectObject(dc, dc_pen);
		::SetDCPenColor(dc, bgr2rgb(settings.graph.cursor_color));
		auto const mode = ::SetROP2(dc, R2_XORPEN);
		line_v(func_x(curr));
		::SetROP2(dc, mode);
	}

	// set the pen back.
	::SelectObject(dc, old_pen);
}

inline void tooltip_content::measure(size_t index, HDC hdc)
{
	if (is_valid()) return;

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
			auto const vals = formatted_values{ obj, index };
			values = vals.span().trim_from_sect(
				settings.values.left < 0 ? vals.size() : settings.values.left,
				settings.values.right < 0 ? vals.size() : settings.values.right)
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
				graph.empty() ? 0 : settings.graph.height);
			pos_y_values = height - h_vals;
		}
		else {
			pos_y_values = 0;
			height = std::max(std::max(
				easing.empty() ? 0 : h_ease,
				values.empty() ? 0 : h_vals),
				graph.empty() ? 0 : settings.graph.height);
		}
	}
}

inline void tooltip_content::draw(HDC dc, RECT const& rc) const
{
	if (!is_valid()) return;

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
			constexpr auto dummy_text_a = "\t";
			constexpr auto dummy_text_w = L"\t";

			switch (hdr->code) {
			case TTN_GETDISPINFOA:
			{
				// supply the content string, which is this time a dummy,
				// or nothing if tooltip has no use for the current state.
				size_t idx = static_cast<size_t>(data);
				auto const& obj = (*exedit.ObjectArray_ptr)[*exedit.SettingDialogObjectIndex];
				auto const& mode = obj.track_mode[idx];
				if ((mode.num & 0x0f) == 0) break; // 移動無し
				if (settings.mode || settings.graph.enabled ||
					(settings.values.is_enabled() && !(
						obj.index_midpt_leader < 0 || // no mid-points.
						easing_spec{ mode }.twopoints))) {
					reinterpret_cast<NMTTDISPINFOA*>(lparam)
						->lpszText = const_cast<char*>(dummy_text_a);
					content.invalidate();
				}
				return 0;
			}

			case NM_CUSTOMDRAW:
			{
				auto const dhdr = reinterpret_cast<NMTTCUSTOMDRAW*>(lparam);
				switch (dhdr->nmcd.dwDrawStage) {
				case CDDS_PREPAINT:
				{
					// prepare the tooltip content.
					content.measure(static_cast<size_t>(data), dhdr->nmcd.hdc);

					// actual drawing is done on the CDDS_POSTPAINT notification.
					if (::IsWindowVisible(tooltip) != FALSE)
						return CDRF_NOTIFYPOSTPAINT;

					// if the tooltip window is invisible, it's measuring its own size,
					// and will resize before it actually draws itself.
					// calculate the lacking size and set the margin for it,
					// so the size of the tooltip will fit with the desired content.
					RECT margin{};
					::DrawTextW(dhdr->nmcd.hdc, dummy_text_w, 1, &margin, dhdr->uDrawFlags | DT_CALCRECT);
					margin.right = content.width - margin.right + 1; // slight extra space on the right.
					margin.bottom = content.height - margin.bottom;
					::SendMessageW(tooltip, TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&margin));
					break;
				}
				case CDDS_POSTPAINT:
				{
					// draw the content.
					content.draw(dhdr->nmcd.hdc, dhdr->nmcd.rc);
					break;
				}
				}
				return CDRF_DODEFAULT;
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

		read(int, graph., polls,		5, 1025);
		read(int, graph., curve_width,	1, 64 * graph.pixel_scale);

		read(int, graph., curve_color,	min_color, max_color);
		read(int, graph., cursor_color,	min_color, max_color);
		read(int, graph., line_color_1,	min_color, max_color);
		read(int, graph., line_color_2,	min_color, max_color);
		read(int, graph., line_color_3,	min_color, max_color);
	}

#undef read
}
