// InfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "permo.h"
#include "InfoDlg.h"

extern int nTempDisk;		//硬盘温度
extern int nTempCpu;
extern BOOL gIsMsr;
extern unsigned int nSkin;

extern vector<CProInfo*> vecProInfo;

extern vector<CProInfo*> vecCpu;
extern vector<CProInfo*> vecMem;
extern vector<CProInfo*> vecNet;

extern bool bShowNetInfo;
extern unsigned int nFontSize;
extern int processor_count_;
DWORD id;
// CInfoDlg 对话框

bool comT(CProInfo* pProInfo)
{
	if (pProInfo->id == id)
	{
		return true;
	}
	return false;
}

//自定义排序函数
//注意：本函数的参数的类型一定要与vector中元素的类型一致
bool SortByCpu(const CProInfo * v1, const CProInfo * v2)
{
	return v1->cpu > v2->cpu;//降序排列
}

bool SortByMem(const CProInfo * v1, const CProInfo * v2)
{
	return v1->mem > v2->mem;//降序排列
}

bool SortByNet(const CProInfo * v1, const CProInfo * v2)
{
	return (v1->prevTxRate + v1->prevRxRate) > (v2->prevTxRate + v2->prevRxRate);//降序排列
}

IMPLEMENT_DYNAMIC(CInfoDlg, CDialog)

CInfoDlg::CInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInfoDlg::IDD, pParent)
	, m_pParent(NULL)
	, nProNum(0)
{
	m_pParent = pParent;
}

CInfoDlg::~CInfoDlg()
{
	FreeVec();
}

void CInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInfoDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CInfoDlg 消息处理程序

void CInfoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (1 == nIDEvent)
	{
		//进行清理内存操作
		vecCpu.clear();
		vecMem.clear();
		vecNet.clear();
		GetProInfoVec();
		Invalidate(FALSE);
	}

	CDialog::OnTimer(nIDEvent);
}

void CInfoDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialog::OnPaint()
	RECT rcClient;
	this->GetClientRect(&rcClient);
	CDC MemDC;
	CBitmap bitmap;
	BITMAP  m_bitmap;
	bitmap.LoadBitmap(IDB_PROINFO_BACK);
	bitmap.GetBitmap(&m_bitmap);
	MemDC.CreateCompatibleDC(&dc);
	MemDC.SelectObject(&bitmap);
	DrawInfo(&MemDC);
	dc.BitBlt(0, 0, m_bitmap.bmWidth,m_bitmap.bmHeight, &MemDC,
		0, 0, SRCCOPY);
	bitmap.DeleteObject();
	MemDC.DeleteDC();
}

BOOL CInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	nTempDisk = 0;
	//提升权限
	DebugPrivilege(true);

	GetProInfoVec();
	SetTimer(1, 1100, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CInfoDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	CDialog::OnLButtonDown(nFlags, point);
}


void CInfoDlg::PostNcDestroy()
{
	// TODO: 在此添加专用代码和/或调用基类

	CDialog::PostNcDestroy();
	delete this;
	CoUninitialize();
	//权限还原
	DebugPrivilege(false);
}

void CInfoDlg::OnCancel()
{
	// TODO: 在此添加专用代码和/或调用基类
//非模态对话框需要重载函数OnCanel，并且在这个函数中调用DestroyWindow。并且不能调用基类的OnCancel，因为基类的OnCancel调用了EndDialog这个函数，这个函数是针对模态对话框的。 如果OnOk需要关闭对话框也要进行重载，方法类似。
	DestroyWindow();
}

void CInfoDlg::DrawInfo(CDC * pDC)
{
// 	CRect rcIcon;
// 	rcIcon.left = 0;
// 	rcIcon.top = 0;
// 	rcIcon.right = 20;
// 	rcIcon.bottom = 20;
// 	rcIcon.DeflateRect(2, 2, 2, 2);
	//DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
	int i=0;
	CFont font, *pOldFont;
	LOGFONT logFont;
	pDC->GetCurrentFont()->GetLogFont(&logFont);
	logFont.lfWidth = 0;
	logFont.lfHeight = nFontSize;
	logFont.lfWeight = 0;
	lstrcpy(logFont.lfFaceName, _T("微软雅黑"));
	font.CreateFontIndirect(&logFont);
	pOldFont = pDC->SelectObject(&font);
	COLORREF cOldTextColor;
	cOldTextColor = pDC->SetTextColor(RGB(255, 255, 255));
	int nOldBkMode = pDC->SetBkMode(TRANSPARENT);
	//绘制悬浮窗口标题
	CString strTemp = _T("详细信息悬浮窗");
	CRect rcLeftText, rcRightText, rcTitleText;
	rcTitleText.left = 25;
	rcTitleText.right = 155;
	rcTitleText.top = 3;
	rcTitleText.bottom = 20;
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	pDC->SetTextColor(RGB(0, 0, 0));
	//绘制标题下方的四个小内容
	//MessageBox(strTemp);
	rcTitleText.top = 30;
	rcTitleText.bottom = 50;

	strTemp.Format(_T("进程数量: %d"), nProNum);
	
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	rcTitleText.left = 155 + 10;
	rcTitleText.right = 230;
	
	if (gIsMsr)
	{
		strTemp.Format(_T("CPU: %d℃"), nTempCpu);
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		strTemp = _T("预留");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	}
	
	rcTitleText.left = 25;
	rcTitleText.right = 155;
	rcTitleText.top = 50;
	rcTitleText.bottom = 70;

	strTemp.Format(_T("硬盘温度: %d℃"), nTempDisk);
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	rcTitleText.left = 155 + 10;
	rcTitleText.right = 230;
	strTemp = _T("预留");
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	//列表两个小标题
	rcTitleText.left = 5;
	rcTitleText.right = 155;
	rcTitleText.top = 80;
	rcTitleText.bottom = 100;
	strTemp = _T("程序名称");
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	rcTitleText.left = 155 + 10;
	rcTitleText.right = 240;
	strTemp = _T("资源占用");
	pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	//绘制网络占用进程信息
 	CRect	rcIcon;
 	rcIcon.left = 6;
 	rcIcon.top = 124;
 	rcIcon.bottom = 140;
 	rcIcon.right = 22;
// 	DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
// 	rcLeftText.top = 122;
// 	rcLeftText.bottom = 142;
// 	rcLeftText.left = 26;
// 	rcLeftText.right = 160;
// 	strTemp = _T("explorer.exe");
// 	pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
// 	
// 	rcIcon.top = 144;
// 	rcIcon.bottom = 160;
// 	DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
// 	rcLeftText.top = 142;
// 	rcLeftText.bottom = 162;
// 	strTemp = _T("thunderplatform.exe");
// 	pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
// 	
// 	rcIcon.top = 164;
// 	rcIcon.bottom = 180;
// 	DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
// 	rcLeftText.top = 162;
// 	rcLeftText.bottom = 182;
// 	strTemp = _T("QQ.exe");
// 	pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

	//绘制CPU占用进程信息
	if (bShowNetInfo)
	{
		//首先绘制三个标题
		rcTitleText.left = 5;
		rcTitleText.right = 200;
		rcTitleText.top = 100;
		rcTitleText.bottom = 120;

		strTemp = _T("当前最占网络的程序");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		rcTitleText.top += 20*4;
		rcTitleText.bottom = rcTitleText.top + 20;
		strTemp = _T("当前最占CPU的程序");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		rcTitleText.top += 20*4;
		rcTitleText.bottom = rcTitleText.top + 20;
		strTemp = _T("当前最占内存的程序");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//接着绘制每块具体内容
		pDC->SetTextColor(RGB(80, 80, 80));

		rcLeftText.left = 30;
		rcLeftText.right = 155;
		rcLeftText.top = 120;
		rcLeftText.bottom = 140;
		for (i=0; i<3; i++)
		{
			rcIcon.top = rcLeftText.top + 2;
			rcIcon.bottom = rcLeftText.bottom -2;
			if (vecNet[i]->hIcon)
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, vecNet[i]->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			else
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			strTemp.Format(_T("%s"), vecNet[i]->szExeFile);
			pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcRightText = rcLeftText;
			rcRightText.left = 165;
			rcRightText.right = 240;
			double net;
			net = (vecNet[i]->prevRxRate + vecNet[i]->prevTxRate) / 1024.0;
			if (net > 1000)
			{
				strTemp.Format(_T("%.1fM/s"), net/1024);
			}
			else
			{
				strTemp.Format(_T("%.1fK/s"), net);
			}
			pDC->DrawText(strTemp, &rcRightText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcLeftText.top += 20;
			rcLeftText.bottom += 20;
		}

		rcLeftText.left = 30;
		rcLeftText.right = 155;
		rcLeftText.top = 200;
		rcLeftText.bottom = 220;

		for(i=0;i<3;i++)
		{
			rcIcon.top = rcLeftText.top + 2;
			rcIcon.bottom = rcLeftText.bottom -2;
			if (vecCpu[i]->hIcon)
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, vecCpu[i]->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			else
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			strTemp.Format(_T("%s"), vecCpu[i]->szExeFile);
			pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcRightText = rcLeftText;
			rcRightText.left = 165;
			rcRightText.right = 240;
			strTemp.Format(_T("%d%%"), vecCpu[i]->cpu);
			pDC->DrawText(strTemp, &rcRightText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcLeftText.top += 20;
			rcLeftText.bottom += 20;
		}
		rcLeftText.top += 20;
		rcLeftText.bottom += 20;
		//绘制内存占用进程信息
		for(i=0; i<3; i++)
		{
			rcIcon.top = rcLeftText.top + 2;
			rcIcon.bottom = rcLeftText.bottom -2;
			if (vecMem[i]->hIcon)
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, vecMem[i]->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			else
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			strTemp.Format(_T("%s"), vecMem[i]->szExeFile);
			pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcRightText = rcLeftText;
			rcRightText.left = 165;
			rcRightText.right = 240;
			if (vecMem[i]->mem < 1000)
			{
				strTemp.Format(_T("%.1fMB"), vecMem[i]->mem);
			}
			else
			{
				strTemp.Format(_T("%.1fGB"), vecMem[i]->mem / 1024.0);
			}
			pDC->DrawText(strTemp, &rcRightText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcLeftText.top += 20;
			rcLeftText.bottom += 20;
		}
	}
	else
	{
		rcTitleText.left = 5;
		rcTitleText.right = 200;
		rcTitleText.top = 100;
		rcTitleText.bottom = 120;

		strTemp = _T("当前最占CPU的程序");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		rcTitleText.top += 20*6;
		rcTitleText.bottom = rcTitleText.top + 20;
		strTemp = _T("当前最占内存的程序");
		pDC->DrawText(strTemp, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//接着绘制每块具体内容
		pDC->SetTextColor(RGB(80, 80, 80));

		rcLeftText.left = 30;
		rcLeftText.right = 155;
		rcLeftText.top = 120;
		rcLeftText.bottom = 140;
		//绘制CPU占用信息
		vector<CProInfo*>::iterator iter;
		for(iter=vecCpu.begin();iter!=vecCpu.end();iter++)
		{
			rcIcon.top = rcLeftText.top + 2;
			rcIcon.bottom = rcLeftText.bottom -2;
			if ((*iter)->hIcon)
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, (*iter)->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			else
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			strTemp.Format(_T("%s"), (*iter)->szExeFile);
			pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcRightText = rcLeftText;
			rcRightText.left = 165;
			rcRightText.right = 240;
			strTemp.Format(_T("%d%%"), (*iter)->cpu);
			pDC->DrawText(strTemp, &rcRightText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcLeftText.top += 20;
			rcLeftText.bottom += 20;
		}
		rcLeftText.top += 20;
		rcLeftText.bottom += 20;
		//绘制内存占用进程信息
		vector<CProInfo*>::iterator iter2;
		for(iter2=vecMem.begin();iter2!=vecMem.end();iter2++)
		{
			rcIcon.top = rcLeftText.top + 2;
			rcIcon.bottom = rcLeftText.bottom -2;
			if ((*iter2)->hIcon)
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, (*iter2)->hIcon, rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			else
			{
				DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PROCESS)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
			}
			strTemp.Format(_T("%s"), (*iter2)->szExeFile);
			pDC->DrawText(strTemp, &rcLeftText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcRightText = rcLeftText;
			rcRightText.left = 165;
			rcRightText.right = 240;
			if ((*iter2)->mem < 1000)
			{
				strTemp.Format(_T("%.1fMB"), (*iter2)->mem);
			}
			else
			{
				strTemp.Format(_T("%.1fGB"), (*iter2)->mem / 1024.0);
			}
			pDC->DrawText(strTemp, &rcRightText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rcLeftText.top += 20;
			rcLeftText.bottom += 20;
		}
	}
	
	pDC->SetTextColor(cOldTextColor);
	pDC->SetBkMode(nOldBkMode);
	pDC->SelectObject(pOldFont);
	font.DeleteObject();
}

// 提升权限
bool CInfoDlg::DebugPrivilege(bool bEnable)
{
	bool              bResult = true;
	HANDLE            hToken;
	TOKEN_PRIVILEGES  TokenPrivileges;

	if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&hToken) == 0)
	{
		//printf("OpenProcessToken Error: %d\n",GetLastError());
		bResult = FALSE;
	}
	TokenPrivileges.PrivilegeCount           = 1;
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&TokenPrivileges.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken,FALSE,&TokenPrivileges,sizeof(TOKEN_PRIVILEGES),NULL,NULL);
	if(GetLastError() != ERROR_SUCCESS)
	{
		bResult = false;
	}
	CloseHandle(hToken);

	return bResult;
}

//时间转换
uint64_t CInfoDlg::file_time_2_utc(const FILETIME* ftime)
{
	LARGE_INTEGER li;  

	li.LowPart = ftime->dwLowDateTime;  
	li.HighPart = ftime->dwHighDateTime;  
	return li.QuadPart;
}

double CInfoDlg::get_cpu_usage(HANDLE hProcess, CProInfo* pProInfo)
{
	FILETIME now;  
	FILETIME creation_time;  
	FILETIME exit_time;  
	FILETIME kernel_time;  
	FILETIME user_time;  
	int64_t system_time;  
	int64_t time;  
	int64_t system_time_delta;  
	int64_t time_delta;  

	int cpu = 0;  

	GetSystemTimeAsFileTime(&now);  

	if (!GetProcessTimes(hProcess, &creation_time, &exit_time,  
		&kernel_time, &user_time))  
	{  
		// We don't assert here because in some cases (such as in the Task   

		//Manager)  
		// we may call this function on a process that has just exited but   

		//we have  
		// not yet received the notification.  
		return 0;  
	}  
	system_time = (file_time_2_utc(&kernel_time) + file_time_2_utc(&user_time))  /  processor_count_;  
	time = file_time_2_utc(&now);  

	if ((pProInfo->last_system_time_ == 0) || (pProInfo->last_time_ == 0))  
	{  
		// First call, just set the last values.  
		pProInfo->last_system_time_ = system_time;  
		pProInfo->last_time_ = time;  
		pProInfo->cpu = 0;
		return 0;  
	}  

	system_time_delta = system_time - pProInfo->last_system_time_;  
	time_delta = time - pProInfo->last_time_;  

	if (time_delta == 0)  
	{
		pProInfo->cpu = 0;
		return 0;  
	}

	// We add time_delta / 2 so the result is rounded.  
	cpu = (int)((system_time_delta * 100 + time_delta / 2) / time_delta);  
	pProInfo->last_system_time_ = system_time;  
	pProInfo->last_time_ = time;  
	pProInfo->cpu = cpu;
	return cpu; 
}

//获取进程信息并存入vector
void CInfoDlg::GetProInfoVec(void)
{
	//WinExec("cmd /c tasklist /v >d:\\tasklist.txt",SW_HIDE);
	PROCESSENTRY32 pe32;
	//设置大小
	pe32.dwSize=sizeof(pe32);
	//拍摄系统进程快照
	HANDLE hProcessSnap=::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if(hProcessSnap==INVALID_HANDLE_VALUE)
	{
		//获取系统进程快照失败
		return;
	}
	nProNum = 0;
	//遍历快照
	BOOL bMORE=::Process32First(hProcessSnap,&pe32);
	while(bMORE)
	{
		nProNum++;
		id = pe32.th32ProcessID;
		//取得进程的内存使用信息
		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE, pe32.th32ProcessID);
		if (NULL == hProcess)
		{
			//System和System idle进程一般会打开失败，暂时忽略这几个系统进程
			bMORE=::Process32Next(hProcessSnap,&pe32);
			continue;
		}
		else
		{
			PROCESS_MEMORY_COUNTERS processMemCounters ;
			if( ::GetProcessMemoryInfo (hProcess, &processMemCounters, sizeof(processMemCounters) ) )
			{
				double memtmp = processMemCounters.WorkingSetSize/(1024.0*1024.0);

				vector<CProInfo*>::iterator iter = find_if(vecProInfo.begin(),vecProInfo.end(),&comT);
				if(iter!=vecProInfo.end())
				{
					//找到了，即需要更新的进程
					get_cpu_usage(hProcess, (*iter));
					(*iter)->mem = memtmp;
					(*iter)->bExit = FALSE;
					//当前进程的内存使用量单位是byte，该结构体的其他成员的作用请参考MSDN
				}
				else
				{
					//找不到，即需要新增的进程
					CProInfo *p = new CProInfo();
					p->id = pe32.th32ProcessID;
					p->mem = memtmp;
					p->bExit = FALSE;
					wcscpy(p->szExeFile, pe32.szExeFile);
					GetProcessFullPath(hProcess, p->szExeFilePath);
					if (!p->hIcon)
					{
						ExtractIconEx(p->szExeFilePath, 0, NULL, &(p->hIcon), 1);
					}
					//p->szExeFile = pe32.szExeFile;
					vecProInfo.push_back(p);
				}
			}
			else
			{
				//GetProcessMemoryInfo failed.
				GetLastError();
			}
		}
		bMORE=::Process32Next(hProcessSnap,&pe32);
	}
	//去掉已经退出的进程，并置bExit为TRUE
	RemoveExitPro();
	//已经获取到所有进程信息
	//按照cpu占用进行排序（降序）
	sort(vecProInfo.begin(),vecProInfo.end(),SortByCpu);
	vecCpu.push_back(vecProInfo[0]);
	vecCpu.push_back(vecProInfo[1]);
	vecCpu.push_back(vecProInfo[2]);
	vecCpu.push_back(vecProInfo[3]);
	vecCpu.push_back(vecProInfo[4]);
	//按内存排序（降序）
	sort(vecProInfo.begin(),vecProInfo.end(),SortByMem);
	vecMem.push_back(vecProInfo[0]);
	vecMem.push_back(vecProInfo[1]);
	vecMem.push_back(vecProInfo[2]);
	vecMem.push_back(vecProInfo[3]);
	vecMem.push_back(vecProInfo[4]);
	//按网络排序（降序）
	sort(vecProInfo.begin(),vecProInfo.end(),SortByNet);
	vecNet.push_back(vecProInfo[0]);
	vecNet.push_back(vecProInfo[1]);
	vecNet.push_back(vecProInfo[2]);
	vecNet.push_back(vecProInfo[3]);
	vecNet.push_back(vecProInfo[4]);
	//vector<CProInfo*>::iterator iter;
	//for(iter=vecProInfo.begin();iter!=vecProInfo.end();iter++)
	//{
	//	printf("进程名：%s ID:%d MEM:%.1f CPU:%d\n", (*iter)->szExeFile, (*iter)->id, (*iter)->mem, (*iter)->cpu);
	//}  
	CloseHandle(hProcessSnap);
}

// 清理内存
void CInfoDlg::FreeVec(void)
{
	//进行清理内存操作
	vector<CProInfo*>::iterator iter;
	for(iter=vecProInfo.begin();iter!=vecProInfo.end();iter++)
	{  
		if ((*iter) != NULL)
		{
			delete *iter;  
		}
	}
}

//删除已经退出的进程
void CInfoDlg::RemoveExitPro(void)
{
	vector<CProInfo*>::iterator iter;
	for(iter=vecProInfo.begin();iter!=vecProInfo.end();)
	{  
		if ((*iter)->bExit)
		{
			delete *iter;
			iter = vecProInfo.erase(iter);
		}
		else
		{
			(*iter)->bExit = TRUE;
			iter++;
		}
	}
}

// 获取进程完整路径 
BOOL CInfoDlg::GetProcessFullPath(HANDLE hProcess, TCHAR * pszFullPath)
{
	TCHAR       szImagePath[MAX_PATH];  

	if(!pszFullPath)  
		return FALSE;  

	pszFullPath[0] = '\0';  
	if(!hProcess)  
		return FALSE;  

	if(!GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))  
	{  
		CloseHandle(hProcess);  
		return FALSE;  
	}  

	if(!DosPathToNtPath(szImagePath, pszFullPath))  
	{  
		CloseHandle(hProcess);  
		return FALSE;  
	}  

	CloseHandle(hProcess);  

	return TRUE; 
}

BOOL CInfoDlg::DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	TCHAR           szDriveStr[500];  
	TCHAR           szDrive[3];  
	TCHAR           szDevName[100];  
	INT             cchDevName;  
	INT             i;  

	//检查参数  
	if(!pszDosPath || !pszNtPath )  
		return FALSE;  

	//获取本地磁盘字符串  
	if(GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))  
	{  
		for(i = 0; szDriveStr[i]; i += 4)  
		{  
			if(!lstrcmpi(&(szDriveStr[i]), TEXT("A:\\")) || !lstrcmpi(&(szDriveStr[i]), TEXT("B:\\")))  
				continue;  

			szDrive[0] = szDriveStr[i];  
			szDrive[1] = szDriveStr[i + 1];  
			szDrive[2] = '\0';  
			if(!QueryDosDevice(szDrive, szDevName, 100))//查询 Dos 设备名  
				return FALSE;  

			cchDevName = lstrlen(szDevName);  
			if(_tcsnicmp(pszDosPath, szDevName, cchDevName) == 0)//命中  
			{  
				lstrcpy(pszNtPath, szDrive);//复制驱动器  
				lstrcat(pszNtPath, pszDosPath + cchDevName);//复制路径 

				return TRUE;  
			}             
		}  
	}  

	lstrcpy(pszNtPath, pszDosPath);  

	return FALSE;
}

BOOL CInfoDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	return TRUE;
}