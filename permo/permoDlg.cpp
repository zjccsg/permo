// permoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "permo.h"
#include "permoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CpermoDlg 对话框
#define IDM_TOPMOST                     10001
#define IDM_GREEN                       10002
#define IDM_BLUE                        10003
#define IDM_BLACK                       10004
#define IDM_RED                         10005
#define IDM_ORANGE                      10006
#define IDM_EXIT                        10007
#define IDM_AUTOHIDE					10008

__int64 CompareFileTime(FILETIME time1, FILETIME time2)
{
	__int64 a = time1.dwHighDateTime << 32 | time1.dwLowDateTime;
	__int64 b = time2.dwHighDateTime << 32 | time2.dwLowDateTime;

	return   (b - a);
}

CpermoDlg::CpermoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CpermoDlg::IDD, pParent)
	, SelectedInterface(0)
	, nCPU(0)
	, nMem(0)
	, fNetUp(0.0)
	, fNetDown(0.0)
	, bTopmost(TRUE)
	, nSkin(0)
	, total_net_up(0)
	, total_net_down(0)
	, isOnline(FALSE)
	, bAutoHide(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CpermoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CpermoDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_COMMAND(IDM_TOPMOST, &CpermoDlg::OnTopmost)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(IDM_GREEN, &CpermoDlg::OnGreen)
	ON_COMMAND(IDM_BLUE, &CpermoDlg::OnBlue)
	ON_COMMAND(IDM_BLACK, &CpermoDlg::OnBlack)
	ON_COMMAND(IDM_RED, &CpermoDlg::OnRed)
	ON_COMMAND(IDM_ORANGE, &CpermoDlg::OnOrange)
	ON_WM_MOVE()
	ON_COMMAND(IDM_EXIT, &CpermoDlg::OnExit)
	//}}AFX_MSG_MAP
	ON_WM_NCHITTEST()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CpermoDlg 消息处理程序

BOOL CpermoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rWorkArea, 0);   // 获得工作区大小
	BOOL bRet = OpenConfig();
	InitSize(bRet);
	GetWindowRect(&rCurPos);
	if (!::GetSystemTimes(&preidleTime, &prekernelTime, &preuserTime))
	{
		return -1;
	}
	MFNetTraffic m_cTrafficClassTemp;
	//更新：增加对所有发现的网络接口监控遍历，防止监控的接口非连接网络的接口
	int nCount = m_cTrafficClassTemp.GetNetworkInterfacesCount();
	for (int i = 0; i <= nCount; i++)
	{
		if (m_cTrafficClassTemp.GetInterfaceTotalTraffic(i) > 0)
		{
			SelectedInterface = i;
			isOnline = TRUE;
			break;
		}
	}
	//创建菜单
	InitPopMenu(nCount);
	//默认置顶
	if (bTopmost)
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		m_Menu.CheckMenuItem(IDM_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
	}
	//默认绿色
	if (!bRet)
	{
		nSkin = IDM_GREEN;
	}
	if (bAutoHide)
	{
		m_Menu.CheckMenuItem(IDM_AUTOHIDE, MF_BYCOMMAND | MF_CHECKED);
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED); // 在前面打钩 

	//设置网络监控类型
	m_cTrafficClassDown.SetTrafficType(MFNetTraffic::IncomingTraffic);
	m_cTrafficClassUp.SetTrafficType(MFNetTraffic::OutGoingTraffic);

	//取消任务栏显示
	SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, WS_EX_TOOLWINDOW);
	//每隔一秒刷新各种信息
	SetTimer(1, 1000, NULL);


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CpermoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);
		//解决闪烁问题,双缓冲绘图
		//可以单独建立一个内存DC类
		RECT rcClient;
		this->GetClientRect(&rcClient);
		CDC MemDC;
		CBitmap bitmap;
		MemDC.CreateCompatibleDC(&dc);
		bitmap.CreateCompatibleBitmap(&dc, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
		MemDC.SelectObject(&bitmap);
		DrawBackground(&MemDC);
		DrawInfo(&MemDC);
		dc.BitBlt(0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, &MemDC,
			0, 0, SRCCOPY);
		bitmap.DeleteObject();
		MemDC.DeleteDC();
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CpermoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CpermoDlg::InitSize(BOOL bInit)
{
	if (!bInit)
	{
		rCurPos.top = rWorkArea.bottom - 20;
		rCurPos.left = rWorkArea.right - 170;
	}
	rCurPos.right = rCurPos.left + 150;
	rCurPos.bottom = rCurPos.top + 20;
	MoveWindow(&rCurPos, TRUE);
}


void CpermoDlg::DrawBackground(CDC* pDC)
{
	switch (nSkin)
	{
	case IDM_GREEN:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(2, 200, 20));
			CBrush MiBrush(RGB(150, 240, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_BLUE:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(20, 80, 180));
			CBrush MiBrush(RGB(150, 150, 240));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_BLACK:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(50, 50, 50));
			CBrush MiBrush(RGB(100, 100, 100));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_RED:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(180, 20, 20));
			CBrush MiBrush(RGB(240, 150, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_ORANGE:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(150, 100, 20));
			CBrush MiBrush(RGB(200, 180, 50));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	default:
		{
			CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
			CBrush RLBrush(RGB(2, 200, 20));
			CBrush MiBrush(RGB(150, 240, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&RLBrush);

			//左侧矩形
			pDC->Rectangle(0, 0, 20, 20);
			//右侧矩形
			pDC->Rectangle(130, 0, 150, 20);

			pDC->SelectObject(&MiBrush);
			//中间矩形
			pDC->Rectangle(20, 0, 130, 20);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	}
	//以下三句可以不要,局部变量会进行析构
	// 	WhitePen.DeleteObject();
	// 	GreenBrush.DeleteObject();
	// 	WhiteBrush.DeleteObject();
}

void CpermoDlg::DrawInfo(CDC* pDC)
{
	CFont font, *pOldFont;
	LOGFONT logFont;
	pDC->GetCurrentFont()->GetLogFont(&logFont);
	logFont.lfWidth = 5;
	logFont.lfHeight = 16;
	logFont.lfWeight = 0;
	lstrcpy(logFont.lfFaceName, _T("微软雅黑"));
	font.CreateFontIndirect(&logFont);
	pOldFont = pDC->SelectObject(&font);
	COLORREF cOldTextColor;
	if (IDM_GREEN == nSkin || IDM_ORANGE == nSkin)
	{
		cOldTextColor = pDC->SetTextColor(RGB(0, 0, 0));
	}
	else
	{
		cOldTextColor = pDC->SetTextColor(RGB(255, 255, 255));
	}
	int nOldBkMode = pDC->SetBkMode(TRANSPARENT);
	CString strCPU, strMem, strNetUp, strNetDown;
	strCPU.Format(_T("%d"), nCPU);
	strMem.Format(_T("%d"), nMem);
	strNetUp.Format(_T("%.1fKB/s"), fNetUp);
	strNetDown.Format(_T("%.1fKB/s"), fNetDown);
	//pDC->TextOut(1, 1, strCPU);
	CRect rText;
	rText.left = 1;
	rText.right = 18;
	rText.top = 2;
	rText.bottom = 18;
	pDC->DrawText(strCPU, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rText.left = 131;
	rText.right = 149;
	pDC->DrawText(strMem, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->SetTextColor(RGB(0, 0, 0));
	rText.left = 23;
	rText.right = 73;
	pDC->DrawText(strNetDown, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rText.left = 77;
	rText.right = 127;
	pDC->DrawText(strNetUp, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);


	pDC->SetTextColor(cOldTextColor);
	pDC->SetBkMode(nOldBkMode);
	pDC->SelectObject(pOldFont);
	font.DeleteObject();
}

void CpermoDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	CDialog::OnLButtonDown(nFlags, point);
}


void CpermoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (1 == nIDEvent)
	{
		{
			if (!(bAutoHide && rCurPos.left < 0))
			{
				//获取CPU占用
				::GetSystemTimes(&idleTime, &kernelTime, &userTime);
				int idle = (int)CompareFileTime(preidleTime, idleTime);
				int kernel = (int)CompareFileTime(prekernelTime, kernelTime);
				int user = (int)CompareFileTime(preuserTime, userTime);
				int cpu = (kernel + user - idle) * 100 / (kernel + user);
				//int cpuidle = (idle)* 100 / (kernel + user);
				preidleTime = idleTime;
				prekernelTime = kernelTime;
				preuserTime = userTime;
				if (cpu < 0)
				{
					cpu = -cpu;
				}
				nCPU = cpu;
			}
		}
		{
			if (!(bAutoHide && rCurPos.right > rWorkArea.right))
			{
				//获取内存使用率
				memStatex.dwLength = sizeof(memStatex);
				::GlobalMemoryStatusEx(&memStatex);
				nMem = memStatex.dwMemoryLoad;
			}
			// 			//内存使用率。
			// 			printf("内存使用率:\t%d%%\r\n", memStatex.dwMemoryLoad);
			// 			//总共物理内存。
			// 			printf("总共物理内存:\t%I64d\r\n", memStatex.ullTotalPhys);
			// 			//可用物理内存。
			// 			printf("可用物理内存:\t%I64d\r\n", memStatex.ullAvailPhys);
			// 			//全部内存。
			// 			printf("全部内存:\t%I64d\r\n", memStatex.ullTotalPageFile);
			// 			//全部可用的内存。
			// 			printf("全部可用的内存:\t%I64d\r\n", memStatex.ullAvailPageFile);
			// 			//全部的虚拟内存。
			// 			printf("全部的内存:\t%I64d\r\n", memStatex.ullTotalVirtual);
		}
		{
			if (isOnline)
			{
				if (!(bAutoHide && (rCurPos.left < 0 || rCurPos.right > rWorkArea.right)))
				{
					//获取流量信息
					double traffic1 = m_cTrafficClassDown.GetTraffic(SelectedInterface);
					double traffic2 = m_cTrafficClassUp.GetTraffic(SelectedInterface);
					//total_net_down = m_cTrafficClassDown.GetInterfaceTotalTraffic(SelectedInterface);
					//total_net_up = m_cTrafficClassUp.GetInterfaceTotalTraffic(SelectedInterface);
					fNetDown = traffic1 / 1024.0;
					fNetUp = traffic2 / 1024.0;
				}
			}
		}
		Invalidate(FALSE);
	}
	CDialog::OnTimer(nIDEvent);
}


void CpermoDlg::OnTopmost()
{
	// TODO:  在此添加命令处理程序代码
	if (bTopmost)
	{
		SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		bTopmost = FALSE;
		m_Menu.CheckMenuItem(IDM_TOPMOST, MF_BYCOMMAND | MF_UNCHECKED);
	}
	else
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		bTopmost = TRUE;
		m_Menu.CheckMenuItem(IDM_TOPMOST, MF_BYCOMMAND | MF_CHECKED); // 在前面打钩 
	}
}

void CpermoDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CPoint p;
	//传递过来的坐标为相对于窗口左上角的坐标，WM_CONTEXTMENU传递过来的是屏幕坐标
	GetCursorPos(&p);//鼠标点的屏幕坐标
	int nID = m_Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, p.x, p.y, this);
	switch (nID)
	{
	case IDM_TOPMOST:
		OnTopmost();
		break;
	case IDM_GREEN:
		OnGreen();
		break;
	case IDM_BLUE:
		OnBlue();
		break;
	case IDM_BLACK:
		OnBlack();
		break;
	case IDM_RED:
		OnRed();
		break;
	case IDM_ORANGE:
		OnOrange();
		break;
	case IDM_EXIT:
		OnExit();
		break;
	case IDM_AUTOHIDE:
		OnAutoHide();
		break;
	case 0:
		return;
	default:
		break;
	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CpermoDlg::OnGreen()
{
	// TODO:  在此添加命令处理程序代码
	if (IDM_GREEN == nSkin)
	{
		return;
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_UNCHECKED);
	nSkin = IDM_GREEN;
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}


void CpermoDlg::OnBlue()
{
	// TODO:  在此添加命令处理程序代码
	if (IDM_BLUE == nSkin)
	{
		return;
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_UNCHECKED);
	nSkin = IDM_BLUE;
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}


void CpermoDlg::OnBlack()
{
	// TODO:  在此添加命令处理程序代码
	if (IDM_BLACK == nSkin)
	{
		return;
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_UNCHECKED);
	nSkin = IDM_BLACK;
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}


void CpermoDlg::OnRed()
{
	// TODO:  在此添加命令处理程序代码
	if (IDM_RED == nSkin)
	{
		return;
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_UNCHECKED);
	nSkin = IDM_RED;
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}


void CpermoDlg::OnOrange()
{
	// TODO:  在此添加命令处理程序代码
	if (IDM_ORANGE == nSkin)
	{
		return;
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_UNCHECKED);
	nSkin = IDM_ORANGE;
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}


//禁止移动到桌面之下
void CpermoDlg::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);
	GetWindowRect(&rCurPos);
	if (rCurPos.bottom > rWorkArea.bottom)
	{
		rCurPos.bottom = rWorkArea.bottom;
		rCurPos.top = rCurPos.bottom - 20;
		MoveWindow(&rCurPos, TRUE);
	}
}


void CpermoDlg::OnExit()
{
	// TODO:  在此添加命令处理程序代码
	SaveConfig();
	OnOK();
}


void CpermoDlg::InitPopMenu(int nCount)
{
	BOOL bRet = m_Menu.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_SubMenu_Skin.CreatePopupMenu();
	ASSERT(bRet);
	//MENUITEM mi1;
	mi1.strText = _T("窗口置顶");
	mi1.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TOPMOST));
	m_Menu.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_TOPMOST, (LPCTSTR)&mi1);

	mi9.strText = _T("贴边隐藏");
	mi9.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUTOHIDE));
	m_Menu.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_AUTOHIDE, (LPCTSTR)&mi9);

	//MENUITEM mi3;
	mi3.strText = _T("绿色");
	mi3.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_GREEN));
	m_SubMenu_Skin.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_GREEN, (LPCTSTR)&mi3);
	//MENUITEM mi4;
	mi4.strText = _T("蓝色");
	mi4.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BLUE));
	m_SubMenu_Skin.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_BLUE, (LPCTSTR)&mi4);
	//MENUITEM mi5;
	mi5.strText = _T("黑色");
	mi5.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BLACK));
	m_SubMenu_Skin.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_BLACK, (LPCTSTR)&mi5);
	//MENUITEM mi6;
	mi6.strText = _T("红色");
	mi6.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RED));
	m_SubMenu_Skin.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_RED, (LPCTSTR)&mi6);
	//MENUITEM mi7;
	mi7.strText = _T("黄色");
	mi7.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ORANGE));
	m_SubMenu_Skin.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_ORANGE, (LPCTSTR)&mi7);

	mi2.strText = _T("皮肤");
	mi2.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SKIN));
	m_Menu.AppendMenuW(MF_BYPOSITION | MF_POPUP | MF_OWNERDRAW,
		(UINT)m_SubMenu_Skin.m_hMenu, (LPCTSTR)&mi2);
	//MENUITEM mi8;
	mi8.strText = _T("退出");
	mi8.hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_EXIT));
	m_Menu.AppendMenuW(MF_OWNERDRAW | MF_BYCOMMAND, IDM_EXIT, (LPCTSTR)&mi8);
}


BOOL CpermoDlg::OpenConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	int left, top, topmost, skin, autohide;
	left = ::GetPrivateProfileInt(_T("Main"), _T("left"), -1, temp);
	top = ::GetPrivateProfileInt(_T("Main"), _T("top"), -1, temp);
	topmost = ::GetPrivateProfileInt(_T("Main"), _T("topmost"), -1, temp);
	skin = ::GetPrivateProfileInt(_T("Main"), _T("skin"), -1, temp);
	autohide = ::GetPrivateProfileInt(_T("Main"), _T("autohide"), -1, temp);
	if (-1 == left || left < 0)
	{
		return FALSE;
	}
	if (-1 == top || top < 0)
	{
		return FALSE;
	}
	if (-1 == topmost || (topmost != 0 && topmost != 1))
	{
		return FALSE;
	}
	if (skin != IDM_GREEN && skin != IDM_BLUE && skin != IDM_BLACK
		&& skin != IDM_RED && skin != IDM_ORANGE)
	{
		return FALSE;
	}
	if (-1 == autohide || (autohide != 0 && autohide != 1))
	{
		return FALSE;
	}
	rCurPos.left = left;
	rCurPos.top = top;
	bTopmost = topmost;
	nSkin = skin;
	bAutoHide = autohide;
	return TRUE;
}


BOOL CpermoDlg::SaveConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	TCHAR cLeft[32], cTop[32], cTopMost[32], cSkin[32], cAutoHide[32];
	_itow_s(rCurPos.left, cLeft, 10);
	_itow_s(rCurPos.top, cTop, 10);
	_itow_s(bTopmost, cTopMost, 10);
	_itow_s(nSkin, cSkin, 10);
	_itow_s(bAutoHide, cAutoHide, 10);
	::WritePrivateProfileString(_T("Main"), _T("left"), cLeft, temp);
	::WritePrivateProfileString(_T("Main"), _T("top"), cTop, temp);
	::WritePrivateProfileString(_T("Main"), _T("topmost"), cTopMost, temp);
	::WritePrivateProfileString(_T("Main"), _T("skin"), cSkin, temp);
	::WritePrivateProfileString(_T("Main"), _T("autohide"), cAutoHide, temp);
	return TRUE;
}
LRESULT CpermoDlg::OnNcHitTest(CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (bAutoHide)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_HOVER | TME_LEAVE; //注册非客户区离开
		tme.hwndTrack = m_hWnd;
		tme.dwHoverTime = HOVER_DEFAULT; //只对HOVER有效  
		::TrackMouseEvent(&tme);
	}
	return CDialog::OnNcHitTest(point);
}

void CpermoDlg::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (bAutoHide)
	{
		if (rCurPos.right == rWorkArea.right + 130)
		{
			rCurPos.right = rWorkArea.right;
			rCurPos.left = rCurPos.right - 150;
			MoveWindow(&rCurPos, TRUE);
		}
		if (rCurPos.left == rWorkArea.left - 130)
		{
			rCurPos.left = rWorkArea.left;
			rCurPos.right = rCurPos.left + 150;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	CDialog::OnMouseHover(nFlags, point);
}

void CpermoDlg::OnMouseLeave()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (bAutoHide)
	{
		if (rCurPos.right >= rWorkArea.right)
		{
			rCurPos.right = rWorkArea.right + 130;
			rCurPos.left = rCurPos.right - 150;
			MoveWindow(&rCurPos, TRUE);
		}
		if (rCurPos.left <= rWorkArea.left)
		{
			rCurPos.left = rWorkArea.left - 130;
			rCurPos.right = rCurPos.left + 150;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	CDialog::OnMouseLeave();
}

void CpermoDlg::OnAutoHide(void)
{
	if (bAutoHide)
	{
		bAutoHide = FALSE;
		m_Menu.CheckMenuItem(IDM_AUTOHIDE, MF_BYCOMMAND | MF_UNCHECKED);
	}
	else
	{
		bAutoHide = TRUE;
		m_Menu.CheckMenuItem(IDM_AUTOHIDE, MF_BYCOMMAND | MF_CHECKED); // 在前面打钩 
	}
}
