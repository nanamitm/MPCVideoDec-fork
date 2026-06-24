#pragma once

// Minimal dark-mode support for the internal property pages
// (CInternalPropertyPageWnd and its derivatives). These pages are WS_CHILD
// windows embedded by the host application into its own property-sheet
// frame, so unlike a normal top-level dialog there's no DWM titlebar or
// app-mode opt-in to deal with here - just following the OS "apps use dark
// mode" setting for our own child controls, the same way TVTest's
// DarkMode.cpp/Dialog.cpp do for its dialogs.

namespace MPCDarkMode {

	// !IsHighContrast() && "apps use dark theme" (Settings > Personalization > Colors).
	bool IsDarkMode();

	bool IsHighContrast();

	// SetWindowTheme(hwnd, fDark ? L"DarkMode_Explorer" : nullptr, nullptr)
	bool SetWindowDarkTheme(HWND hwnd, bool fDark);

	// True if this is a WM_SETTINGCHANGE for the "apps use dark theme" setting.
	bool IsDarkModeSettingChanged(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	COLORREF TextColor(bool fDark);
	COLORREF BackColor(bool fDark);
	COLORREF FaceColor(bool fDark);
	COLORREF GrayTextColor(bool fDark);

	// Walks the immediate children of hwndPage and themes them for dark/light
	// mode: SetWindowDarkTheme() for checkboxes/listboxes/scrollbars, a
	// dedicated dark/light "CFD" theme for comboboxes (incl. their dropdown
	// list) and edit fields, and a custom-paint subclass for BS_GROUPBOX
	// buttons (group boxes don't reliably pick up dark colors from
	// SetWindowTheme alone).
	void ApplyTheme(HWND hwndPage, bool fDark);

} // namespace MPCDarkMode
