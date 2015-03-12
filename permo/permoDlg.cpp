// permoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "permo.h"
#include "permoDlg.h"

#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

unsigned int nSkin;			//皮肤编号
// 总的上传流量
DWORD total_net_up;
// 总的下载流量
DWORD total_net_down;
//明细窗口显示2部分(true)或者3部分(false)
bool bShowNetInfo;
//bool bIsWindowsVistaOrGreater;
unsigned int nFontSize;
// CpermoDlg 对话框

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
	, nTrans(255)
	, fNetUp(0.0)
	, fNetDown(0.0)
	, bTopmost(TRUE)
	, isOnline(TRUE)
	, bAutoHide(FALSE)
	, nShowWay(0)
	, _bMouseTrack(TRUE)
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
	ON_COMMAND(IDM_EXIT, &CpermoDlg::OnExit)
	//}}AFX_MSG_MAP
//	ON_WM_NCHITTEST()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
//	ON_WM_NCLBUTTONUP()
ON_WM_MOUSEMOVE()
ON_WM_ERASEBKGND()
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
	//AllocConsole();
	//freopen("CONOUT$","w",stdout);
	//bIsWindowsVistaOrGreater = false;
	//判断操作系统版本
// 	DWORD dwVersion = 0;
// 	DWORD dwMajorVersion = 0;
//     DWORD dwMinorVersion = 0; 
// 	dwVersion = ::GetVersion();
// 	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
//     //dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
// 	if (dwMajorVersion > 5)
// 	{
// 		bIsWindowsVistaOrGreater = true;
// 	}
	/*
	If dwMajorVersion = 6 And dwMinorVersion = 1 Then GetWinVersion = "windows 7"
    If dwMajorVersion = 6 And dwMinorVersion = 0 Then GetWinVersion = "windows vista"
    If dwMajorVersion = 5 And dwMinorVersion = 1 Then GetWinVersion = "windows xp"
    If dwMajorVersion = 5 And dwMinorVersion = 0 Then GetWinVersion = "windows 2000"
	*/
	BOOL bRet = FALSE;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rWorkArea, 0);   // 获得工作区大小
	bRet = SetWorkDir();
	if (!bRet)
	{
		AfxMessageBox(_T("额...初始化失败！"));
		return FALSE;
	}
	OpenConfig();
	InitSize();
	CreateInfoDlg();
	GetWindowRect(&rCurPos);
	if (!::GetSystemTimes(&preidleTime, &prekernelTime, &preuserTime))
	{
		return -1;
	}
	m_SubMenu_NetPort.CreatePopupMenu();
	MFNetTraffic m_cTrafficClassTemp;
	//更新：增加对所有发现的网络接口监控遍历，防止监控的接口非连接网络的接口
	double tottraff = 0, maxtmp = 0;
	CString tmp, tmp2;
	int nCount = m_cTrafficClassTemp.GetNetworkInterfacesCount();
	for (int i = 0; i <= nCount; i++)
	{
		if ((tottraff = m_cTrafficClassTemp.GetInterfaceTotalTraffic(i) / (1024.0*1024.0)) > 0)
		{
			if (tottraff > maxtmp)
			{
				maxtmp = tottraff;
				SelectedInterface = i;
				isOnline = TRUE;
			}
		}
		m_cTrafficClassTemp.GetNetworkInterfaceName(&tmp, i);
		tmp2.Format(_T("%s : %.1f MB"), tmp, tottraff);
		m_SubMenu_NetPort.AppendMenu(MF_STRING, i + START_INDEX, tmp2);
	}
	//创建菜单
	InitPopMenu(nCount);
	//默认置顶
	if (bTopmost)
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		m_Menu.CheckMenuItem(IDM_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bAutoHide)
	{
		m_Menu.CheckMenuItem(IDM_AUTOHIDE, MF_BYCOMMAND | MF_CHECKED);
	}
	if (0 == nShowWay)
	{
		m_Menu.CheckMenuItem(IDM_SHOWBYHOVER, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		m_Menu.CheckMenuItem(IDM_SHOWBYLDOWN, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bShowNetInfo)
	{
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_UNCHECKED);
	}
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED); // 在前面打钩 
	m_Menu.CheckMenuItem(IDM_FONTSIZE12 + nFontSize - 12, MF_BYCOMMAND | MF_CHECKED);
	IfAutoRun();//判断是否已经开机自启
	isOnline = TRUE;

	//设置网络监控类型
	m_cTrafficClassDown.SetTrafficType(MFNetTraffic::IncomingTraffic);
	m_cTrafficClassUp.SetTrafficType(MFNetTraffic::OutGoingTraffic);

	//取消任务栏显示
	SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, WS_EX_TOOLWINDOW);
	//每隔一秒刷新CPU和网络信息
	SetTimer(1, 1000, NULL);
	//刷新内存信息
	SetTimer(2, 5000, NULL);
	
	::SetWindowLong( m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_CHECKED);

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

void CpermoDlg::InitSize()
{
	if (-1 == rCurPos.left || -1 == rCurPos.top)
	{
		rCurPos.top = rWorkArea.bottom - 22;
		rCurPos.left = rWorkArea.right - 250;
	}
	rCurPos.right = rCurPos.left + 220;
	rCurPos.bottom = rCurPos.top + 22;
	MoveWindow(&rCurPos, TRUE);
}


void CpermoDlg::DrawBackground(CDC* pDC)
{
	CPen MyPen(PS_SOLID, 1, RGB(255, 255, 255));
	switch (nSkin)
	{
	case IDM_GREEN:
		{
			CBrush RLBrush(RGB(2, 200, 20));
			CBrush MiBrush(RGB(150, 240, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_BLUE:
		{
			CBrush RLBrush(RGB(26, 160, 225));
			CBrush MiBrush(RGB(66, 66, 66));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_BLACK:
		{
			CBrush RLBrush(RGB(50, 50, 50));
			CBrush MiBrush(RGB(100, 100, 100));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_RED:
		{
			CBrush RLBrush(RGB(180, 20, 20));
			CBrush MiBrush(RGB(240, 150, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	case IDM_ORANGE:
		{
			CBrush RLBrush(RGB(230, 100, 25));
			CBrush MiBrush(RGB(250, 230, 20));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);
			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);
		}
		break;
	default:
		{
			CBrush RLBrush(RGB(2, 200, 20));
			CBrush MiBrush(RGB(150, 240, 150));
			CPen *pOldPen = pDC->SelectObject(&MyPen);
			CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
			//画边框和竖线
			pDC->Rectangle(0, 0, 220, 22);
			pDC->MoveTo(35, 0);
			pDC->LineTo(35, 22);
			pDC->MoveTo(185, 0);
			pDC->LineTo(185, 22);
			//选择不带边框的画笔
			pDC->SelectStockObject(NULL_PEN);
			pDC->SelectObject(&RLBrush);
			//左侧矩形
			pDC->Rectangle(1, 1, 36, 22);
			//右侧矩形
			pDC->Rectangle(186, 1, 220, 22);
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
	logFont.lfWidth = 0;
	logFont.lfHeight = nFontSize;
	logFont.lfWeight = FW_REGULAR;
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
	strCPU.Format(_T("%d%%"), nCPU);
	strMem.Format(_T("%d%%"), nMem);
	if (fNetUp >= 1000)
	{
		strNetUp.Format(_T("%.2fMB/S"), fNetUp/1024.0);
	}
	else
	{
		if (fNetUp < 100)
		{
			strNetUp.Format(_T("%.1fKB/S"), fNetUp);
		}
		else
		{
			strNetUp.Format(_T("%.0fKB/S"), fNetUp);
		}
	}
	if (fNetDown >= 1000)
	{
		strNetDown.Format(_T("%.2fMB/S"), fNetDown/1024.0);
	}
	else
	{
		if (fNetDown < 100)
		{
			strNetDown.Format(_T("%.1fKB/S"), fNetDown);
		}
		else
		{
			strNetDown.Format(_T("%.0fKB/S"), fNetDown);
		}
	}
	CRect rText;
	rText.left = 1;
	rText.right = 36;
	rText.top = 2;
	rText.bottom = 21;
	pDC->DrawText(strCPU, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rText.left = 186;
	rText.right = 219;
	pDC->DrawText(strMem, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (IDM_BLACK == nSkin || IDM_BLUE == nSkin)
	{
		pDC->SetTextColor(RGB(255, 255, 255));
	}
	else
	{
		pDC->SetTextColor(RGB(0, 0, 0));
	}

	CRect rcIcon;
	rcIcon.left = 38;
	rcIcon.right = 50;
	rcIcon.top = 5;
	rcIcon.bottom = 17;
	DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DOWN)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
	rText.left = 52;
	rText.right = 110;
	pDC->DrawText(strNetDown, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rcIcon.left = 112;
	rcIcon.right = 124;
	DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_UP)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
	rText.left = 126;
	rText.right = 185;
	pDC->DrawText(strNetUp, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);


	pDC->SetTextColor(cOldTextColor);
	pDC->SetBkMode(nOldBkMode);
	pDC->SelectObject(pOldFont);
	font.DeleteObject();
}

void CpermoDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	//PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	SetCapture();
	CDialog::OnLButtonDown(nFlags, point);
}


void CpermoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (1 == nIDEvent)
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
		if (isOnline)
		{
			if (!(bAutoHide && (rCurPos.left < 0 || rCurPos.right > rWorkArea.right)))
			{
				//获取流量信息
				double traffic1 = m_cTrafficClassDown.GetTraffic(SelectedInterface);
				double traffic2 = m_cTrafficClassUp.GetTraffic(SelectedInterface);
				total_net_down = m_cTrafficClassDown.GetInterfaceTotalTraffic(SelectedInterface);
				total_net_up = m_cTrafficClassUp.GetInterfaceTotalTraffic(SelectedInterface);
				fNetDown = traffic1 / 1024.0;
				fNetUp = traffic2 / 1024.0;
			}
		}
		Invalidate(FALSE);
	}
	if (2 == nIDEvent)
	{
		if (!(bAutoHide && rCurPos.right > rWorkArea.right))
		{
			//获取内存使用率
			MEMORYSTATUSEX memStatex;
			memStatex.dwLength = sizeof(memStatex);
			::GlobalMemoryStatusEx(&memStatex);
			nMem = memStatex.dwMemoryLoad;
		}
		// 		//内存使用率。
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
	m_Menu.CheckMenuItem(SelectedInterface + START_INDEX, MF_BYCOMMAND | MF_CHECKED); 
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
	case IDM_SHOWBYHOVER:
		SetShowWay();
		break;
	case IDM_SHOWBYLDOWN:
		SetShowWay();
		break;
	case IDM_TRANS0:
		OnTrans0();
		break;
	case IDM_TRANS10:
		OnTrans10();
		break;
	case IDM_TRANS20:
		OnTrans20();
		break;
	case IDM_TRANS30:
		OnTrans30();
		break;
	case IDM_TRANS40:
		OnTrans40();
		break;
	case IDM_TRANS50:
		OnTrans50();
		break;
	case IDM_TRANS60:
		OnTrans60();
		break;
	case IDM_TRANS70:
		OnTrans70();
		break;
	case IDM_TRANS80:
		OnTrans80();
		break;
	case IDM_TRANS90:
		OnTrans90();
		break;
	case IDM_FONTSIZE12:
		SetFontSize(12);
		break;
	case IDM_FONTSIZE13:
		SetFontSize(13);
		break;
	case IDM_FONTSIZE14:
		SetFontSize(14);
		break;
	case IDM_FONTSIZE15:
		SetFontSize(15);
		break;
	case IDM_FONTSIZE16:
		SetFontSize(16);
		break;
	case IDM_FONTSIZE17:
		SetFontSize(17);
		break;
	case IDM_FONTSIZE18:
		SetFontSize(18);
		break;
	case IDM_AUTOSTART:
		SetAutoRun();
		break;
	case IDM_SHOWNETINFO:
		ShowNetInfo();
		break;
	case 0:
		return;
	default:
		{m_Menu.CheckMenuItem(SelectedInterface + START_INDEX, MF_BYCOMMAND | MF_UNCHECKED);
		SelectedInterface = nID - START_INDEX; }
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


void CpermoDlg::OnExit()
{
	// TODO:  在此添加命令处理程序代码
	if (!SaveConfig())
	{
		AfxMessageBox(_T("额...配置信息保存失败！"));
	}
	OnOK();
}


void CpermoDlg::InitPopMenu(int nCount)
{
	BOOL bRet = m_Menu.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_SubMenu_Skin.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_SubMenu_Trans.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_SubMenu_ShowWay.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_SubMenu_FontSize.CreatePopupMenu();
	ASSERT(bRet);
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_TOPMOST, _T("保持悬浮窗置顶"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_AUTOHIDE, _T("开启悬浮窗贴边隐藏"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_SHOWNETINFO, _T("显示流量监控信息"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_SEPARATOR);
	m_SubMenu_Skin.AppendMenu(MF_BYCOMMAND, IDM_GREEN, _T("绿色"));
	m_SubMenu_Skin.AppendMenu(MF_BYCOMMAND, IDM_BLUE, _T("蓝色"));
	m_SubMenu_Skin.AppendMenu(MF_BYCOMMAND, IDM_BLACK, _T("黑色"));
	m_SubMenu_Skin.AppendMenu(MF_BYCOMMAND, IDM_RED, _T("红色"));
	m_SubMenu_Skin.AppendMenu(MF_BYCOMMAND, IDM_ORANGE, _T("黄色"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_Skin.m_hMenu, _T("设置皮肤颜色"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE12, _T("12"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE13, _T("13"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE14, _T("14"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE15, _T("15"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE16, _T("16"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE17, _T("17"));
	m_SubMenu_FontSize.AppendMenu(MF_BYCOMMAND, IDM_FONTSIZE18, _T("18"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_FontSize.m_hMenu, _T("设置字体大小"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS0, _T("不透明"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS10, _T("10%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS20, _T("20%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS30, _T("30%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS40, _T("40%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS50, _T("50%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS60, _T("60%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS70, _T("70%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS80, _T("80%"));
	m_SubMenu_Trans.AppendMenu(MF_BYCOMMAND, IDM_TRANS90, _T("90%"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_Trans.m_hMenu, _T("悬浮窗透明度设置"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_SEPARATOR);
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_NetPort.m_hMenu, _T("选择需要监控的网络接口"));
	m_SubMenu_ShowWay.AppendMenu(MF_BYCOMMAND, IDM_SHOWBYHOVER, _T("鼠标滑过立即弹出"));
	m_SubMenu_ShowWay.AppendMenu(MF_BYCOMMAND, IDM_SHOWBYLDOWN, _T("鼠标点击弹出"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_ShowWay.m_hMenu, _T("明细窗口弹出方式"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_AUTOSTART, _T("允许开机自启动"));

	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_EXIT, _T("退出"));
}


void CpermoDlg::OpenConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	int left, top, topmost, skin, autohide, trans, showway, shownetinfo, fontsize;
	left = ::GetPrivateProfileInt(_T("Main"), _T("left"), -1, temp);
	top = ::GetPrivateProfileInt(_T("Main"), _T("top"), -1, temp);
	topmost = ::GetPrivateProfileInt(_T("Main"), _T("topmost"), -1, temp);
	skin = ::GetPrivateProfileInt(_T("Main"), _T("skin"), -1, temp);
	autohide = ::GetPrivateProfileInt(_T("Main"), _T("autohide"), -1, temp);
	trans = ::GetPrivateProfileInt(_T("Main"), _T("trans"), -1, temp);
	showway = ::GetPrivateProfileInt(_T("Main"), _T("showway"), -1, temp);
	shownetinfo = ::GetPrivateProfileInt(_T("Main"), _T("shownetinfo"), -1, temp);
	fontsize = ::GetPrivateProfileInt(_T("Main"), _T("fontsize"), -1, temp);
	//读取到的且正确的数据进行赋值，负责采用默认数值
	if (-1 == left || left < 0)
	{
		rCurPos.left = -1;
	}
	else
	{
		rCurPos.left = left;
	}
	if (-1 == top || top < 0)
	{
		rCurPos.top = -1;
	}
	else
	{
		rCurPos.top = top;
	}
	if (-1 == topmost || (topmost != 0 && topmost != 1))
	{
		bTopmost = FALSE;
	}
	else
	{
		bTopmost = topmost;
	}
	if (skin != IDM_GREEN && skin != IDM_BLUE && skin != IDM_BLACK
		&& skin != IDM_RED && skin != IDM_ORANGE)
	{
		nSkin = IDM_GREEN;
	}
	else
	{
		nSkin = skin;
	}
	if (-1 == autohide || (autohide != 0 && autohide != 1))
	{
		bAutoHide = FALSE;
	}
	else
	{
		bAutoHide = autohide;
	}
	if (trans < 0 || trans > 255)
	{
		nTrans = 255;
	}
	else
	{
		nTrans = trans;
	}
	if (-1 == showway || showway > 1)
	{
		nShowWay = 0;
	}
	else
	{
		nShowWay = showway;
	}
	if (-1 == shownetinfo || (shownetinfo != 0 && shownetinfo != 1))
	{
		bShowNetInfo = FALSE;
	}
	else
	{
		bShowNetInfo = shownetinfo;
	}
	if (-1 == fontsize || fontsize < 12 || fontsize > 18)
	{
		nFontSize = 18;
	}
	else
	{
		nFontSize = fontsize;
	}
}


BOOL CpermoDlg::SaveConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	TCHAR cLeft[32], cTop[32], cTopMost[32], cSkin[32], cAutoHide[32], 
		cTrans[32], cShowWay[32], cShowNetInfo[32], cFontSize[32];
	_itow_s(rCurPos.left, cLeft, 10);
	_itow_s(rCurPos.top, cTop, 10);
	_itow_s(bTopmost, cTopMost, 10);
	_itow_s(nSkin, cSkin, 10);
	_itow_s(bAutoHide, cAutoHide, 10);
	_itow_s(nTrans, cTrans, 10);
	_itow_s(nShowWay, cShowWay, 10);
	_itow_s(bShowNetInfo, cShowNetInfo, 10);
	_itow_s(nFontSize, cFontSize, 10);
	::WritePrivateProfileString(_T("Main"), _T("left"), cLeft, temp);
	::WritePrivateProfileString(_T("Main"), _T("top"), cTop, temp);
	::WritePrivateProfileString(_T("Main"), _T("topmost"), cTopMost, temp);
	::WritePrivateProfileString(_T("Main"), _T("skin"), cSkin, temp);
	::WritePrivateProfileString(_T("Main"), _T("autohide"), cAutoHide, temp);
	::WritePrivateProfileString(_T("Main"), _T("trans"), cTrans, temp);
	::WritePrivateProfileString(_T("Main"), _T("showway"), cShowWay, temp);
	::WritePrivateProfileString(_T("Main"), _T("shownetinfo"), cShowNetInfo, temp);
	::WritePrivateProfileString(_T("Main"), _T("fontsize"), cFontSize, temp);
	return TRUE;
}
//LRESULT CpermoDlg::OnNcHitTest(CPoint point)
//{
//	// TODO: 在此添加消息处理程序代码和/或调用默认值
//	//if (bAutoHide)
//	{
//		TRACKMOUSEEVENT tme;
//		tme.cbSize = sizeof(tme);
//		tme.dwFlags = TME_HOVER | TME_LEAVE; //注册非客户区离开
//		tme.hwndTrack = m_hWnd;
//		tme.dwHoverTime = HOVER_DEFAULT; //只对HOVER有效  
//		::TrackMouseEvent(&tme);
//	}
//	return CDialog::OnNcHitTest(point);
//}

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

void CpermoDlg::OnTrans0(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS0, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 255;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans10(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS10, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 230;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans20(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS20, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 205;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans30(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS30, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 180;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans40(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS40, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 155;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans50(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS50, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 130;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans60(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS60, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 105;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans70(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS70, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 80;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans80(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS80, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 55;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

void CpermoDlg::OnTrans90(void)
{
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_TRANS90, MF_BYCOMMAND | MF_CHECKED);
	nTrans = 30;
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
}

//设置开机自启动
//先查找，找到就删除，找不到就创建
void CpermoDlg::SetAutoRun(void)
{
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");//找到系统的启动项
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, _T("permo"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			RegDeleteValue (hKey,_T("permo"));   
			m_Menu.CheckMenuItem(IDM_AUTOSTART, MF_BYCOMMAND | MF_UNCHECKED);
		}
		else
		{
			TCHAR szModule[_MAX_PATH];
			GetModuleFileName(NULL, szModule, _MAX_PATH);//得到本程序自身的全路径
			RegSetValueEx(hKey,_T("permo"), 0, REG_SZ, (LPBYTE)szModule, wcslen(szModule)*sizeof(TCHAR)); //添加一个子Key,并设置值
			m_Menu.CheckMenuItem(IDM_AUTOSTART, MF_BYCOMMAND | MF_CHECKED);
		}
		RegCloseKey(hKey); //关闭注册表
	}
	else
	{
		AfxMessageBox(_T("无法设置！"));   
	}
}

void CpermoDlg::IfAutoRun(void)
{
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");//找到系统的启动项
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, _T("permo"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			m_Menu.CheckMenuItem(IDM_AUTOSTART, MF_BYCOMMAND | MF_CHECKED);
		}
		RegCloseKey(hKey); //关闭注册表
	}
}

void CpermoDlg::CreateInfoDlg(void)
{
	pInfoDlg = new CInfoDlg(this);
	pInfoDlg->Create(IDD_INFO_DIALOG, this);
	if (NULL == pInfoDlg)
	{
		return;
	}
	//默认不显示
	MoveInfoDlg();
	//pInfoDlg->ShowWindow(SW_SHOW);
}

//将详细信息窗口移动到适当的位置
void CpermoDlg::MoveInfoDlg(void)
{
	CRect rect = rCurPos;
	rect.left -= 10;
	rect.right += 10;
	if (rect.left < 0)
	{
		rect.left = 0;
		rect.right = rect.left + 240;
	}
	else if (rect.right > rWorkArea.right)
	{
		rect.right = rWorkArea.right;
		rect.left = rect.right - 240;
	}
	if (rCurPos.top < 365)
	{
		rect.top = rCurPos.bottom + 5;
		rect.bottom = rect.top + 360;
	}
	else
	{
		rect.bottom = rCurPos.top - 5;
		rect.top = rect.bottom - 360;
	}
	
	pInfoDlg->MoveWindow(&rect,TRUE);
}

//修正工作目录，防止开机自启读取配置出错
BOOL CpermoDlg::SetWorkDir(void)
{
	TCHAR szPath[MAX_PATH]; 
	if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
	{
		//printf("GetModuleFileName failed (%d)\n", GetLastError()); 
		return FALSE;
	}
	PathRemoveFileSpec(szPath);
	::SetCurrentDirectory(szPath);
	return TRUE;
}

void CpermoDlg::SetShowWay(void)
{
	if (0 == nShowWay)
	{
		nShowWay = 1;
		m_Menu.CheckMenuItem(IDM_SHOWBYLDOWN, MF_BYCOMMAND | MF_CHECKED);
		m_Menu.CheckMenuItem(IDM_SHOWBYHOVER, MF_BYCOMMAND | MF_UNCHECKED);
	}
	else
	{
		nShowWay = 0;
		m_Menu.CheckMenuItem(IDM_SHOWBYHOVER, MF_BYCOMMAND | MF_CHECKED);
		m_Menu.CheckMenuItem(IDM_SHOWBYLDOWN, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

void CpermoDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (1 == nShowWay)
	{
		MoveInfoDlg();
		pInfoDlg->SetTimer(1, 1000, NULL);
		pInfoDlg->ShowWindow(SW_SHOW);
	}
	ReleaseCapture();
	CDialog::OnLButtonUp(nFlags, point);
}

//void CpermoDlg::OnNcLButtonUp(UINT nHitTest, CPoint point)
//{
//	// TODO: 在此添加消息处理程序代码和/或调用默认值
//	if (1 == nShowWay)
//	{
//		MoveInfoDlg();
//		pInfoDlg->ShowWindow(SW_SHOW);
//	}
//	CDialog::OnNcLButtonUp(nHitTest, point);
//}

void CpermoDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (_bMouseTrack)    //若允许追踪，则。
	{
		TRACKMOUSEEVENT csTME;
		csTME.cbSize = sizeof(csTME);
		csTME.dwFlags = TME_LEAVE|TME_HOVER;
		csTME.hwndTrack = m_hWnd;//指定要追踪的窗口
		csTME.dwHoverTime = 10;  //鼠标在按钮上停留超过10ms，才认为状态为HOVER
		::_TrackMouseEvent(&csTME); //开启Windows的WM_MOUSELEAVE，WM_MOUSEHOVER事件支持
		_bMouseTrack=FALSE;   //若已经追踪，则停止追踪
	}

	static CPoint PrePoint = CPoint(0, 0);  
	if(MK_LBUTTON == nFlags)  
	{  
		if(point != PrePoint)  
		{  
			CPoint ptTemp = point - PrePoint;  
			CRect rcWindow;  
			GetWindowRect(&rcWindow);  
			rcWindow.OffsetRect(ptTemp.x, ptTemp.y);  
			if (rcWindow.bottom <= rWorkArea.bottom && rcWindow.left >= 0 && 
				rcWindow.right <= rWorkArea.right && rcWindow.top >= 0)
			{
				MoveWindow(&rcWindow); 
				rCurPos = rcWindow;
				MoveInfoDlg();
			}
			return ;  
			
		}  
	}  
	PrePoint = point; 
	CDialog::OnMouseMove(nFlags, point);
}


void CpermoDlg::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (bAutoHide)
	{
		if (rCurPos.right == rWorkArea.right + 185)
		{
			rCurPos.right = rWorkArea.right;
			rCurPos.left = rCurPos.right - 220;
			MoveWindow(&rCurPos, TRUE);
		}
		if (rCurPos.left == rWorkArea.left - 185)
		{
			rCurPos.left = rWorkArea.left;
			rCurPos.right = rCurPos.left + 220;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	{
		//pInfoDlg->SetTimer(1, 1000, NULL);
		if (0 == nShowWay)
		{
			pInfoDlg->SetTimer(1, 1000, NULL);
			pInfoDlg->ShowWindow(SW_SHOW);
		}
	}
	CDialog::OnMouseHover(nFlags, point);
}

void CpermoDlg::OnMouseLeave()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	_bMouseTrack=TRUE; 
	if (bAutoHide)
	{
		if (rCurPos.right >= rWorkArea.right-5)
		{
			rCurPos.right = rWorkArea.right + 185;
			rCurPos.left = rCurPos.right - 220;
			MoveWindow(&rCurPos, TRUE);
		}
		if (rCurPos.left <= rWorkArea.left+5)
		{
			rCurPos.left = rWorkArea.left - 185;
			rCurPos.right = rCurPos.left + 220;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	{
		pInfoDlg->KillTimer(1);
		pInfoDlg->ShowWindow(SW_HIDE);
	}
	CDialog::OnMouseLeave();
}

BOOL CpermoDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	return TRUE;
}

void CpermoDlg::ShowNetInfo(void)
{
	if (bShowNetInfo)
	{
		bShowNetInfo = false;
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_UNCHECKED);
	}
	else
	{
		bShowNetInfo = true;
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_CHECKED);
	}
}

void CpermoDlg::SetFontSize(UINT fontSize)
{
	nFontSize = fontSize;
	m_Menu.CheckMenuItem(IDM_FONTSIZE12, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE13, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE14, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE15, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE16, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE17, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE18, MF_BYCOMMAND | MF_UNCHECKED);
	m_Menu.CheckMenuItem(IDM_FONTSIZE12 + nFontSize - 12, MF_BYCOMMAND | MF_CHECKED);
	Invalidate(FALSE);
}
