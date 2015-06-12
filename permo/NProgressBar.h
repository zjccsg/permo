#if !defined(AFX_NPROGRESSBAR_H__432FF06E_0462_4C88_9DB9_9C36B32DBAD2__INCLUDED_)
#define AFX_NPROGRESSBAR_H__432FF06E_0462_4C88_9DB9_9C36B32DBAD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NProgressBar.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Fenster CNProgressBar 

#include "globals.h"

class Wnd;

#include "TextProgressCtrl.h"


class CNProgressBar : public CTextProgressCtrl
{
// Konstruktion
public:
	CNProgressBar(CWnd* pcoParent);

	void SetPosEx(int nPos);
	void SetColorMode(bool bColorMode);
	void Show(bool bShow);
	void Refresh();
	bool IsHided();
	bool IsControlSuccessfullyCreated();

// Attribute
public:

// Operationen
public:

// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CNProgressBar)
	//}}AFX_VIRTUAL

// Implementierung
public:
	virtual ~CNProgressBar();

	// Generierte Nachrichtenzuordnungsfunktionen
protected:
	//{{AFX_MSG(CNProgressBar)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	CWnd*              m_pcoParentWindow;
	bool               m_bOk;
	bool               m_bWindowSet;
	bool               m_bSet;
	bool               m_bCreated;
	int                m_nMode;
	int                m_nLeft;
	int                m_nHeightTskControl;
	static const TCHAR* m_pstcoReBarWindow;
	static const TCHAR* m_pstcoTrayNotifyWnd;
	static const TCHAR* m_pstcoParent;
	static int         m_stnWindowWidth;
	static int         m_stnWindowHeight;
	CRect              m_ReBarWindowCurrentRect;
	Wnd*               m_pcoReBarWindow;
	Wnd*               m_pcoTrayNotifyWnd;
	Wnd*               m_pcoParent;
	bool               m_bSpezialColored;
	bool               m_bHided;
	bool			   m_bTaskbarSizeChanged;
	DWORD			   dwBefore;
	DWORD			   dwAfter;
/*	CMenu			   m_Menu;*/
private:
	void               StartUp();
	void               PutWindowIntoTaskbar(int nDirection);
	void               ModifyTaskbar();
	void               ReModifyTaskbar();
public:
	void SetWidth(int nWidth);
	void SetHeight(int nHeight);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_NPROGRESSBAR_H__432FF06E_0462_4C88_9DB9_9C36B32DBAD2__INCLUDED_
