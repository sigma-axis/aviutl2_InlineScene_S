/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <string>
#include <cerrno>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32")
#include <imm.h>
#pragma comment(lib, "imm32")

#include "InlineScene_S.hpp"
#define HEADER_NAME (L"Plugin." PLUGIN_NAME L".Settings")
using AviUtl2::translate;

#include "Settings.hpp"
#include "Setting_Dlg.hpp"
using namespace InlineScene_S::Settings;
using namespace GUI;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B;
////////////////////////////////
// helper functions.
////////////////////////////////
constexpr auto dialog_class_name = L"InlineScene_S_SettingDialogClass";
constexpr wchar_t const* dialog_title = PLUGIN_NAME L" の設定";
constexpr wchar_t const* label_texts[] = {
	L"長期キャッシュの保持期間 (分)", // Setting::cache_expire_minutes
	L"長期キャッシュの総ドット数", // Setting::cache_total_dots
};

constexpr int scale_by_dpi(int value, int dpi)
{
	return (value * dpi + 48) / 96;
}

constexpr int descale_by_dpi(int value, int dpi)
{
	return (value * 96 + (dpi >> 1)) / dpi;
}

HFONT create_caption_font()
{
	if (::NONCLIENTMETRICSW ncm{ .cbSize = sizeof(ncm) };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) [[likely]]
		return ::CreateFontIndirectW(&ncm.lfCaptionFont);
	else return reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
}

void set_control_font(HWND hwnd, HFONT font)
{
	[[assume(font != nullptr)]];
	if (hwnd != nullptr) [[likely]]
		::SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

SIZE measure_text_size(HWND hwnd, std::wstring_view const& text, HFONT font)
{
	[[assume(hwnd != nullptr)]];
	HDC const hdc = ::GetDC(hwnd);
	HFONT const previousFont = reinterpret_cast<HFONT>(::SelectObject(hdc, font));
	SIZE size = {};
	::GetTextExtentPoint32W(hdc, text.data(), static_cast<int>(text.size()), &size);
	::SelectObject(hdc, previousFont);
	::ReleaseDC(hwnd, hdc);
	return size;
}

struct dialog_cxt {
	dialog_result result{};
	Setting current{};
};

dialog_cxt* get_cxt(HWND hwnd)
{
	return reinterpret_cast<dialog_cxt*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

static bool get_dlg_item_int(HWND hwnd, int id, int32_t& out)
{
	BOOL translated{};
	int const value = ::GetDlgItemInt(hwnd, id, &translated, TRUE);
	if (translated == FALSE) return false;
	out = value;
	return true;
}
static bool get_dlg_item_int(HWND hwnd, int id, int64_t& out)
{
	wchar_t buffer[64];
	int const length = ::GetDlgItemTextW(hwnd, id, buffer, std::size(buffer));
	if (length <= 0) return false;

	wchar_t* end = nullptr;
	errno = 0;
	auto const value = std::wcstoll(buffer, &end, 10);
	if (errno == ERANGE || end == buffer || *end != L'\0') return false;
	out = value;
	return true;
}

template<class PropT>
static void clamp_edit_value(HWND hwnd, int id)
{
	using int_type = PropT::type;
	int_type value = 0;
	if (!get_dlg_item_int(hwnd, id, value)) return;

	int_type const clamped = std::clamp(value, PropT::min, PropT::max);
	if (clamped == value) return;
	::SetDlgItemTextW(hwnd, id, std::to_wstring(clamped).c_str());
}

////////////////////////////////
// window message handler.
////////////////////////////////
namespace ctrl_id
{
	constexpr int
		min_edit = 1001,
		min_spin = 1002,
		min_label = 1003,

		total_edit = 1004,
		total_label = 1005,

		_dummy = -1;
}

LRESULT CALLBACK esc_hook_func(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
	UINT_PTR id, DWORD_PTR)
{
	if ((msg == WM_GETDLGCODE || msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) &&
		wparam == VK_ESCAPE) {
		// close the host window on ESC key.
		::PostMessageW(::GetParent(hwnd), WM_CLOSE, 0, 0);
		return 0;
	}
	if (msg == WM_NCDESTROY)
		::RemoveWindowSubclass(hwnd, &esc_hook_func, id);

	return ::DefSubclassProc(hwnd, msg, wparam, lparam);
}
void set_esc_hook(HWND hwnd)
{
	::SetWindowSubclass(hwnd, &esc_hook_func, reinterpret_cast<UINT_PTR>(&set_esc_hook), 0);
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_CREATE:
	{
		// associate the context to the window.
		auto* const create_struct = reinterpret_cast<::CREATESTRUCTW*>(lparam);
		auto* const context = reinterpret_cast<dialog_cxt*>(create_struct->lpCreateParams);
		if (context == nullptr) return -1;
		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));
		return 0;
	}
	case WM_SHOWWINDOW:
	{
		if (wparam != FALSE) {
			// set focus to the edit control and select all text.
			auto const min_edit = ::GetDlgItem(hwnd, ctrl_id::min_edit);
			::SetFocus(min_edit);
			::SendMessageW(min_edit, EM_SETSEL, 0, -1);
		}
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wparam) == EN_KILLFOCUS) {
			// clamp to the valid range when the focus is lost.
			switch (LOWORD(wparam)) {
			case ctrl_id::min_edit:
				clamp_edit_value<decltype(Setting::cache_expire_minutes)>(hwnd, ctrl_id::min_edit);
				return 0;
			case ctrl_id::total_edit:
				clamp_edit_value<decltype(Setting::cache_total_dots)>(hwnd, ctrl_id::total_edit);
				return 0;
			}
		}

		if (LOWORD(wparam) == IDOK && HIWORD(wparam) == BN_CLICKED) {
			// OK clicked.
			auto* const context = get_cxt(hwnd);
			if (context == nullptr) {
				::DestroyWindow(hwnd);
				return 0;
			}
			context->result.confirmed = true;
			int min_value{}; int64_t total_dots{};
			context->result.settings = Setting{
				.cache_expire_minutes = get_dlg_item_int(hwnd, ctrl_id::min_edit, min_value) ?
					min_value : context->current.cache_expire_minutes.get(),
				.cache_total_dots = get_dlg_item_int(hwnd, ctrl_id::total_edit, total_dots) ?
					total_dots : context->current.cache_total_dots.get(),
			};
			::DestroyWindow(hwnd);
			return 0;
		}
		break;
	}

	case WM_CLOSE:
	case WM_KEYDOWN:
	case WM_SYSCOMMAND:
	case WM_GETDLGCODE:
	{
		if (msg == WM_KEYDOWN && wparam != VK_ESCAPE) break;
		if (msg == WM_SYSCOMMAND && wparam != SC_CLOSE) break;
		if (msg == WM_GETDLGCODE && wparam != VK_ESCAPE) break;

		// cancel.
		auto* context = get_cxt(hwnd);
		if (context != nullptr) {
			context->result.confirmed = false;
			context->result.settings = context->current;
		}
		::DestroyWindow(hwnd);
		return 0;
	}
	case WM_DESTROY:
	{
		// exit from the modal message loop.
		::PostQuitMessage(0);
		return 0;
	}

	default: break;
	}

	return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

void register_dialog_class(HMODULE dll_hinst)
{
	static bool registered = false;
	if (registered) return;

	::WNDCLASSEXW const wc{
		.cbSize = sizeof(wc),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = &wnd_proc,
		.hInstance = dll_hinst,
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1),
		.lpszClassName = dialog_class_name,
	};
	::RegisterClassExW(&wc);
	registered = true;
}

HWND create_dialog(HWND parent, HMODULE dll_hinst, HFONT gui_font, dialog_cxt& context)
{
	int const dpi = static_cast<int>(::GetDpiForWindow(parent));

	// Layout uses DPI-scaled logical units.
	FRAME_MARGIN const margin{
		static_cast<int16_t>(scale_by_dpi(8, dpi)),
		static_cast<int16_t>(scale_by_dpi(8, dpi)),
		static_cast<int16_t>(scale_by_dpi(4, dpi)),
		static_cast<int16_t>(scale_by_dpi(8, dpi)),
	};
	SIZE const button_size = {
		scale_by_dpi(80, dpi),
		scale_by_dpi(20, dpi),
	};
	int const row_gap = scale_by_dpi(4, dpi), col_gap = scale_by_dpi(8, dpi);
	int const min_edit_width = scale_by_dpi(52, dpi), total_edit_width = scale_by_dpi(72, dpi);
	int const label_tail = scale_by_dpi(8, dpi), button_gap = scale_by_dpi(12, dpi);

	std::wstring_view const
		min_label_text = translate(label_texts[0], HEADER_NAME),
		total_label_text = translate(label_texts[1], HEADER_NAME);
	SIZE const min_label_text_size = measure_text_size(parent, min_label_text, gui_font),
		min_edit_size = { min_edit_width, min_label_text_size.cy + 2 * (::GetSystemMetricsForDpi(SM_CYBORDER, dpi) + 1) },
		min_label_size = { min_label_text_size.cx + label_tail, min_edit_size.cy },
		total_label_text_size = measure_text_size(parent, total_label_text, gui_font),
		total_edit_size = { total_edit_width, min_edit_size.cy },
		total_label_size = { total_label_text_size.cx + label_tail, min_edit_size.cy };
	int const value_area_right = margin.left + std::max({
		min_label_size.cx + col_gap + min_edit_size.cx,
		total_label_size.cx + col_gap + total_edit_size.cx,
		button_size.cx,
		});
	POINT const min_label_pos = { margin.left, margin.top },
		min_edit_pos = { value_area_right - min_edit_size.cx, min_label_pos.y },
		total_label_pos = { margin.left, min_label_pos.y + min_edit_size.cy + row_gap },
		total_edit_pos = { value_area_right - total_edit_size.cx, total_label_pos.y },
		button_pos = {
			value_area_right - button_size.cx,
			total_edit_pos.y + total_edit_size.cy + button_gap,
	};
	SIZE const client_size = {
		button_pos.x + button_size.cx + margin.right,
		button_pos.y + button_size.cy + margin.bottom,
	};

	// dialog size.
	constexpr DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
		window_ex_style = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
	RECT rc{ 0, 0, client_size.cx, client_size.cy };
	::AdjustWindowRectExForDpi(&rc, window_style, FALSE, window_ex_style, dpi);
	SIZE const window_size = { rc.right - rc.left, rc.bottom - rc.top };
	POINT window_pos = { rc.left, rc.top };

	// dialog position: place the cursor at the center of the OK button,
	// while keeping the whole dialog within the display work area.
	POINT cursor_pos{};
	::GetCursorPos(&cursor_pos);
	HMONITOR const monitor = ::MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitor_info{ .cbSize = sizeof(monitor_info) };
	::GetMonitorInfoW(monitor, &monitor_info);
	window_pos.x += cursor_pos.x - (button_pos.x + button_size.cx / 2);
	window_pos.y += cursor_pos.y - (button_pos.y + button_size.cy / 2);
	window_pos.x = std::clamp(window_pos.x, monitor_info.rcWork.left,
		std::max(monitor_info.rcWork.left, monitor_info.rcWork.right - window_size.cx));
	window_pos.y = std::clamp(window_pos.y, monitor_info.rcWork.top,
		std::max(monitor_info.rcWork.top, monitor_info.rcWork.bottom - window_size.cy));

	// create dialog window.
	HWND const hwnd = ::CreateWindowExW(
		window_ex_style,
		dialog_class_name,
		translate(dialog_title, HEADER_NAME),
		window_style,
		window_pos.x, window_pos.y, window_size.cx, window_size.cy,
		parent, nullptr, dll_hinst,
		&context);
	if (hwnd == nullptr) return nullptr;

	// create controls.
	HWND const min_label = ::CreateWindowExW(
		0, WC_STATICW, min_label_text.data(),
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		min_label_pos.x, min_label_pos.y, min_label_size.cx, min_label_size.cy,
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrl_id::min_label)),
		dll_hinst, nullptr);
	set_control_font(min_label, gui_font);

	HWND const min_edit = ::CreateWindowExW(
		0, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_RIGHT,
		min_edit_pos.x, min_edit_pos.y, min_edit_size.cx, min_edit_size.cy,
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrl_id::min_edit)),
		dll_hinst, nullptr);
	::ImmAssociateContext(min_edit, nullptr); // disable IME for this control.
	set_control_font(min_edit, gui_font);
	set_esc_hook(min_edit);

	HWND const min_spin = ::CreateWindowExW(
		0, UPDOWN_CLASSW, nullptr,
		WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_HOTTRACK,
		0, 0, 0, 0, // auto-buddy is set.
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrl_id::min_spin)),
		dll_hinst, nullptr);
	set_esc_hook(min_spin);
	::SendMessageW(min_spin, UDM_SETRANGE32, context.current.cache_expire_minutes.min, context.current.cache_expire_minutes.max);
	::SendMessageW(min_spin, UDM_SETPOS32, 0, context.current.cache_expire_minutes.get());

	HWND const total_label = ::CreateWindowExW(
		0, WC_STATICW, total_label_text.data(),
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		total_label_pos.x, total_label_pos.y, total_label_size.cx, total_label_size.cy,
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrl_id::total_label)),
		dll_hinst, nullptr);
	set_control_font(total_label, gui_font);

	HWND const total_edit = ::CreateWindowExW(
		0, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_RIGHT,
		total_edit_pos.x, total_edit_pos.y, total_edit_size.cx, total_edit_size.cy,
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrl_id::total_edit)),
		dll_hinst, nullptr);
	::ImmAssociateContext(total_edit, nullptr); // disable IME for this control.
	set_control_font(total_edit, gui_font);
	set_esc_hook(total_edit);
	::SetWindowTextW(total_edit, std::to_wstring(context.current.cache_total_dots.get()).c_str());

	HWND const ok_button = ::CreateWindowExW(
		0, WC_BUTTONW, L"OK",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		button_pos.x, button_pos.y, button_size.cx, button_size.cy,
		hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDOK)),
		dll_hinst, nullptr);
	set_control_font(ok_button, gui_font);
	set_esc_hook(ok_button);

	return hwnd;
}

ANON_NS_E

////////////////////////////////
// Exported functions.
////////////////////////////////
dialog_result GUI::show_setting_dialog(HWND parent, HMODULE dll_hinst, Setting const& current)
{
	register_dialog_class(dll_hinst);
	HFONT const gui_font = create_caption_font();

	// disable the parent.
	::EnableWindow(parent, FALSE);

	// prepare dialog context.
	dialog_cxt context{
		.result = {
			.confirmed = false,
			.settings = current,
		},
		.current = current,
	};

	if (HWND const hwnd = create_dialog(parent, dll_hinst, gui_font, context);
		hwnd != nullptr) {
		// show the dialog window.
		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		// message loop for modal dialog.
		MSG msg;
		while (::GetMessageW(&msg, nullptr, 0, 0) && msg.message != WM_QUIT) {
			if (!::IsDialogMessageW(hwnd, &msg)) {
				::TranslateMessage(&msg);
				::DispatchMessageW(&msg);
			}
		}
	}

	// cleanup.
	::EnableWindow(parent, TRUE);
	::SetForegroundWindow(parent);
	::DeleteObject(gui_font);

	return context.result;
}
