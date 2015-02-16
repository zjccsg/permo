#pragma once


// CSkinButton

class CSkinButton : public CButton
{
	DECLARE_DYNAMIC(CSkinButton)

public:
	CSkinButton();
	virtual ~CSkinButton();
	void Init(UINT nNormalID,UINT nMouseOverID,CString strTipText);
	BOOL SetBitmap(UINT nNormalID,UINT nMouseOverID);
	void SetToolTipText(CString strText);
	BOOL SetButtonCursor(HCURSOR hCursor);

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CSkinButton)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void AdjustPosition();
	CWnd* pWndParent;
	UINT m_nMouseOverID;
	UINT m_nNormalID;
	CToolTipCtrl m_ToolTip;
	void SetDefaultCursor();
	HCURSOR m_hCursor;
	BOOL m_bMouseOver;
	BOOL bRun;
};


