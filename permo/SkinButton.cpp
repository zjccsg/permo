// SkinButton.cpp : 实现文件
//

#include "stdafx.h"
#include "SkinButton.h"

#define TIP_ID  1
// CSkinButton

IMPLEMENT_DYNAMIC(CSkinButton, CButton)

CSkinButton::CSkinButton()
: bRun(FALSE)
{

}

CSkinButton::~CSkinButton()
{
}


BEGIN_MESSAGE_MAP(CSkinButton, CButton)
	//{{AFX_MSG_MAP(CSkinButton)
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinButton message handlers

void CSkinButton::Init(UINT nNormalID, UINT nMouseOverID,CString strTipText)
{
  m_nNormalID=nNormalID;
  m_nMouseOverID=nMouseOverID;
  m_ToolTip.UpdateTipText(strTipText,this,TIP_ID);
  pWndParent=this->GetParent();
  AdjustPosition();

  m_bMouseOver=FALSE;
  bRun = FALSE;
  
  Invalidate();
}
void CSkinButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	// TODO: Add your code to draw the specified item
	UINT nID;
	CDC*pDC;
	pDC=CDC::FromHandle(lpDrawItemStruct->hDC);
	UINT nState=lpDrawItemStruct->itemState;

    if(m_bMouseOver&&(!(nState & ODS_SELECTED)))
		nID=m_nMouseOverID;
	else
		nID=m_nNormalID;
    
	CBitmap bitmap;
	BITMAP m_Bitmap;
	CDC MemDC;
	

	bitmap.LoadBitmap(nID);
	MemDC.CreateCompatibleDC(pDC);
	MemDC.SelectObject(&bitmap);
	bitmap.GetBitmap(&m_Bitmap);


	pDC->BitBlt(0,0,m_Bitmap.bmWidth,m_Bitmap.bmHeight,&MemDC,0,0,SRCCOPY);
	
	bitmap.DeleteObject();
	MemDC.DeleteDC();
	
}

BOOL CSkinButton::SetButtonCursor(HCURSOR hCursor)
{
	m_hCursor=hCursor;
	if(m_hCursor==NULL){
		SetDefaultCursor();
		return FALSE;
	}

	return TRUE;
}

void CSkinButton::SetDefaultCursor()
{
	m_hCursor=LoadCursor(NULL,MAKEINTRESOURCE(32649));
}

void CSkinButton::SetToolTipText(CString strText)
{
	m_ToolTip.UpdateTipText(strText,this,TIP_ID);

}

BOOL CSkinButton::SetBitmap(UINT nNormalID, UINT nMouseOverID)
{
	m_nNormalID=nNormalID;
	m_nMouseOverID=nMouseOverID;
	AdjustPosition();
	Invalidate();
	return TRUE;
}

void CSkinButton::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	SetDefaultCursor();

	CRect rect;
	GetClientRect(&rect);
	m_ToolTip.Create(this);
	m_ToolTip.SetDelayTime(100);
	m_ToolTip.SetMaxTipWidth(200);
	m_ToolTip.AddTool(this,_T(""),rect,TIP_ID);

	CButton::PreSubclassWindow();
}

BOOL CSkinButton::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	m_ToolTip.RelayEvent(pMsg);
	return CButton::PreTranslateMessage(pMsg);
}



BOOL CSkinButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_hCursor)  //如果设置了光标，就使用新设置的 光标
    {	
		::SetCursor(m_hCursor);		
        return TRUE;
    }
	return FALSE;
}

void CSkinButton::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	//添加bRun变量是为了防止按钮图片的频繁更换，仅仅在鼠标在上面时执行一次Invalidate函数
	//之后就不执行，而仅仅判断鼠标是否移除按钮区域
	m_bMouseOver=TRUE;	
	if (!bRun)
	{
		SetTimer(1,100,NULL);
		Invalidate();
	}
	bRun = TRUE;
	//OnTimer(1);
	CButton::OnMouseMove(nFlags, point);
}

void CSkinButton::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(&point);
	CRect rect;
	GetClientRect(&rect);
	if(!rect.PtInRect(point)){
		KillTimer(1);
		m_bMouseOver=FALSE;
		bRun = FALSE;
		Invalidate();
	}

	CButton::OnTimer(nIDEvent);
}

void CSkinButton::AdjustPosition()
{
	CBitmap bitmap;
	BITMAP m_Bitmap;
	CRect rect;

	bitmap.LoadBitmap(m_nNormalID);
	bitmap.GetBitmap(&m_Bitmap);

  	GetWindowRect(&rect);//根据图象的大小调整按钮大小
	pWndParent->ScreenToClient(&rect);
	rect.right=rect.left+m_Bitmap.bmWidth;
	rect.bottom=rect.top+m_Bitmap.bmHeight;
	MoveWindow(&rect,TRUE);
}