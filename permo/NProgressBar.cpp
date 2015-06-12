// NProgressBar.cpp: Implementierungsdatei
//


/*
NProgressBar Version History:

24.6.2006
---------
- Repositioning bug fixed

27.05.2005
----------
- Fixed a lot of bugs!!!

25.05.2005
----------
- Progressbar is now centered and not streched in horizontal mode

16.05.2005
----------
- Changed code in PutWindowIntoTaskbar from ShowWindow(xxx) to RedrawWindow(...)
- SetPosEx changed

14.05.2005
----------
- First Release

*/


#include "stdafx.h"
#include "NProgressBar.h"
#include "NOperatingSystem.h"
#include "wnd.h"
#include "Resource.h"

/*
#ifdef BATTERYX
#include "..\\CRegistry.h"
#endif
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_EV 100
#define TIMER_EV2 101
extern unsigned int nBandShow;
extern CMenu m_BandMenu;

// Shell_TrayWnd
// |
// |------ ReBarWindow32
// |
// |------ TrayNotifyWnd

const TCHAR* CNProgressBar::m_pstcoReBarWindow = _T("ReBarWindow32");
const TCHAR* CNProgressBar::m_pstcoTrayNotifyWnd = _T("TrayNotifyWnd");
const TCHAR* CNProgressBar::m_pstcoParent = _T("Shell_TrayWnd");
int CNProgressBar::m_stnWindowWidth = 80;
int CNProgressBar::m_stnWindowHeight = 35;

/////////////////////////////////////////////////////////////////////////////
// CNProgressBar

CNProgressBar::CNProgressBar(CWnd* pcoParent)
{
	//add by zjc
	m_bCreated = false;
	//用来实现双击效果，默认双击无反应，当作单击进行处理
	dwAfter = 0;
	dwBefore = 0;
// 	BOOL bRet = m_Menu.CreatePopupMenu();
// 	ASSERT(bRet);
// 	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWCPU, _T("显示CPU占用"));
// 	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWMEM, _T("显示内存占用"));
// 	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWNET, _T("显示网络占用"));
// 	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_EXIT, _T("退出"));

	m_bOk = true;
	m_nMode = NOTDEFINED;
	m_nLeft=0;
	m_bSpezialColored = true;
	m_bHided = true;
	m_pcoParentWindow = pcoParent;

	m_ReBarWindowCurrentRect.left = m_ReBarWindowCurrentRect.right = m_ReBarWindowCurrentRect.top = m_ReBarWindowCurrentRect.bottom;

	m_pcoParent = new Wnd((TCHAR*)m_pstcoParent, NULL);
	m_pcoReBarWindow = new Wnd((TCHAR*)m_pstcoReBarWindow, m_pcoParent->GetHWnd());
	m_pcoTrayNotifyWnd = new Wnd((TCHAR*)m_pstcoTrayNotifyWnd, m_pcoParent->GetHWnd());

	
	CNOperatingSystem *pcoOS = new CNOperatingSystem();
	if (pcoOS->GetOS() < SBOS_NT4)
	{
		m_bOk = false;
	}
	delete pcoOS;
	 

	if (!(m_pcoReBarWindow->GetHWnd()))
	{
		::MessageBox(NULL,
					 m_pstcoReBarWindow,
					 _T("Can't find..."),
					 MB_OK);
		m_bOk = false;
	}
	
	if (!(m_pcoReBarWindow->GetHWnd()))
	{
		::MessageBox(NULL,
					 m_pstcoTrayNotifyWnd,
					 _T("Can't find..."),
					 MB_OK);
		m_bOk = false;
	}

	Wnd* m_pcoReBarWindow = new Wnd(_T("msctls_progress32"), m_pcoParent->GetHWnd());
	if (m_pcoReBarWindow->GetHWnd())
	{
		::MessageBox(NULL,
					 _T("I can place just one Progressbar in the Taskbar! Progressbar not created!"),
					 _T("Control already exists"),
					 MB_OK|MB_ICONWARNING);
		m_bOk = false;		
	}
	delete m_pcoReBarWindow;


	if (IsControlSuccessfullyCreated())
	{	
		this->StartUp();
		this->SetBkColour(RGB(0, 0, 0));
		this->SetForeColour(RGB(0,255,0));
		this->SetTextBkColour(RGB(0,0,0));
		this->SetTextForeColour(RGB(255,255,255));
		this->SetRange32(0, 100);
	}

}

CNProgressBar::~CNProgressBar()
{
	if (IsControlSuccessfullyCreated())
	{
		this->ReModifyTaskbar();	
	}
	delete m_pcoReBarWindow;
	delete m_pcoTrayNotifyWnd;
	delete m_pcoParent;
}


BEGIN_MESSAGE_MAP(CNProgressBar, CTextProgressCtrl)
	//{{AFX_MSG_MAP(CNProgressBar)
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen f黵 Nachrichten CNProgressBar 

void CNProgressBar::StartUp()
{
	this->ModifyTaskbar();
	this->PutWindowIntoTaskbar(m_pcoReBarWindow->GetProportion());	
	this->SetTimer(TIMER_EV, 10, 0);
	this->SetTimer(TIMER_EV2, 1000, 0);
}


bool CNProgressBar::IsControlSuccessfullyCreated()
{
	return m_bOk;
}

void CNProgressBar::PutWindowIntoTaskbar(int nDirection)
{
	RECT coRectReBar;
	CRect coRect;

	coRectReBar = m_pcoReBarWindow->GetRect();	

	int nDiff = 3;
	int nRightSmaller = 5;
	int nCenter = 0;

	if (m_pcoReBarWindow->GetProportion() == HORIZONTAL)
	{		
		m_nHeightTskControl =  m_pcoReBarWindow->GetRect().bottom - m_pcoReBarWindow->GetRect().top;
		if (m_nHeightTskControl > m_stnWindowHeight)
		{
			nCenter = (m_nHeightTskControl - m_stnWindowHeight) / 2;
			m_nHeightTskControl = m_stnWindowHeight ;			
		}

		nDiff = m_nHeightTskControl / 10;

		coRect.SetRect(coRectReBar.right,
					0+nDiff+nCenter, 
					coRectReBar.right+m_stnWindowWidth-nRightSmaller,
					m_nHeightTskControl-nDiff+nCenter);
							
		if (m_bCreated != true)
		{
			this->Create(PBS_SMOOTH | WS_CHILD  , coRect, CWnd::FromHandle(m_pcoParent->GetHWnd()),NULL);	
			m_bCreated = true;
		}
		else
		{
			this->MoveWindow(coRect.left, 
				             coRect.top, 
							 m_stnWindowWidth-nRightSmaller, 
							 coRect.bottom-coRect.top, 
							 TRUE);			

			this->RedrawWindow(NULL,NULL,RDW_UPDATENOW|RDW_FRAME|RDW_INVALIDATE );
		}
	}
	else if (m_pcoReBarWindow->GetProportion() == VERTICAL)
	{		

		coRect.SetRect(0+nDiff,
					coRectReBar.bottom, 
					coRectReBar.right-coRectReBar.left-nDiff,
					coRectReBar.bottom+m_stnWindowHeight-nRightSmaller);
							
		if (m_bCreated != true)
		{
			this->Create(PBS_SMOOTH | WS_CHILD/* | WS_VISIBLE*/  , coRect, CWnd::FromHandle(m_pcoParent->GetHWnd()),NULL);	
			m_bCreated = true;
		}
		else
		{
			this->MoveWindow(coRect.left, 
				             coRect.top, 
							 coRect.right-coRect.left, 
							 m_stnWindowHeight-nRightSmaller, 
							 TRUE);

			this->RedrawWindow(NULL,NULL,RDW_UPDATENOW|RDW_FRAME|RDW_INVALIDATE );
		}		
	}
} 

void CNProgressBar::ModifyTaskbar()
{	
	RECT coReBarWindowRect = m_pcoReBarWindow->GetRect();
	RECT coTrayNotifyWndRect = m_pcoTrayNotifyWnd->GetRect();	

	if (m_pcoReBarWindow->GetProportion() == HORIZONTAL)
	{ 
		//if ((coReBarWindowRect.left-m_ReBarWindowCurrentRect.left) > 2) // Avoid the running taskbar bug in non-skinned windows mode
		//{
			m_ReBarWindowCurrentRect.left = coReBarWindowRect.left + m_pcoParent->GetRect().left;
		//}

		m_ReBarWindowCurrentRect.right = coTrayNotifyWndRect.left-coReBarWindowRect.left-m_stnWindowWidth + m_pcoParent->GetRect().left;
		m_ReBarWindowCurrentRect.bottom = coReBarWindowRect.bottom-coReBarWindowRect.top;		
		m_ReBarWindowCurrentRect.top = 0;

		m_nMode = HORIZONTAL;		
	}
	else if (m_pcoReBarWindow->GetProportion() == VERTICAL)
	{		
		m_ReBarWindowCurrentRect.left =  0;
		m_ReBarWindowCurrentRect.right = coTrayNotifyWndRect.right-coTrayNotifyWndRect.left;

		//if ((coReBarWindowRect.top-m_ReBarWindowCurrentRect.top) > 2) // Avoid the running taskbar bug in non-skinned windows mode
		//{
			m_ReBarWindowCurrentRect.top = coReBarWindowRect.top + m_pcoParent->GetRect().top;
		//}

		m_ReBarWindowCurrentRect.bottom = coTrayNotifyWndRect.top-m_stnWindowHeight-coReBarWindowRect.top + m_pcoParent->GetRect().top;		
		
		m_nMode = VERTICAL;	
	}

	::MoveWindow(m_pcoReBarWindow->GetHWnd(), 
						m_ReBarWindowCurrentRect.left, 
						m_ReBarWindowCurrentRect.top, 
						m_ReBarWindowCurrentRect.right, 
						m_ReBarWindowCurrentRect.bottom, 
						TRUE);	

}

void CNProgressBar::ReModifyTaskbar()
{
	RECT coReBarWindowRect = m_pcoReBarWindow->GetRect();
	RECT coTrayNotifyWndRect = m_pcoTrayNotifyWnd->GetRect();

	if (m_nMode == HORIZONTAL)
	{
		//if ((coReBarWindowRect.left-m_ReBarWindowCurrentRect.left) > 2) // Avoid the running taskbar bug in non-skinned windows mode
		//{
			m_ReBarWindowCurrentRect.left =  coReBarWindowRect.left + m_pcoParent->GetRect().left;
		//}
		m_ReBarWindowCurrentRect.right = m_pcoTrayNotifyWnd->GetRect().left-coReBarWindowRect.left;
		m_ReBarWindowCurrentRect.top = 0;
		m_ReBarWindowCurrentRect.bottom = coReBarWindowRect.bottom-coReBarWindowRect.top;		
	}
	else if (m_nMode == VERTICAL)
	{
		m_ReBarWindowCurrentRect.left =  0;
		m_ReBarWindowCurrentRect.right = coTrayNotifyWndRect.right-coTrayNotifyWndRect.left;
		//if ((coReBarWindowRect.top-m_ReBarWindowCurrentRect.top) > 2) // Avoid the running taskbar bug in non-skinned windows mode
		//{
			m_ReBarWindowCurrentRect.top = coReBarWindowRect.top + m_pcoParent->GetRect().top;
		//}
		m_ReBarWindowCurrentRect.bottom = coTrayNotifyWndRect.top-coReBarWindowRect.top;		
	}

	::MoveWindow(m_pcoReBarWindow->GetHWnd(), 
				   m_ReBarWindowCurrentRect.left, 
				   m_ReBarWindowCurrentRect.top, 
				   m_ReBarWindowCurrentRect.right, 
				   m_ReBarWindowCurrentRect.bottom, 
				   TRUE);	
}

void CNProgressBar::OnTimer(UINT nIDEvent) 
{
	switch (nIDEvent)
	{
	case TIMER_EV:
		if (!m_bHided)
		{
			// BUGFIX 24.6.2006	
			if (m_pcoParent->GetProportion() == HORIZONTAL)
			{
				if ( ((m_pcoTrayNotifyWnd->GetRect().left - m_pcoReBarWindow->GetRect().right) < m_stnWindowWidth) ||
					 (m_pcoTrayNotifyWnd->IsRectChanged() ||
					  m_pcoParent->IsRectChanged())
					  )
				{
					this->Refresh();
					this->PutWindowIntoTaskbar(m_pcoReBarWindow->GetProportion());							
				}
				else
				{
					//this->RedrawWindow();
				}
			}
			else
			{
				if ( ((m_pcoTrayNotifyWnd->GetRect().top - m_pcoReBarWindow->GetRect().bottom) < m_stnWindowHeight) ||
					 (m_pcoTrayNotifyWnd->IsRectChanged() ||
					  m_pcoParent->IsRectChanged())
					 )
				{
					this->Refresh();
					this->PutWindowIntoTaskbar(m_pcoReBarWindow->GetProportion());							
				}
				else
				{
					//this->RedrawWindow();
				}
			}


			/*
			if (m_pcoTrayNotifyWnd->IsRectChanged() ||
				m_pcoParent->IsRectChanged()) // If an icon is added, ...
			{
				//this->ModifyTaskbar();
				this->Refresh();
				this->PutWindowIntoTaskbar(m_pcoReBarWindow->GetProportion());		
			}
			else
			{
				//this->Refresh();		
				this->RedrawWindow();
			}
			*/
		}
		break;
	case TIMER_EV2:
		this->Invalidate(FALSE);
		break;
	}
	
	CTextProgressCtrl::OnTimer(nIDEvent);
}


void CNProgressBar::Refresh()
{
	this->ModifyTaskbar();
	this->RedrawWindow();
}

void CNProgressBar::SetPosEx(int nPos)
{
	int r, g;
	
	if (IsControlSuccessfullyCreated())
	{
		if (m_bSpezialColored)
		{
			if (nPos<0) nPos = 0;
			if (nPos>255) nPos = 255;
			r=(int)(nPos*3.6);
			g=(int)((100-nPos)*3.6);
			if (g>255) g=255;
			if (r>255) r=255;
			if (g<0) g=0;
			if (r<0) r=0;
			this->SetForeColour(RGB(r,g,0));
		}
		else
		{ 
			this->SetBkColour(RGB(0, 0, 0));
			this->SetForeColour(RGB(0,255,0));
			this->SetTextBkColour(RGB(0,0,0));
			this->SetTextForeColour(RGB(255,255,255));
		}

		this->SetPos(nPos);
		this->RedrawWindow();
	}
}


void CNProgressBar::Show(bool bShow)
{
	if (!bShow)
	{
		this->ShowWindow(false);
		KillTimer(TIMER_EV);
		KillTimer(TIMER_EV2);
		ReModifyTaskbar();
		m_bHided = true;

	}
	else
	{
		this->ModifyTaskbar();
		this->ShowWindow(true);
		this->PutWindowIntoTaskbar(m_pcoReBarWindow->GetProportion());	
		this->SetTimer(TIMER_EV, 10, 0);
		this->SetTimer(TIMER_EV2, 1000, 0);
		m_bHided = false;
	}
}

bool CNProgressBar::IsHided()
{
	return m_bHided;
}

void CNProgressBar::SetColorMode(bool bColorMode)
{
	m_bSpezialColored = bColorMode;
	this->RedrawWindow();
}

void CNProgressBar::OnRButtonDown(UINT nFlags, CPoint point) 
{
	//第二种方法
/*	CPoint p;*/
	//传递过来的坐标为相对于窗口左上角的坐标，WM_CONTEXTMENU传递过来的是屏幕坐标
/*	GetCursorPos(&p);//鼠标点的屏幕坐标*/
/*	int nID = m_Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, p.x, p.y, this);*/
	//交由主窗口进行处理
	::SendMessage(this->m_pcoParentWindow->GetSafeHwnd(), MSG_BAND_MENU, (WPARAM)0, (LPARAM)0);
// 	switch(nID)
// 	{
// 	case IDM_BANDSHOWCPU:
// 		{
// 			if (nBandShow != 0)
// 			{
// 				m_Menu.CheckMenuItem(nBandShow + IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_UNCHECKED); 
// 				nBandShow = 0;
// 				m_Menu.CheckMenuItem(IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_CHECKED); 
// 			}
// 		}
// 		break;
// 	case IDM_BANDSHOWMEM:
// 		if (nBandShow != 1)
// 		{
// 			m_Menu.CheckMenuItem(nBandShow + IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_UNCHECKED); 
// 			nBandShow = 1;
// 			m_Menu.CheckMenuItem(IDM_BANDSHOWMEM, MF_BYCOMMAND | MF_CHECKED); 
// 		}
// 		break;
// 	case IDM_BANDSHOWNET:
// 		if (nBandShow != 2)
// 		{
// 			m_Menu.CheckMenuItem(nBandShow + IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_UNCHECKED); 
// 			nBandShow = 2;
// 			m_Menu.CheckMenuItem(IDM_BANDSHOWNET, MF_BYCOMMAND | MF_CHECKED); 
// 		}
// 		break;
// 	case IDM_EXIT:
// 		break;
// 	default:
// 		break;
// 	}
	//CTextProgressCtrl::OnRButtonDown(nFlags, point);
}

void CNProgressBar::OnRButtonUp(UINT nFlags, CPoint point) 
{
	//ReleaseCapture();
	//CTextProgressCtrl::OnRButtonUp(nFlags, point);
}

void CNProgressBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	/*
	//AfxMessageBox(_T("此次鼠标为双击！"));  
	if (this->m_pcoParentWindow->IsWindowVisible())
	{
		this->m_pcoParentWindow->ShowWindow(SW_HIDE);
	}
	else
	{
		this->m_pcoParentWindow->ShowWindow(SW_SHOW);
	}
	//CTextProgressCtrl::OnLButtonDblClk(nFlags, point);
	*/
}

void CNProgressBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	/*
	//使用左键单击消息模拟左键双击消息
	dwAfter = GetTickCount();
	if (dwBefore != 0 && dwAfter != 0)
	{
		if(dwAfter - dwBefore < GetDoubleClickTime())   
		{
			OnLButtonDblClk(nFlags, point);
			dwAfter = 0;
			dwBefore = 0;
		} 
	}
	dwBefore = GetTickCount();
	*/
}

void CNProgressBar::SetWidth(int nWidth)
{
	m_stnWindowWidth = nWidth;
}

void CNProgressBar::SetHeight(int nHeight)
{
	m_stnWindowHeight = nHeight;
}
