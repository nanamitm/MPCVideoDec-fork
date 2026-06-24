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

#pragma once

#include <afxcmn.h>
#include <map>
#include <HighDPI.h>
#include "DarkMode.h"

interface __declspec(uuid("03481710-D73E-4674-839F-03EDE2D60ED8"))
ISpecifyPropertyPages2 :
public ISpecifyPropertyPages {
	STDMETHOD (CreatePage) (const GUID& guid, IPropertyPage** ppPage) PURE;
};

class CInternalPropertyPageWnd : public CWnd, public CDPI
{
	bool m_fDirty;
	CComPtr<IPropertyPageSite> m_pPageSite;

	void UpdateDarkModeBrushes();

protected:
	CFont m_font, m_monospacefont;
	int m_fontheight;

	bool m_fDarkMode = false;
	CBrush m_BackBrush;
	CBrush m_FaceBrush;

	// border: 0 - static or edit text without edges, 2 - radio bottom, 6 - edit text with edges
	void CalcTextRect(CRect& rect, long x, long y, long w, long border = 0);
	void CalcRect(CRect& rect, long x, long y, long w, long h);
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	// Hook for derived pages that own controls ApplyDarkMode() can't reach
	// generically (e.g. an owner-draw list box that paints its own items).
	virtual void OnDarkModeChanged(bool fDarkMode) {}

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

public:
	CInternalPropertyPageWnd();

	void SetDirty(bool fDirty = true) {
		m_fDirty = fDirty;
		if (m_pPageSite) {
			if (fDirty) {
				m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
			} else {
				m_pPageSite->OnStatusChange(PROPPAGESTATUS_CLEAN);
			}
		}
	}
	bool GetDirty() { return m_fDirty; }

	bool IsDarkMode() const { return m_fDarkMode; }

	// Re-themes the page's child controls for the current m_fDarkMode. Called
	// once (by CInternalPropertyPage::Activate()) after OnActivate() builds
	// the controls, and again from OnSettingChange() if the OS dark/light
	// setting changes live.
	void ApplyDarkMode();

	virtual BOOL Create(IPropertyPageSite* pPageSite, LPCRECT pRect, CWnd* pParentWnd);

	virtual bool OnConnect(const std::list<CComQIPtr<IUnknown, &IID_IUnknown>>& pUnks) { return true; }
	virtual void OnDisconnect() {}
	virtual bool OnActivate() { return true; }
	virtual void OnDeactivate() {}
	virtual bool OnApply() { return true; }

	DECLARE_MESSAGE_MAP()
};

class CInternalPropertyPage
	: public CUnknown
	, public IPropertyPage
	, public CCritSec
{
	CComPtr<IPropertyPageSite> m_pPageSite;
	std::list<CComQIPtr<IUnknown, &IID_IUnknown>> m_pUnks;
	CInternalPropertyPageWnd* m_pWnd;

protected:
	virtual CInternalPropertyPageWnd* GetWindow() PURE;
	virtual LPCWSTR GetWindowTitle() PURE;
	virtual CSize GetWindowSize() PURE;

public:
	CInternalPropertyPage(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CInternalPropertyPage();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPropertyPage

	STDMETHODIMP SetPageSite(IPropertyPageSite* pPageSite);
	STDMETHODIMP Activate(HWND hwndParent, LPCRECT pRect, BOOL fModal);
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetPageInfo(PROPPAGEINFO* pPageInfo);
	STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN* ppUnk);
	STDMETHODIMP Show(UINT nCmdShow);
	STDMETHODIMP Move(LPCRECT prect);
	STDMETHODIMP IsPageDirty();
	STDMETHODIMP Apply();
	STDMETHODIMP Help(LPCWSTR lpszHelpDir);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg);
};

template<class WndClass>
class CInternalPropertyPageTempl : public CInternalPropertyPage
{
	virtual CInternalPropertyPageWnd* GetWindow() { return DNew WndClass(); }
	virtual LPCWSTR GetWindowTitle() { return WndClass::GetWindowTitle(); }
	virtual CSize GetWindowSize() { return WndClass::GetWindowSize(); }

public:
	CInternalPropertyPageTempl(LPUNKNOWN lpunk, HRESULT* phr)
		: CInternalPropertyPage(lpunk, phr) {
	}
};
