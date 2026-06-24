/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/std_helper.h"

#include "InternalPropertyPage.h"


#define DEFAULT_FONTSIZE 13

//
// CInternalPropertyPageWnd
//

void CInternalPropertyPageWnd::CalcTextRect(CRect& rect, long x, long y, long w, long border)
{
	rect.left   = x;
	rect.top    = y;
	rect.right  = x + w;
	rect.bottom = y + border;
	ScaleRect(rect);
	rect.bottom += m_fontheight;
}

void CInternalPropertyPageWnd::CalcRect(CRect& rect, long x, long y, long w, long h)
{
	rect.left   = x;
	rect.top    = y;
	rect.right  = x + w;
	rect.bottom = y + h;
	ScaleRect(rect);
}

CInternalPropertyPageWnd::CInternalPropertyPageWnd()
	: m_fDirty(false)
	, m_fontheight(DEFAULT_FONTSIZE)
{

}

BOOL CInternalPropertyPageWnd::Create(IPropertyPageSite* pPageSite, LPCRECT pRect, CWnd* pParentWnd)
{
	if (!pPageSite || !pRect) {
		return FALSE;
	}

	m_pPageSite = pPageSite;

	LPCWSTR wc = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, 0, (HBRUSH)(COLOR_BTNFACE + 1));
	if (!CreateEx(0, wc, L"CInternalPropertyPageWnd", WS_CHILDWINDOW, *pRect, pParentWnd, 0)) {
		return FALSE;
	}

	UseCurentMonitorDPI(m_hWnd);

	if (!m_font.m_hObject) {
		CString face;
		WORD height;
		extern BOOL AFXAPI AfxGetPropSheetFont(CString& strFace, WORD& wSize, BOOL bWizard); // yay
		if (!AfxGetPropSheetFont(face, height, FALSE)) {
			return FALSE;
		}

		LOGFONTW lf = { 0 };
		wcscpy_s(lf.lfFaceName, face);
		lf.lfHeight = -PointsToPixels(height);
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		if (!m_font.CreateFontIndirectW(&lf)) {
			return FALSE;
		}

		lf.lfHeight -= -1;
		wcscpy_s(lf.lfFaceName, L"Lucida Console");
		if (!m_monospacefont.CreateFontIndirectW(&lf)) {
			wcscpy_s(lf.lfFaceName, L"Courier New");
			if (!m_monospacefont.CreateFontIndirectW(&lf)) {
				return FALSE;
			}
		}

		HDC hDC = ::GetDC(nullptr);
		HFONT hFontOld = (HFONT)SelectObject(hDC, m_font.m_hObject);
		CSize size;
		GetTextExtentPoint32W(hDC, L"x", 1, &size);
		SelectObject(hDC, hFontOld);
		::ReleaseDC(nullptr, hDC);

		m_fontheight = size.cy;
	}

	SetFont(&m_font);

	m_fDarkMode = MPCDarkMode::IsDarkMode();
	UpdateDarkModeBrushes();

	return TRUE;
}

BOOL CInternalPropertyPageWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (message == WM_COMMAND || message == WM_HSCROLL || message == WM_VSCROLL) {
		WORD notify = HIWORD(wParam);
		// check only notifications that change the state of a control, otherwise false positives are possible.
		if (notify == BN_CLICKED
				|| notify == CBN_SELCHANGE
				|| notify == EN_CHANGE
				|| notify == CLBN_CHKCHANGE) {
			SetDirty(true);
		}
	}

	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

void CInternalPropertyPageWnd::UpdateDarkModeBrushes()
{
	if (m_BackBrush.GetSafeHandle()) {
		m_BackBrush.DeleteObject();
	}
	if (m_FaceBrush.GetSafeHandle()) {
		m_FaceBrush.DeleteObject();
	}
	m_BackBrush.CreateSolidBrush(MPCDarkMode::BackColor(m_fDarkMode));
	m_FaceBrush.CreateSolidBrush(MPCDarkMode::FaceColor(m_fDarkMode));
}

void CInternalPropertyPageWnd::ApplyDarkMode()
{
	if (m_hWnd) {
		MPCDarkMode::ApplyTheme(m_hWnd, m_fDarkMode);
	}
}

HBRUSH CInternalPropertyPageWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (!m_fDarkMode) {
		return __super::OnCtlColor(pDC, pWnd, nCtlColor);
	}

	switch (nCtlColor) {
	case CTLCOLOR_STATIC:
	case CTLCOLOR_BTN:
		pDC->SetTextColor(MPCDarkMode::TextColor(true));
		pDC->SetBkColor(MPCDarkMode::FaceColor(true));
		return static_cast<HBRUSH>(m_FaceBrush.GetSafeHandle());

	case CTLCOLOR_EDIT:
	case CTLCOLOR_LISTBOX:
		pDC->SetTextColor(MPCDarkMode::TextColor(true));
		pDC->SetBkColor(MPCDarkMode::BackColor(true));
		return static_cast<HBRUSH>(m_BackBrush.GetSafeHandle());
	}

	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

BOOL CInternalPropertyPageWnd::OnEraseBkgnd(CDC* pDC)
{
	if (!m_fDarkMode) {
		return __super::OnEraseBkgnd(pDC);
	}

	CRect rc;
	GetClientRect(&rc);
	pDC->FillRect(&rc, &m_FaceBrush);
	return TRUE;
}

void CInternalPropertyPageWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	__super::OnSettingChange(uFlags, lpszSection);

	if (MPCDarkMode::IsDarkModeSettingChanged(m_hWnd, WM_SETTINGCHANGE, uFlags, reinterpret_cast<LPARAM>(lpszSection))) {
		const bool fDarkMode = MPCDarkMode::IsDarkMode();
		if (fDarkMode != m_fDarkMode) {
			m_fDarkMode = fDarkMode;
			UpdateDarkModeBrushes();
			ApplyDarkMode();
			OnDarkModeChanged(m_fDarkMode);
			RedrawWindow(nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
		}
	}
}

BEGIN_MESSAGE_MAP(CInternalPropertyPageWnd, CWnd)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

//
// CInternalPropertyPage
//

CInternalPropertyPage::CInternalPropertyPage(LPUNKNOWN lpunk, HRESULT* phr)
	: CUnknown(L"CInternalPropertyPage", lpunk)
	, m_pWnd(nullptr)
{
	if (phr) {
		*phr = S_OK;
	}
}

CInternalPropertyPage::~CInternalPropertyPage()
{
	if (m_pWnd) {
		if (m_pWnd->m_hWnd) {
			ASSERT(0);
			m_pWnd->DestroyWindow();
		}
		delete m_pWnd;
		m_pWnd = nullptr;
	}
}

STDMETHODIMP CInternalPropertyPage::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI2(IPropertyPage)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IPropertyPage

STDMETHODIMP CInternalPropertyPage::SetPageSite(IPropertyPageSite* pPageSite)
{
	CAutoLock cAutoLock(this);

	if (pPageSite && m_pPageSite || !pPageSite && !m_pPageSite) {
		return E_UNEXPECTED;
	}

	m_pPageSite = pPageSite;

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Activate(HWND hwndParent, LPCRECT pRect, BOOL fModal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	CheckPointer(pRect, E_POINTER);

	if (!m_pWnd || m_pWnd->m_hWnd || m_pUnks.empty()) {
		return E_UNEXPECTED;
	}

	if (!m_pWnd->Create(m_pPageSite, pRect, CWnd::FromHandle(hwndParent))) {
		return E_OUTOFMEMORY;
	}

	if (!m_pWnd->OnActivate()) {
		m_pWnd->DestroyWindow();
		return E_FAIL;
	}

	m_pWnd->ApplyDarkMode();

	m_pWnd->ModifyStyleEx(WS_EX_DLGMODALFRAME, WS_EX_CONTROLPARENT);
	m_pWnd->ShowWindow(SW_SHOWNORMAL);

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Deactivate()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd || !m_pWnd->m_hWnd) {
		return E_UNEXPECTED;
	}

	m_pWnd->OnDeactivate();

	m_pWnd->ModifyStyleEx(WS_EX_CONTROLPARENT, 0);
	m_pWnd->DestroyWindow();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::GetPageInfo(PROPPAGEINFO* pPageInfo)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPageInfo, E_POINTER);

	LPOLESTR pszTitle;
	HRESULT hr = AMGetWideString(CStringW(GetWindowTitle()), &pszTitle);
	if (FAILED(hr)) {
		return hr;
	}

	pPageInfo->cb = sizeof(PROPPAGEINFO);
	pPageInfo->pszTitle = pszTitle;
	pPageInfo->pszDocString = nullptr;
	pPageInfo->pszHelpFile = nullptr;
	pPageInfo->dwHelpContext = 0;
	pPageInfo->size = GetWindowSize();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::SetObjects(ULONG cObjects, LPUNKNOWN* ppUnk)
{
	CAutoLock cAutoLock(this);

	if (cObjects && m_pWnd || !cObjects && !m_pWnd) {
		return E_UNEXPECTED;
	}

	m_pUnks.clear();

	if (cObjects > 0) {
		CheckPointer(ppUnk, E_POINTER);

		for (ULONG i = 0; i < cObjects; i++) {
			m_pUnks.emplace_back(ppUnk[i]);
		}

		m_pWnd = GetWindow();
		if (!m_pWnd) {
			return E_OUTOFMEMORY;
		}

		if (!m_pWnd->OnConnect(m_pUnks)) {
			delete m_pWnd;
			m_pWnd = nullptr;

			return E_FAIL;
		}
	} else {
		m_pWnd->OnDisconnect();

		m_pWnd->DestroyWindow();
		delete m_pWnd;
		m_pWnd = nullptr;
	}

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Show(UINT nCmdShow)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd) {
		return E_UNEXPECTED;
	}

	if ((nCmdShow != SW_SHOW) && (nCmdShow != SW_SHOWNORMAL) && (nCmdShow != SW_HIDE)) {
		return E_INVALIDARG;
	}

	m_pWnd->ShowWindow(nCmdShow);
	m_pWnd->Invalidate();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Move(LPCRECT pRect)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	CheckPointer(pRect, E_POINTER);

	if (!m_pWnd) {
		return E_UNEXPECTED;
	}

	m_pWnd->MoveWindow(pRect, TRUE);

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::IsPageDirty()
{
	CAutoLock cAutoLock(this);

	return m_pWnd && m_pWnd->GetDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CInternalPropertyPage::Apply()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd || m_pUnks.empty() || !m_pPageSite) {
		return E_UNEXPECTED;
	}

	if (m_pWnd->GetDirty() && m_pWnd->OnApply()) {
		m_pWnd->SetDirty(false);
	}

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Help(LPCWSTR lpszHelpDir)
{
	CAutoLock cAutoLock(this);

	return E_NOTIMPL;
}

STDMETHODIMP CInternalPropertyPage::TranslateAccelerator(LPMSG lpMsg)
{
	CAutoLock cAutoLock(this);

	return E_NOTIMPL;
}
