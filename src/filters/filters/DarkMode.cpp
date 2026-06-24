#include "stdafx.h"
#include "DarkMode.h"
#include "DSUtil/SysVersion.h"
#include <uxtheme.h>
#include <commctrl.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

namespace MPCDarkMode {

namespace {

	// Dark color scheme, same values TVTest's Dialog.cpp uses for its own
	// dark-mode dialogs.
	constexpr COLORREF ColorText = RGB(255, 255, 255);
	constexpr COLORREF ColorBack = RGB(32, 32, 32);
	constexpr COLORREF ColorFace = RGB(56, 56, 56);
	constexpr COLORREF ColorBorder = RGB(96, 96, 96);
	constexpr COLORREF ColorGrayText = RGB(155, 155, 155);

	BOOLEAN WINAPI ShouldAppsUseDarkModeImpl()
	{
		if (!SysVersion::IsWin10v1809orLater()) {
			return FALSE;
		}

		using PFN_ShouldAppsUseDarkMode = BOOLEAN(WINAPI*)();
		static const PFN_ShouldAppsUseDarkMode pShouldAppsUseDarkMode =
			reinterpret_cast<PFN_ShouldAppsUseDarkMode>(
				::GetProcAddress(::GetModuleHandleW(L"uxtheme.dll"), MAKEINTRESOURCEA(132)));

		return pShouldAppsUseDarkMode ? pShouldAppsUseDarkMode() : FALSE;
	}

	void ThemeComboBox(HWND hwnd, bool fDark)
	{
		::SetWindowTheme(hwnd, fDark ? L"DarkMode_CFD" : nullptr, nullptr);

		COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
		if (::GetComboBoxInfo(hwnd, &cbi) && cbi.hwndList) {
			SetWindowDarkTheme(cbi.hwndList, fDark);
		}
	}

	// BS_GROUPBOX doesn't reliably pick up dark colors from
	// SetWindowTheme(L"DarkMode_Explorer") alone, so it's custom-painted
	// here instead - same reasoning as TVTest's Dialog.cpp
	// DrawDarkModeGroupBox()/ButtonSubclassProc().
	constexpr UINT_PTR GroupBoxSubclassId = 1;

	LRESULT CALLBACK GroupBoxSubclassProc(
		HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		switch (uMsg) {
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				const HDC hdc = ::BeginPaint(hWnd, &ps);

				RECT rc;
				::GetClientRect(hWnd, &rc);

				const HBRUSH hFaceBrush = ::CreateSolidBrush(ColorFace);
				::FillRect(hdc, &rc, hFaceBrush);

				TCHAR szText[256] = {};
				const int TextLength = ::GetWindowText(hWnd, szText, _countof(szText));

				const HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
				const HGDIOBJ hOldFont = hFont ? ::SelectObject(hdc, hFont) : nullptr;
				const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

				RECT rcText = {};
				if (TextLength > 0) {
					::DrawText(hdc, szText, TextLength, &rcText, DT_SINGLELINE | DT_CALCRECT);
				}

				RECT rcBox = rc;
				rcBox.top += rcText.bottom / 2;

				const HBRUSH hBorderBrush = ::CreateSolidBrush(ColorBorder);
				RECT rcFrame = rcBox;
				::FrameRect(hdc, &rcFrame, hBorderBrush);
				::DeleteObject(hBorderBrush);

				if (TextLength > 0) {
					RECT rcTextBack = { rcBox.left + 6, rc.top, rcBox.left + 6 + rcText.right + 4, rcBox.top + 1 };
					::FillRect(hdc, &rcTextBack, hFaceBrush);

					RECT rcDraw = { rcTextBack.left + 2, rc.top, rcTextBack.right, rcText.bottom };
					::SetTextColor(hdc, ColorText);
					::DrawText(hdc, szText, TextLength, &rcDraw, DT_SINGLELINE);
				}

				::DeleteObject(hFaceBrush);
				::SetBkMode(hdc, OldBkMode);
				if (hOldFont) {
					::SelectObject(hdc, hOldFont);
				}

				::EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_ERASEBKGND:
			return 1;

		case WM_NCDESTROY:
			::RemoveWindowSubclass(hWnd, GroupBoxSubclassProc, uIdSubclass);
			break;
		}

		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void ThemeButton(HWND hwnd, bool fDark)
	{
		const DWORD style = static_cast<DWORD>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		if ((style & BS_TYPEMASK) == BS_GROUPBOX) {
			if (fDark) {
				::SetWindowSubclass(hwnd, GroupBoxSubclassProc, GroupBoxSubclassId, 0);
			} else {
				::RemoveWindowSubclass(hwnd, GroupBoxSubclassProc, GroupBoxSubclassId);
			}
			::InvalidateRect(hwnd, nullptr, TRUE);
			return;
		}

		SetWindowDarkTheme(hwnd, fDark);
	}

} // namespace


bool IsHighContrast()
{
	HIGHCONTRAST hc = { sizeof(hc) };
	return ::SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE)
		&& !!(hc.dwFlags & HCF_HIGHCONTRASTON);
}

bool IsDarkMode()
{
	return !IsHighContrast() && !!ShouldAppsUseDarkModeImpl();
}

bool SetWindowDarkTheme(HWND hwnd, bool fDark)
{
	return SUCCEEDED(::SetWindowTheme(hwnd, fDark ? L"DarkMode_Explorer" : nullptr, nullptr));
}

bool IsDarkModeSettingChanged(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_SETTINGCHANGE) {
		return false;
	}

	LPCWSTR pszName = reinterpret_cast<LPCWSTR>(lParam);
	return pszName != nullptr && ::_wcsicmp(pszName, L"ImmersiveColorSet") == 0;
}

COLORREF TextColor(bool fDark)
{
	return fDark ? ColorText : ::GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF BackColor(bool fDark)
{
	return fDark ? ColorBack : ::GetSysColor(COLOR_WINDOW);
}

COLORREF FaceColor(bool fDark)
{
	return fDark ? ColorFace : ::GetSysColor(COLOR_BTNFACE);
}

COLORREF GrayTextColor(bool fDark)
{
	return fDark ? ColorGrayText : ::GetSysColor(COLOR_GRAYTEXT);
}

void ApplyTheme(HWND hwndPage, bool fDark)
{
	for (HWND hwnd = ::GetWindow(hwndPage, GW_CHILD); hwnd != nullptr; hwnd = ::GetWindow(hwnd, GW_HWNDNEXT)) {
		TCHAR szClassName[32];
		if (::GetClassName(hwnd, szClassName, _countof(szClassName)) <= 0) {
			continue;
		}

		if (::lstrcmpi(szClassName, TEXT("BUTTON")) == 0) {
			ThemeButton(hwnd, fDark);
		} else if (::lstrcmpi(szClassName, TEXT("COMBOBOX")) == 0) {
			ThemeComboBox(hwnd, fDark);
		} else if (::lstrcmpi(szClassName, TEXT("EDIT")) == 0) {
			::SetWindowTheme(hwnd, fDark ? L"DarkMode_CFD" : nullptr, nullptr);
		} else if (::lstrcmpi(szClassName, TEXT("LISTBOX")) == 0
				|| ::lstrcmpi(szClassName, TEXT("SCROLLBAR")) == 0) {
			SetWindowDarkTheme(hwnd, fDark);
		}
	}
}

} // namespace MPCDarkMode
