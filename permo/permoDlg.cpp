// permoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "permo.h"
#include "permoDlg.h"

#include <shlwapi.h>

#include "utils/PcapNetFilter.h"
#include "utils/Utils.h"
#include "utils/PortCache.h"

#include <Windows.h>
#include <Winsvc.h>
#include <WinIoCtl.h>

#define  MYNPF _T("NPF")
#define  MYWINRIN0 _T("MyWinRing0")
#define OLS_TYPE 40000
#define IOCTL_OLS_READ_MSR \
	CTL_CODE(OLS_TYPE, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)

HANDLE gHandle = INVALID_HANDLE_VALUE;
HANDLE gHandle2 = INVALID_HANDLE_VALUE;
TCHAR gDriverPath[MAX_PATH];
TCHAR gDriverPath2[MAX_PATH];
BOOL gIsMsr = FALSE;

void LoadDriver();
BOOL StopDriver(SC_HANDLE hSCManager,LPCTSTR DriverId);
BOOL RemoveDriver(SC_HANDLE hSCManager, LPCTSTR DriverId);
BOOL IsFileExist(LPCTSTR fileName);
BOOL Initialize(int driveId);
void LoadDriver(int driveId);
BOOL OpenDriver(int driveId);
void Remove();


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CpermoDlg *pThis;

unsigned int nSkin;			//皮肤编号

int nTempDisk=0;		//硬盘温度
//明细窗口显示2部分(true)或者3部分(false)
bool bShowNetInfo;
int nTempCpu=0;		//cpu温度
//bool bIsWindowsVistaOrGreater;
unsigned int nFontSize;

int processor_count_ = -1;

//任务栏区域显示的内容：0-cpu 1-内存 2-下载 3-上传
unsigned int nBandShow;
//标尺右键菜单
CMenu              m_BandMenu;
CMenu              m_BandFontSizeMenu;
CMenu              m_BandWidthMenu;
CMenu              m_BandHeightMenu;


// Capture thread
HANDLE g_hCaptureThread;
bool   g_bCapture = false;

// Adapter
int    g_nAdapters = 0;
int    g_iAdapter;
TCHAR  g_szAdapterNames[16][256];
static CRITICAL_SECTION _cs;

vector<CProInfo*> vecProInfo;

vector<CProInfo*> vecCpu;
vector<CProInfo*> vecMem;
vector<CProInfo*> vecNet;

void Lock()
{
	EnterCriticalSection(&_cs);
}

void Unlock()
{
	LeaveCriticalSection(&_cs);
}

int GetProcessIndex(int pid)
{
	int index = -1;
	for (int i=0; i<vecProInfo.size(); i++)
	{
		if (pid == vecProInfo[i]->id)
		{
			return i;
		}
	}
	return index;
}

void OnPacket(PacketInfoEx *pi)
{
	int index = GetProcessIndex(pi->pid);

	if( index == -1 ) // A new process
	{
// 		// Insert a ProcessItem
// 		Lock();
// 		proinfo item;
// 
// 		RtlZeroMemory(&item, sizeof(item));
// 
// 		item.active = true;
// 		item.dirty = false;
// 		item.pid = pi->pid; // The first pid is logged
// 		item.puid = pi->puid;
// 
// 
// 		//item.hidden = false;
// 
// 		item.txRate = 0;
// 		item.rxRate = 0;
// 		item.prevTxRate = 0;
// 		item.prevRxRate = 0;
// 
// 		// Add to process list
// 		vProInfo.push_back(item);
// 		Unlock();
		
	}
	else
	{
		Lock();

		// Update the ProcessItem that already Exists
		CProInfo *item = vecProInfo[index];

		if( !item->active )
		{
			item->active = true;
			item->pid = pi->pid; // The first pid is logged

			//_tcscpy_s(item.fullPath, MAX_PATH, pi->fullPath);

			item->txRate = 0;
			item->rxRate = 0;
			item->prevTxRate = 0;
			item->prevRxRate = 0;
		}

		if( pi->dir == DIR_UP )
		{
			item->txRate += pi->size;
		}
		else if( pi->dir == DIR_DOWN )
		{
			item->rxRate += pi->size;
		}
		item->dirty = true;

		Unlock();
	}
}
static DWORD WINAPI CaptureThread(LPVOID lpParam)
{
	PcapNetFilter filter;
	PacketInfo pi;
	PacketInfoEx pie;

	PortCache pc;

	// Init Filter ------------------------------------------------------------
	if( !filter.Init())
	{
		return 1;
	}

	// Find Devices -----------------------------------------------------------
	if( !filter.FindDevices())
	{
		return 2;
	}

	// Select a Device --------------------------------------------------------
	if( !filter.Select(g_iAdapter))
	{
		return 3;
	}

	// Capture Packets --------------------------------------------------------
	while( g_bCapture )
	{
		int pid = -1;
		int processUID = -1;
		//TCHAR processName[MAX_PATH] = TEXT("Unknown");
		//TCHAR processFullPath[MAX_PATH] = TEXT("-");

		// - Get a Packet (Process UID or PID is not Provided Here)
		if (!filter.Capture(&pi, &g_bCapture))
		{
			Sleep(10000);
			::PostMessage(pThis->GetSafeHwnd(), WM_RECONNECT, 0, 0);
			break;
		}

		// - Stop
		if( !g_bCapture )
		{
			break;
		}

		// - Get PID
		if( pi.trasportProtocol == TRA_TCP )
		{
			pid = pc.GetTcpPortPid(pi.local_port);
			pid = ( pid == 0 ) ? -1 : pid;
		}
		else if( pi.trasportProtocol == TRA_UDP )
		{
			pid = pc.GetUdpPortPid(pi.local_port);
			pid = ( pid == 0 ) ? -1 : pid;
		}

		// - Fill PacketInfoEx
		memcpy(&pie, &pi, sizeof(pi));

		pie.pid = pid;
		pie.puid = processUID;
		
		if (pid != -1)
		{
			OnPacket(&pie);
		}
	}

	// End --------------------------------------------------------------------
	filter.End();

	return 0;
}
BOOL Is64BitSystem()
{
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||    
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 )
	{
		return TRUE;
	}
	return FALSE;	
}
void FileCopyTo(CString source, CString destination, CString searchStr, BOOL cover = TRUE)
{
	CString strSourcePath = source;
	CString strDesPath = destination;
	CString strFileName = searchStr;
	CFileFind filefinder;
	CString strSearchPath = strSourcePath + _T("\\") + strFileName;
	CString filename;
	BOOL bfind = filefinder.FindFile(strSearchPath);
	CString SourcePath, DisPath;
	while (bfind)
	{
		bfind = filefinder.FindNextFile();
		filename = filefinder.GetFileName();
		SourcePath = strSourcePath + _T("\\") + filename;
		DisPath = strDesPath + _T("\\") + filename;
		CopyFile((LPCTSTR)SourcePath, (LPCTSTR)DisPath, cover);
	}
	filefinder.Close();
}


void Remove()
{
	SC_HANDLE	hSCManager = NULL;
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	CloseHandle(gHandle2);
	StopDriver(hSCManager,MYWINRIN0);
	if (!pThis->bHadWinpcap)
	{
		CloseHandle(gHandle);
		StopDriver(hSCManager,MYNPF);
	}
	//RemoveDriver(hSCManager,MYWINRIN0);
	CloseServiceHandle(hSCManager);
}
//打开驱动
BOOL OpenDriver(int driverId)
{
	//char message[256];
	//char *str=_T("\\\\.\\") OLS_DRIVER_ID;
	if (0 == driverId)
	{
		gHandle = CreateFile(
			_T("\\\\.\\NPF"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		// 	CString tmp;
		// 	tmp.Format(_T("OpenDriver Failed:%d"), code);
		if(gHandle == INVALID_HANDLE_VALUE)
		{
			/*		AfxMessageBox(tmp);*/
			return FALSE;
		}
	}
	else
	{
		gHandle2 = CreateFile(
			_T("\\\\.\\WinRing0_2_0_0"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if(gHandle2 == INVALID_HANDLE_VALUE)
		{
			/*		AfxMessageBox(tmp);*/
			return FALSE;
		}
	}
	return TRUE;
}
BOOL Initialize(int driveId)
{
	TCHAR dir[MAX_PATH];
	TCHAR *ptr;

	GetModuleFileName(NULL, dir, MAX_PATH);
	if((ptr = _tcsrchr(dir, '\\')) != NULL)
	{
		*ptr = '\0';
	}
	if (0 == driveId)
	{
		wsprintf(gDriverPath, _T("%s\\%s"), dir, _T("npf.sys"));
		if(IsFileExist(gDriverPath) == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		wsprintf(gDriverPath2, _T("%s\\%s"), dir, _T("WinRing0.sys"));
		if(IsFileExist(gDriverPath2) == FALSE)
		{
			return FALSE;
		}
	}
	LoadDriver(driveId);
	OpenDriver(driveId);
	return TRUE;

}
BOOL IsFileExist(LPCTSTR fileName)
{
	WIN32_FIND_DATA	findData;

	HANDLE hFile = FindFirstFile(fileName, &findData);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		FindClose( hFile );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
//加载驱动
void LoadDriver(int driveId)
{
	SC_HANDLE	hSCManager = NULL;
	SC_HANDLE	hService = NULL;
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		return;
	}
	if (0 == driveId)
	{
		hService = CreateService(hSCManager,
			MYNPF,
			MYNPF,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			gDriverPath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);
	}
	else
	{
		hService = CreateService(hSCManager,
			MYWINRIN0,
			MYWINRIN0,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			gDriverPath2,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);
	}
//	CString tmp;
	if(hService == NULL)
	{
		DWORD dwRtn = GetLastError();
		if( dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS ) 
		{
// 			tmp.Format(_T("由于其他原因创建服务失败:%u"), dwRtn);
// 			AfxMessageBox(tmp);
			return;
		}
		else
		{
/*			AfxMessageBox(_T("服务创建失败，是由于服务已经创立过"));*/
		}
		//驱动程序存在只需要加载
		if (0 == driveId)
		{
			hService = OpenService(hSCManager, MYNPF, SERVICE_ALL_ACCESS);
		}
		else
		{
			hService = OpenService(hSCManager, MYWINRIN0, SERVICE_ALL_ACCESS);
		}
		if (hService == NULL)
		{
/*			AfxMessageBox(_T("驱动服务打开失败！"));*/
		}
		else
		{
/*			AfxMessageBox(_T("驱动服务打开成功！"));*/
		}
	}
	else
	{
/*		AfxMessageBox(_T("驱动服务创建成功"));*/
	}
	if (0 == driveId)
	{
		hService = OpenService(hSCManager, MYNPF, SERVICE_ALL_ACCESS);
	}
	else
	{
		hService = OpenService(hSCManager, MYWINRIN0, SERVICE_ALL_ACCESS);
	}
	
	if (hService == NULL)
	{
/*		AfxMessageBox(_T("驱动服务打开失败！"));*/
	}
	else
	{
/*		AfxMessageBox(_T("驱动服务打开成功！"));*/
	}

	BOOL result=StartService(hService, 0, NULL);
	if (!result)
	{
		int dwRtn = GetLastError(); 
/*		tmp.Format(_T("驱动服务启动失败:%d"), dwRtn);*/
		if( dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING ) 
		{
/*			AfxMessageBox(tmp);*/
		}
		else
		{
/*			AfxMessageBox(_T("驱动服务已经启动了！"));*/
		}
	}
	else
	{
/*		AfxMessageBox(_T("驱动服务启动成功！"));*/
	}

	if (hService)
	{
		CloseServiceHandle(hService);
/*		AfxMessageBox(_T("关闭hService"));*/
	}
	if (hSCManager)
	{
		CloseServiceHandle(hSCManager);
/*		AfxMessageBox(_T("关闭hSCManager"));*/
	}
}
BOOL  StopDriver(SC_HANDLE hSCManager,LPCTSTR DriverId)
{
	SC_HANDLE		hService = NULL;
	BOOL			rCode = FALSE;
	SERVICE_STATUS	serviceStatus;
	DWORD		error = NO_ERROR;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);

	if(hService != NULL)
	{
		rCode = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
		if (!rCode)
		{
/*			AfxMessageBox(_T("停止驱动失败"));*/
		}

		rCode = DeleteService(hService);
		if (!rCode)
		{
/*			AfxMessageBox(_T("删除服务失败"));*/
		}

		CloseServiceHandle(hService);
/*		AfxMessageBox(_T("关闭hService"));*/
	}
	else
	{
/*		AfxMessageBox(_T("打开服务失败"));*/
	}

	return rCode;
}
BOOL RemoveDriver(SC_HANDLE hSCManager, LPCTSTR DriverId)
{
	SC_HANDLE   hService = NULL;
	BOOL        rCode = FALSE;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if(hService == NULL)
	{
		rCode = TRUE;
	}
	else
	{
		rCode = DeleteService(hService);
		CloseServiceHandle(hService);
	}

	return rCode;
}

// CpermoDlg 对话框

__int64 CompareFileTime(FILETIME time1, FILETIME time2)
{
	__int64 a = time1.dwHighDateTime << 32 | time1.dwLowDateTime;
	__int64 b = time2.dwHighDateTime << 32 | time2.dwLowDateTime;

	return   (b - a);
}

CpermoDlg::CpermoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CpermoDlg::IDD, pParent)
	, nCPU(0)
	, nMem(0)
	, nTrans(255)
	, fNetUp(0.0)
	, fNetDown(0.0)
	, bTopmost(TRUE)
	, bShowBand(false)
	, bAutoHide(FALSE)
	, nShowWay(0)
	, _bMouseTrack(TRUE)
	, pcoControl(NULL)
	, bInfoDlgShowing(false)
	, nBandFontSize(16)
	, bLockWndPos(false)
	, nBandWidth(80)
	, nBandHeight(30)
	, bIsWndVisable(true)
	, bFullScreen(true)
	, bHideWndSides(false)
	, nNowBandShowIndex(0)
	, bBandShowCpu(false)
	, bBandShowMem(false)
	, bBandShowNetUp(false)
	, bBandShowNetDown(false)
	, nCount(0)
	, pLoc(NULL)
	, pSvc(NULL)
	, pEnumerator(NULL)
	, bBandShowDiskTem(false)
	, bHadWinpcap(false)
	, bIsWindowsVistaOrGreater(false)
	, bShowOneSideInfo(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CpermoDlg::~CpermoDlg()
{
	delete pcoControl;
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
	ON_MESSAGE(MSG_BAND_MENU,&CpermoDlg::OnBandMenu)
	ON_MESSAGE(WM_RECONNECT, &CpermoDlg::OnReconnect)
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
	get_processor_number();
	bIsWindowsVistaOrGreater = false;
 	DWORD dwVersion = 0;
 	DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0; 
 	dwVersion = ::GetVersion();
 	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
     //dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
 	if (dwMajorVersion > 5)
 	{
 		bIsWindowsVistaOrGreater = true;
 	}
	/*
	If dwMajorVersion = 6 And dwMinorVersion = 1 Then GetWinVersion = "windows 7"
    If dwMajorVersion = 6 And dwMinorVersion = 0 Then GetWinVersion = "windows vista"
    If dwMajorVersion = 5 And dwMinorVersion = 1 Then GetWinVersion = "windows xp"
    If dwMajorVersion = 5 And dwMinorVersion = 0 Then GetWinVersion = "windows 2000"
	*/

	BOOL bRet = FALSE;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rWorkArea, 0);   // 获得工作区大小
	bRet = SetWorkDir();

	DeleteFiles();
	OpenConfig();
	InitSize();

	CreateInfoDlg();
	GetWindowRect(&rCurPos);
	
	InitializeCriticalSection(&_cs);

	//拷贝文件
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	CString dis;
	dis.Format(_T("%s"), direc);
	CString str32 = dis + _T("\\x32");
	CString str64 = dis + _T("\\x64");
	if (Is64BitSystem())
	{
		FileCopyTo(str64, dis, _T("WinRing0.sys"), TRUE);
	}
	else
	{
		FileCopyTo(str32, dis, _T("WinRing0.sys"), TRUE);
	}
	if (!Initialize(1))
	{
		AfxMessageBox(_T("文件丢失"));
	}

	PcapNetFilter filter;
	// Init Filter
	if( !filter.Init())
	{
		bHadWinpcap = false;

		TCHAR direc[256];
		::GetCurrentDirectory(256, direc);//获取当前目录函数
		CString dis;
		dis.Format(_T("%s"), direc);
		CString str32 = dis + _T("\\x32");
		CString str64 = dis + _T("\\x64");
		if (Is64BitSystem())
		{
			FileCopyTo(str64, dis, _T("npf.sys"), TRUE);
			FileCopyTo(str64, dis, _T("wpcap.dll"), TRUE);
			FileCopyTo(str64, dis, _T("Packet.dll"), TRUE);
		}
		else
		{
			FileCopyTo(str32, dis, _T("npf.sys"), TRUE);
			FileCopyTo(str32, dis, _T("wpcap.dll"), TRUE);
			if (bIsWindowsVistaOrGreater)
			{
				FileCopyTo(str64, dis, _T("Packet.dll"), TRUE);
			}
			else
			{
				FileCopyTo(str32, dis, _T("Packet.dll"), TRUE);
			}
		}
		if (!Initialize(0))
		{
			AfxMessageBox(_T("文件丢失"));
		}
	
		filter.Init();
	}
	else
	{
		bHadWinpcap = true;
	}
	// Find Devices
	g_nAdapters = filter.FindDevices();
	if( g_nAdapters <= 0 )
	{
		AfxMessageBox(_T("No network adapters has been found on this machine."));
	}
	// Get Device Names
	for(int i = 0; i < g_nAdapters; i++)
	{
		TCHAR *name = filter.GetName(i);

		// Save device name
		if( i < _countof(g_szAdapterNames))
		{
			_tcscpy_s(g_szAdapterNames[i], 256, name);
		}
	}
	// End
	filter.End();

	strNetDown.Format(_T("0.0KB/S"));
	strNetUp.Format(_T("0.0KB/S"));
	strCPU.Format(_T("0%%"));
	strMem.Format(_T("0%%"));

	if (IsIntel())
	{
		GetCpuTemp();
	}

	if (!::GetSystemTimes(&preidleTime, &prekernelTime, &preuserTime))
	{
		return -1;
	}
	m_SubMenu_NetPort.CreatePopupMenu();

	//添加网络适配器菜单
	for (int i = 0; i < g_nAdapters; i++)
	{
		m_SubMenu_NetPort.AppendMenu(MF_STRING, i + START_INDEX, g_szAdapterNames[i]);
	}
	if (g_iAdapter >= g_nAdapters)
	{
		g_iAdapter = 0;
	}
	//初始化COM接口用来获取硬盘温度
	hres = CoInitializeEx(0, COINIT_MULTITHREADED); 

	hres = CoInitializeSecurity(
		NULL, 
		-1, // COM authentication
		NULL, // Authentication services
		NULL, // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT, // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation 
		NULL, // Authentication info
		EOAC_NONE, // Additional capabilities 
		NULL // Reserved
		);

	hres = CoCreateInstance(
		CLSID_WbemLocator, 
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator, (LPVOID *) &pLoc);

	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\WMI"), // Object path of WMI namespace
		NULL, // User name. NULL = current user
		NULL, // User password. NULL = current
		0, // Locale. NULL indicates current
		NULL, // Security flags.
		0, // Authority (e.g. Kerberos)
		0, // Context object 
		&pSvc // pointer to IWbemServices proxy
		);

	hres = CoSetProxyBlanket(
		pSvc, // Indicates the proxy to set
		RPC_C_AUTHN_WINNT, // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE, // RPC_C_AUTHZ_xxx
		NULL, // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL, // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL, // client identity
		EOAC_NONE // proxy capabilities 
		);

	GetDiskTem();
	
	//创建菜单
	InitPopMenu(nCount);
	//默认置顶
	if (bTopmost)
	{
		m_Menu.CheckMenuItem(IDM_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bAutoHide)
	{
		m_Menu.CheckMenuItem(IDM_AUTOHIDE, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bShowOneSideInfo)
	{
		m_Menu.CheckMenuItem(IDM_SHOWONESIDEINFO, MF_BYCOMMAND | MF_CHECKED);
	}
	m_Menu.CheckMenuItem(IDM_SHOWBYHOVER + nShowWay, MF_BYCOMMAND | MF_CHECKED);
	if (bShowNetInfo)
	{
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		m_Menu.CheckMenuItem(IDM_SHOWNETINFO, MF_BYCOMMAND | MF_UNCHECKED);
	}
	//需要显示任务栏标尺则进行创建
	if (bShowBand)
	{
		if (NULL == pcoControl)
		{
			pcoControl = new CNProgressBar(this);
			pcoControl->SetPosEx(0);
		}
		pcoControl->SetFontSize(nBandFontSize);
		SetBandWidth(nBandWidth);
		SetBandHeight(nBandHeight);
		pcoControl->Show(bShowBand);
		m_Menu.CheckMenuItem(IDM_SHOWBAND, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		m_Menu.CheckMenuItem(IDM_SHOWBAND, MF_BYCOMMAND | MF_UNCHECKED);
	}
	if (bFullScreen)
	{
		m_Menu.CheckMenuItem(IDM_FULLSCREEN, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bLockWndPos)
	{
		m_Menu.CheckMenuItem(IDM_LOCKWNDPOS, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bHideWndSides)
	{
		m_Menu.CheckMenuItem(IDM_HIDEWNDSIDES, MF_BYCOMMAND | MF_CHECKED);
	}
	if (bBandShowCpu)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(0);
	}
	if (bBandShowMem)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWMEM, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(1);
	}
	if (bBandShowNetDown)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETDOWN, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(2);
	}
	if (bBandShowNetUp)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETUP, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(3);
	}
	if (bBandShowDiskTem)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWDISKTEM, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(4);
	}
	if (bBandShowCpuTem)
	{
		m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPUTEM, MF_BYCOMMAND | MF_CHECKED);
		vBandShow.push_back(5);
	}

	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE12 + nBandFontSize - 12, MF_BYCOMMAND | MF_CHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDWIDTH50 + (nBandWidth-50)/10, MF_BYCOMMAND | MF_CHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDHEIGHT20 + (nBandHeight-20)/5, MF_BYCOMMAND | MF_CHECKED);
	m_Menu.CheckMenuItem(nSkin, MF_BYCOMMAND | MF_CHECKED); // 在前面打钩 
	m_Menu.CheckMenuItem(IDM_FONTSIZE12 + nFontSize - 12, MF_BYCOMMAND | MF_CHECKED);
	IfAutoRun();//判断是否已经开机自启

	//取消任务栏显示
	SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, WS_EX_TOOLWINDOW);

	//每隔一秒刷新CPU和网络温度等信息
	//采用多媒体定时器
	mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
	//SetTimer(1, 1000, NULL);
	//每隔1.8秒检测全屏程序
	SetTimer(2, 1800, NULL);
	//每隔2.5分钟重新启动一下监控
	SetTimer(3, 150000, NULL);
	
	::SetWindowLong( m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	::SetLayeredWindowAttributes( m_hWnd, 0, nTrans, LWA_ALPHA); // 120是透明度，范围是0～255
	m_Menu.CheckMenuItem(IDM_TRANS0+(255-nTrans)/25, MF_BYCOMMAND | MF_CHECKED);
	
	pThis = this;

	if (bTopmost)
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	StartCapture();

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
	if (bHideWndSides)
	{
		rCurPos.right = rCurPos.left + 150;
	}
	else
	{
		rCurPos.right = rCurPos.left + 220;
	}
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
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(150, 240, 150));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
		}
		break;
	case IDM_BLUE:
		{
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(66, 66, 66));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
		}
		break;
	case IDM_BLACK:
		{
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(100, 100, 100));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
		}
		break;
	case IDM_RED:
		{
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(240, 150, 150));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
		}
		break;
	case IDM_ORANGE:
		{
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(250, 230, 20));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
		}
		break;
	default:
		{
			if (bHideWndSides)
			{
				CBrush MiBrush(RGB(150, 240, 150));
				CPen *pOldPen = pDC->SelectObject(&MyPen);
				CBrush *pOldBrush = pDC->SelectObject(&MiBrush);
				pDC->Rectangle(0, 0, 150, 22);
				pDC->SelectObject(pOldPen);
				pDC->SelectObject(pOldBrush);
			}
			else
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
	COLORREF cTempTextColor;
	if (IDM_GREEN == nSkin || IDM_ORANGE == nSkin)
	{
		cOldTextColor = pDC->SetTextColor(RGB(0, 0, 0));
	}
	else
	{
		cOldTextColor = pDC->SetTextColor(RGB(255, 255, 255));
	}
	int nOldBkMode = pDC->SetBkMode(TRANSPARENT);
	CRect rText;
	CRect rcIcon;
	if (bHideWndSides)
	{
		if (IDM_BLACK == nSkin || IDM_BLUE == nSkin)
		{
			pDC->SetTextColor(RGB(255, 255, 255));
		}
		else
		{
			pDC->SetTextColor(RGB(0, 0, 0));
		}
		rcIcon.left = 3;
		rcIcon.right = 15;
		rcIcon.top = 5;
		rcIcon.bottom = 17;
		DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DOWN)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
		rText.top = 2;
		rText.bottom = 21;
		rText.left = 17;
		rText.right = 75;
		pDC->DrawText(strNetDown, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rcIcon.left = 77;
		rcIcon.right = 89;
		DrawIconEx(pDC->GetSafeHdc(), rcIcon.left, rcIcon.top, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_UP)), rcIcon.Width(), rcIcon.Height(), 0, NULL, DI_NORMAL);
		rText.left = 91;
		rText.right = 149;
		pDC->DrawText(strNetUp, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		rText.left = 1;
		rText.right = 36;
		rText.top = 2;
		rText.bottom = 21;
		if (nCPU > 70)
		{
			cTempTextColor = pDC->SetTextColor(RGB(250, 180, 50));
		}
		pDC->DrawText(strCPU, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rText.left = 186;
		rText.right = 219;
		if (nMem > 70 && nCPU <= 70)
		{
			pDC->SetTextColor(RGB(250, 180, 50));
		}
		else if (nMem <= 10 && nCPU > 10)
		{
			pDC->SetTextColor(cTempTextColor);
		}
		pDC->DrawText(strMem, &rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (IDM_BLACK == nSkin || IDM_BLUE == nSkin)
		{
			pDC->SetTextColor(RGB(255, 255, 255));
		}
		else
		{
			pDC->SetTextColor(RGB(0, 0, 0));
		}
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
	}

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
	mm_Timer.KillTimer();
	CDialog::OnLButtonDown(nFlags, point);
}


void CpermoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (2 == nIDEvent)
	{
		if (bFullScreen)
		{
			bool bFullScreen = IsForegroundFullscreen();
			if (bFullScreen)
			{
				if (bIsWndVisable)
				{
					ShowWindow(SW_HIDE);
				}
			}
			else
			{
				if ((!IsWindowVisible()) && bIsWndVisable)
				{
					ShowWindow(SW_SHOW);
					if (bTopmost)
					{
						SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					}
				}
			}
		}
	}
 	if (3 == nIDEvent)
 	{
 		StopCapture();
 		StartCapture();
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
	int nID = 0;
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CPoint p;
	//传递过来的坐标为相对于窗口左上角的坐标，WM_CONTEXTMENU传递过来的是屏幕坐标
	GetCursorPos(&p);//鼠标点的屏幕坐标
	m_Menu.CheckMenuItem(g_iAdapter + START_INDEX, MF_BYCOMMAND | MF_CHECKED); 
	nID = m_Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, p.x, p.y, this);
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
		SetShowWay(0);
		break;
	case IDM_SHOWBYLDOWN:
		SetShowWay(1);
		break;
	case IDM_SHOWNEVER:
		SetShowWay(2);
		break;
	case IDM_LOCKWNDPOS:
		{
			if (bLockWndPos)
			{
				m_Menu.CheckMenuItem(IDM_LOCKWNDPOS, MF_BYCOMMAND | MF_UNCHECKED);
				bLockWndPos = false;
			}
			else
			{
				m_Menu.CheckMenuItem(IDM_LOCKWNDPOS, MF_BYCOMMAND | MF_CHECKED);
				bLockWndPos = true;
			}
		}
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
	case IDM_FULLSCREEN:
		{
			if (bFullScreen)
			{
				m_Menu.CheckMenuItem(IDM_FULLSCREEN, MF_BYCOMMAND | MF_UNCHECKED);
				bFullScreen = false;
			}
			else
			{
				m_Menu.CheckMenuItem(IDM_FULLSCREEN, MF_BYCOMMAND | MF_CHECKED);
				bFullScreen = true;
			}
		}
		break;
	case IDM_AUTOSTART:
		SetAutoRun();
		break;
	case IDM_SHOWNETINFO:
		ShowNetInfo();
		break;
	case IDM_SHOWBAND:
		ShowBand();
		break;
	case IDM_HIDEWNDSIDES:
		{
			if (bHideWndSides)
			{
				rCurPos.right = rCurPos.left + 220;
				MoveWindow(&rCurPos, TRUE);
				m_Menu.CheckMenuItem(IDM_HIDEWNDSIDES, MF_BYCOMMAND | MF_UNCHECKED);
				bHideWndSides = false;
				MoveInfoDlg();
			}
			else
			{
				rCurPos.right = rCurPos.left + 150;
				MoveWindow(&rCurPos, TRUE);
				m_Menu.CheckMenuItem(IDM_HIDEWNDSIDES, MF_BYCOMMAND | MF_CHECKED);
				bHideWndSides = true;
				MoveInfoDlg();
			}
		}
		break;
	case IDM_SHOWONESIDEINFO:
		{
			if (bShowOneSideInfo)
			{
				m_Menu.CheckMenuItem(IDM_SHOWONESIDEINFO, MF_BYCOMMAND | MF_UNCHECKED);
				bShowOneSideInfo = false;
			}
			else
			{
				m_Menu.CheckMenuItem(IDM_SHOWONESIDEINFO, MF_BYCOMMAND | MF_CHECKED);
				bShowOneSideInfo = true;
			}
		}
		break;
	case 0:
		return;
	default:
		{m_Menu.CheckMenuItem(g_iAdapter + START_INDEX, MF_BYCOMMAND | MF_UNCHECKED);
		g_iAdapter = nID - START_INDEX; StopCapture(); StartCapture();}
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
	if (g_bCapture)
	{
		StopCapture();
	}
	Remove();
	DeleteFiles();
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
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_SHOWONESIDEINFO, _T("贴边隐藏显示一侧信息"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_SHOWNETINFO, _T("显示流量监控信息"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_SHOWBAND, _T("显示任务栏标尺"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_LOCKWNDPOS, _T("锁定悬浮窗位置"));
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
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_HIDEWNDSIDES, _T("隐藏悬浮窗两侧信息"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_SEPARATOR);
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_NetPort.m_hMenu, _T("选择需要监控的网络接口"));
	m_SubMenu_ShowWay.AppendMenu(MF_BYCOMMAND, IDM_SHOWBYHOVER, _T("鼠标滑过立即弹出"));
	m_SubMenu_ShowWay.AppendMenu(MF_BYCOMMAND, IDM_SHOWBYLDOWN, _T("鼠标点击弹出"));
	m_SubMenu_ShowWay.AppendMenu(MF_BYCOMMAND, IDM_SHOWNEVER, _T("从不弹出"));
	m_Menu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_SubMenu_ShowWay.m_hMenu, _T("明细窗口弹出方式"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_FULLSCREEN, _T("全屏免打扰"));
	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_AUTOSTART, _T("允许开机自启动"));

	m_Menu.AppendMenu(MF_BYCOMMAND, IDM_EXIT, _T("退出"));

	bRet = m_BandMenu.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_BandFontSizeMenu.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_BandWidthMenu.CreatePopupMenu();
	ASSERT(bRet);
	bRet = m_BandHeightMenu.CreatePopupMenu();
	ASSERT(bRet);
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWCPU, _T("显示CPU占用"));
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWMEM, _T("显示内存占用"));
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWNETDOWN, _T("显示下载速度"));
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWNETUP, _T("显示上传速度"));
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWDISKTEM, _T("显示硬盘温度"));
	if (gIsMsr)
	{
		m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDSHOWCPUTEM, _T("显示CPU温度"));
	}
	m_BandMenu.AppendMenu(MF_BYPOSITION | MF_SEPARATOR);
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE12, _T("12"));
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE13, _T("13"));
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE14, _T("14"));
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE15, _T("15"));
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE16, _T("16"));
	m_BandFontSizeMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDFONTSIZE17, _T("17"));
	m_BandMenu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_BandFontSizeMenu.m_hMenu, _T("标尺字体大小设置"));
	m_BandWidthMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDWIDTH50, _T("50"));
	m_BandWidthMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDWIDTH60, _T("60"));
	m_BandWidthMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDWIDTH70, _T("70"));
	m_BandWidthMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDWIDTH80, _T("80(默认)"));
	m_BandMenu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_BandWidthMenu.m_hMenu, _T("标尺宽度设置"));
	m_BandHeightMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDHEIGHT20, _T("20"));
	m_BandHeightMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDHEIGHT25, _T("25"));
	m_BandHeightMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDHEIGHT30, _T("30(默认)"));
	m_BandHeightMenu.AppendMenu(MF_BYCOMMAND, IDM_BANDHEIGHT35, _T("35"));
	m_BandMenu.AppendMenu(MF_BYPOSITION | MF_POPUP,
		(UINT)m_BandHeightMenu.m_hMenu, _T("标尺高度设置"));
	m_BandMenu.AppendMenu(MF_BYPOSITION | MF_SEPARATOR);
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_SHOWHIDEWND, _T("隐藏/显示悬浮窗"));
	m_BandMenu.AppendMenu(MF_BYCOMMAND, IDM_EXIT, _T("退出"));
}


void CpermoDlg::OpenConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	int left, top, topmost, skin, autohide, trans, showway, shownetinfo,
		fontsize, showband, bandfontsize, bandwidth, bandheight,
		fullscreen, lockwndpos, hidewndsides, bandshowcpu, bandshowmem,
		bandshownetup, bandshownetdown, bandshowdisktem, bandshowcputem,
		adapter, showonesideinfo;
	left = ::GetPrivateProfileInt(_T("Main"), _T("left"), -1, temp);
	top = ::GetPrivateProfileInt(_T("Main"), _T("top"), -1, temp);
	topmost = ::GetPrivateProfileInt(_T("Main"), _T("topmost"), -1, temp);
	skin = ::GetPrivateProfileInt(_T("Main"), _T("skin"), -1, temp);
	autohide = ::GetPrivateProfileInt(_T("Main"), _T("autohide"), -1, temp);
	trans = ::GetPrivateProfileInt(_T("Main"), _T("trans"), -1, temp);
	showway = ::GetPrivateProfileInt(_T("Main"), _T("showway"), -1, temp);
	shownetinfo = ::GetPrivateProfileInt(_T("Main"), _T("shownetinfo"), -1, temp);
	fontsize = ::GetPrivateProfileInt(_T("Main"), _T("fontsize"), -1, temp);
	showband = ::GetPrivateProfileInt(_T("Main"), _T("showband"), -1, temp);
	bandfontsize = ::GetPrivateProfileInt(_T("Main"), _T("bandfontsize"), -1, temp);
	bandwidth = ::GetPrivateProfileInt(_T("Main"), _T("bandwidth"), -1, temp);
	bandheight = ::GetPrivateProfileInt(_T("Main"), _T("bandheight"), -1, temp);
	fullscreen = ::GetPrivateProfileInt(_T("Main"), _T("fullscreen"), -1, temp);
	lockwndpos = ::GetPrivateProfileInt(_T("Main"), _T("lockwndpos"), -1, temp);
	hidewndsides = ::GetPrivateProfileInt(_T("Main"), _T("hidewndsides"), -1, temp);
	bandshowcpu = ::GetPrivateProfileInt(_T("Main"), _T("bandshowcpu"), -1, temp);
	bandshowmem = ::GetPrivateProfileInt(_T("Main"), _T("bandshowmem"), -1, temp);
	bandshownetup = ::GetPrivateProfileInt(_T("Main"), _T("bandshownetup"), -1, temp);
	bandshownetdown = ::GetPrivateProfileInt(_T("Main"), _T("bandshownetdown"), -1, temp);
	bandshowdisktem = ::GetPrivateProfileInt(_T("Main"), _T("bandshowdisktem"), -1, temp);
	bandshowcputem = ::GetPrivateProfileInt(_T("Main"), _T("bandshowcputem"), -1, temp);
	adapter = ::GetPrivateProfileInt(_T("Main"), _T("adapter"), -1, temp);
	showonesideinfo = ::GetPrivateProfileInt(_T("Main"), _T("showonesideinfo"), -1, temp);
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
	if (1 == topmost)
	{
		bTopmost = TRUE;
	}
	else
	{
		bTopmost = FALSE;
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
	if (1 == autohide)
	{
		bAutoHide = TRUE;
	}
	else
	{
		bAutoHide = FALSE;
	}
	if (trans < 0 || trans > 255)
	{
		nTrans = 255;
	}
	else
	{
		nTrans = trans;
	}
	if (-1 == showway || showway > 2)
	{
		nShowWay = 0;
	}
	else
	{
		nShowWay = showway;
	}
	if (1 == shownetinfo)
	{
		bShowNetInfo = TRUE;
	}
	else
	{
		bShowNetInfo = FALSE;
	}
	if (-1 == fontsize || fontsize < 12 || fontsize > 18)
	{
		nFontSize = 18;
	}
	else
	{
		nFontSize = fontsize;
	}
	if (1 == showband)
	{
		bShowBand = true;
	}
	else
	{
		bShowBand = false;
	}
	if (-1 == bandfontsize || bandfontsize < 12 || bandfontsize > 17)
	{
		nBandFontSize = 16;
	}
	else
	{
		nBandFontSize = bandfontsize;
	}
	if (-1 == bandwidth || bandwidth<50 || bandwidth>80)
	{
		nBandWidth= 80;
	}
	else
	{
		nBandWidth = bandwidth;
	}
	if (-1 == bandheight || bandheight<20 || bandheight>35)
	{
		nBandHeight= 30;
	}
	else
	{
		nBandHeight = bandheight;
	}
	if (0 == fullscreen)
	{
		bFullScreen = false;
	}
	else
	{
		bFullScreen = true;
	}
	if (1 == lockwndpos)
	{
		bLockWndPos = true;
	}
	else
	{
		bLockWndPos = false;
	}
	if (1 == hidewndsides)
	{
		bHideWndSides = true;
	}
	else
	{
		bHideWndSides = false;
	}
	if (1 == bandshowcpu)
	{
		bBandShowCpu = true;
	}
	else
	{
		bBandShowCpu = false;
	}
	if (1 == bandshowmem)
	{
		bBandShowMem = true;
	}
	else
	{
		bBandShowMem = false;
	}
	if (1 == bandshownetup)
	{
		bBandShowNetUp = true;
	}
	else
	{
		bBandShowNetUp = false;
	}
	if (1 == bandshownetdown)
	{
		bBandShowNetDown = true;
	}
	else
	{
		bBandShowNetDown = false;
	}
	if (1 == bandshowdisktem)
	{
		bBandShowDiskTem = true;
	}
	else
	{
		bBandShowDiskTem = false;
	}
	if (1 == bandshowcputem)
	{
		bBandShowCpuTem = true;
	}
	else
	{
		bBandShowCpuTem = false;
	}
	if (adapter != -1)
	{
		g_iAdapter= adapter;
	}
	else
	{
		g_iAdapter = 0;
	}
	if (1 == showonesideinfo)
	{
		bShowOneSideInfo = true;
	}
	else
	{
		bShowOneSideInfo = false;
	}
}


BOOL CpermoDlg::SaveConfig()
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\config.ini"), direc);
	TCHAR cLeft[32], cTop[32], cTopMost[32], cSkin[32], cAutoHide[32], 
		cTrans[32], cShowWay[32], cShowNetInfo[32], cFontSize[32], 
		cShowBand[32], cBandFontSize[32], cBandWidth[32],
		cBandHeight[32], cFullScreen[32], cLockWndPos[32], cHideWndSides[32],
		cBandShowCpu[32], cBandShowMem[32], cBandShowNetUp[32], cBandShowNetDown[32],
		cBandShowDiskTem[32], cBandShowCpuTem[32], cAdapter[32], cShowOneSideInfo[32];
	_itow_s(rCurPos.left, cLeft, 10);
	_itow_s(rCurPos.top, cTop, 10);
	_itow_s(bTopmost, cTopMost, 10);
	_itow_s(nSkin, cSkin, 10);
	_itow_s(bAutoHide, cAutoHide, 10);
	_itow_s(nTrans, cTrans, 10);
	_itow_s(nShowWay, cShowWay, 10);
	_itow_s(bShowNetInfo, cShowNetInfo, 10);
	_itow_s(nFontSize, cFontSize, 10);
	_itow_s(bShowBand, cShowBand, 10);
	_itow_s(nBandFontSize, cBandFontSize, 10);
	_itow_s(nBandWidth, cBandWidth, 10);
	_itow_s(nBandHeight, cBandHeight, 10);
	_itow_s(bFullScreen, cFullScreen, 10);
	_itow_s(bLockWndPos, cLockWndPos, 10);
	_itow_s(bHideWndSides, cHideWndSides, 10);
	_itow_s(bBandShowCpu, cBandShowCpu, 10);
	_itow_s(bBandShowMem, cBandShowMem, 10);
	_itow_s(bBandShowNetUp, cBandShowNetUp, 10);
	_itow_s(bBandShowNetDown, cBandShowNetDown, 10);
	_itow_s(bBandShowDiskTem, cBandShowDiskTem, 10);
	_itow_s(bBandShowCpuTem, cBandShowCpuTem, 10);
	_itow_s(g_iAdapter, cAdapter, 10);
	_itow_s(bShowOneSideInfo, cShowOneSideInfo, 10);
	::WritePrivateProfileString(_T("Main"), _T("left"), cLeft, temp);
	::WritePrivateProfileString(_T("Main"), _T("top"), cTop, temp);
	::WritePrivateProfileString(_T("Main"), _T("topmost"), cTopMost, temp);
	::WritePrivateProfileString(_T("Main"), _T("skin"), cSkin, temp);
	::WritePrivateProfileString(_T("Main"), _T("autohide"), cAutoHide, temp);
	::WritePrivateProfileString(_T("Main"), _T("trans"), cTrans, temp);
	::WritePrivateProfileString(_T("Main"), _T("showway"), cShowWay, temp);
	::WritePrivateProfileString(_T("Main"), _T("shownetinfo"), cShowNetInfo, temp);
	::WritePrivateProfileString(_T("Main"), _T("fontsize"), cFontSize, temp);
	::WritePrivateProfileString(_T("Main"), _T("showband"), cShowBand, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandfontsize"), cBandFontSize, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandwidth"), cBandWidth, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandheight"), cBandHeight, temp);
	::WritePrivateProfileString(_T("Main"), _T("fullscreen"), cFullScreen, temp);
	::WritePrivateProfileString(_T("Main"), _T("lockwndpos"), cLockWndPos, temp);
	::WritePrivateProfileString(_T("Main"), _T("hidewndsides"), cHideWndSides, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshowcpu"), cBandShowCpu, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshowmem"), cBandShowMem, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshownetup"), cBandShowNetUp, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshownetdown"), cBandShowNetDown, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshowdisktem"), cBandShowDiskTem, temp);
	::WritePrivateProfileString(_T("Main"), _T("bandshowcputem"), cBandShowCpuTem, temp);
	::WritePrivateProfileString(_T("Main"), _T("adapter"), cAdapter, temp);
	::WritePrivateProfileString(_T("Main"), _T("showonesideinfo"), cShowOneSideInfo, temp);
	return TRUE;
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
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, _T("permo"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			RegDeleteValue (hKey,_T("permo"));   
			if (bIsWindowsVistaOrGreater)
			{
				DelWin7SchTasks();
			}
			m_Menu.CheckMenuItem(IDM_AUTOSTART, MF_BYCOMMAND | MF_UNCHECKED);
		}
		else
		{
			TCHAR szModule[_MAX_PATH];
			GetModuleFileName(NULL, szModule, _MAX_PATH);//得到本程序自身的全路径
			if (bIsWindowsVistaOrGreater)
			{
				AddWin7SchTasks();
				RegSetValueEx(hKey,_T("permo"), 0, REG_SZ, (LPBYTE)szModule, 0); //添加一个子Key,并设置值
			}
			else
			{
				RegSetValueEx(hKey,_T("permo"), 0, REG_SZ, (LPBYTE)szModule, wcslen(szModule)*sizeof(TCHAR)); //添加一个子Key,并设置值
			}
			m_Menu.CheckMenuItem(IDM_AUTOSTART, MF_BYCOMMAND | MF_CHECKED);
		}
		RegCloseKey(hKey); //关闭注册表
	}
	else
	{
		AfxMessageBox(_T("设置失败，可能权限问题或者被杀毒软件拦截了~~"));   
	}
}

void CpermoDlg::IfAutoRun(void)
{
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");//找到系统的启动项
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
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
	if (bHideWndSides)
	{
		rect.left -= 45;
		rect.right += 45;
	}
	else
	{
		rect.left -= 10;
		rect.right += 10;
	}
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

void CpermoDlg::SetShowWay(int index)
{
	m_Menu.CheckMenuItem(IDM_SHOWBYHOVER + nShowWay, MF_BYCOMMAND | MF_UNCHECKED);
	nShowWay = index;
	m_Menu.CheckMenuItem(IDM_SHOWBYHOVER + nShowWay, MF_BYCOMMAND | MF_CHECKED);
}

void CpermoDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (1 == nShowWay)
	{
		MoveInfoDlg();
		pInfoDlg->ShowWindow(SW_SHOW);
		bInfoDlgShowing = true;
	}
	ReleaseCapture();
	mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
	CDialog::OnLButtonUp(nFlags, point);
}


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
	if (bLockWndPos)
	{
		return;
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
	if (bAutoHide && !bHideWndSides)
	{
		int tmp;
		if (bShowOneSideInfo)
		{
			tmp = 185;
		}
		else
		{
			tmp = 219;
		}
		if (rCurPos.right == rWorkArea.right + tmp)
		{
			rCurPos.right = rWorkArea.right;
			rCurPos.left = rCurPos.right - 220;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.left == rWorkArea.left - tmp)
		{
			rCurPos.left = rWorkArea.left;
			rCurPos.right = rCurPos.left + 220;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.bottom == rWorkArea.top + 1)
		{
			rCurPos.top = rWorkArea.top;
			rCurPos.bottom = rCurPos.top + 22;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	else if (bAutoHide && bHideWndSides)
	{
		int tmp;
		tmp = 145;
		if (rCurPos.right == rWorkArea.right + tmp)
		{
			rCurPos.right = rWorkArea.right;
			rCurPos.left = rCurPos.right - 150;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.left == rWorkArea.left - tmp)
		{
			rCurPos.left = rWorkArea.left;
			rCurPos.right = rCurPos.left + 150;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.bottom == rWorkArea.top + 1)
		{
			rCurPos.top = rWorkArea.top;
			rCurPos.bottom = rCurPos.top + 22;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	{
		//pInfoDlg->SetTimer(1, 1000, NULL);
		if (0 == nShowWay)
		{
			pInfoDlg->ShowWindow(SW_SHOW);
			bInfoDlgShowing = true;
		}
	}
	CDialog::OnMouseHover(nFlags, point);
}

void CpermoDlg::OnMouseLeave()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	_bMouseTrack=TRUE; 
	if (bAutoHide && !bHideWndSides)
	{
		int tmp;
		if (bShowOneSideInfo)
		{
			tmp = 185;
		}
		else
		{
			tmp = 219;
		}
		if (rCurPos.right >= rWorkArea.right - 5)
		{
			rCurPos.right = rWorkArea.right + tmp;
			rCurPos.left = rCurPos.right - 220;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.left <= rWorkArea.left + 5)
		{
			rCurPos.left = rWorkArea.left - tmp;
			rCurPos.right = rCurPos.left + 220;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.top <= rWorkArea.top + 5)
		{
			rCurPos.bottom = rWorkArea.top + 1;
			rCurPos.top = rCurPos.bottom - 22;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	else if (bAutoHide && bHideWndSides)
	{
		int tmp;
		tmp = 145;
		if (rCurPos.right >= rWorkArea.right - 5)
		{
			rCurPos.right = rWorkArea.right + tmp;
			rCurPos.left = rCurPos.right - 150;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.left <= rWorkArea.left + 5)
		{
			rCurPos.left = rWorkArea.left - tmp;
			rCurPos.right = rCurPos.left + 150;
			MoveWindow(&rCurPos, TRUE);
		}
		else if (rCurPos.top <= rWorkArea.top + 5)
		{
			rCurPos.bottom = rWorkArea.top + 1;
			rCurPos.top = rCurPos.bottom - 22;
			MoveWindow(&rCurPos, TRUE);
		}
	}
	{
		pInfoDlg->ShowWindow(SW_HIDE);
		bInfoDlgShowing = false;
	}
	SaveConfig();
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

void CpermoDlg::ShowBand(void)
{
	if (NULL == pcoControl)
	{
		pcoControl = new CNProgressBar(this);
		pcoControl->SetFontSize(nBandFontSize);
		SetBandWidth(nBandWidth);
		SetBandHeight(nBandHeight);
	}
	if (bShowBand)
	{
		bShowBand = false;
		m_Menu.CheckMenuItem(IDM_SHOWBAND, MF_BYCOMMAND | MF_UNCHECKED);
	}
	else
	{
		bShowBand = true;
		m_Menu.CheckMenuItem(IDM_SHOWBAND, MF_BYCOMMAND | MF_CHECKED);
	}
	if (pcoControl->IsControlSuccessfullyCreated())
	{
		pcoControl->Show(bShowBand);
	}
}

// 显示或者隐藏悬浮窗（标尺中调用）
void CpermoDlg::ShowHideWindow(void)
{
	if (IsWindowVisible())
	{
		ShowWindow(SW_HIDE);
		bIsWndVisable = false;
	}
	else
	{
		ShowWindow(SW_SHOW);
		bIsWndVisable = true;
	}
}

LRESULT CpermoDlg::OnReconnect(WPARAM wparam,LPARAM lparam)
{
	StopCapture();
	StartCapture();
	return 0;
}

LRESULT CpermoDlg::OnBandMenu(WPARAM wparam,LPARAM lparam)
{
	SetForegroundWindow();
	CPoint p;
	//传递过来的坐标为相对于窗口左上角的坐标，WM_CONTEXTMENU传递过来的是屏幕坐标
	GetCursorPos(&p);//鼠标点的屏幕坐标
	int nID = m_BandMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, p.x, p.y, this);
	switch (nID)
	{
	//显示CPU占用信息（默认）
	case IDM_BANDSHOWCPU:
		{
			mm_Timer.KillTimer();
			if (IfExist(0))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_UNCHECKED); 
				RemoveBandShow(0);
				bBandShowCpu = false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPU, MF_BYCOMMAND | MF_CHECKED); 
				vBandShow.push_back(0);
				bBandShowCpu = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	//显示内存占用信息
	case IDM_BANDSHOWMEM:
		{
			mm_Timer.KillTimer();
			if (IfExist(1))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWMEM, MF_BYCOMMAND | MF_UNCHECKED);
				RemoveBandShow(1);
				bBandShowMem =false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWMEM, MF_BYCOMMAND | MF_CHECKED);
				vBandShow.push_back(1);
				bBandShowMem = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	//显示网络占用信息
	case IDM_BANDSHOWNETDOWN:
		{
			mm_Timer.KillTimer();
			if (IfExist(2))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETDOWN, MF_BYCOMMAND | MF_UNCHECKED); 
				RemoveBandShow(2);
				bBandShowNetDown = false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETDOWN, MF_BYCOMMAND | MF_CHECKED);
				vBandShow.push_back(2);
				bBandShowNetDown = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	case IDM_BANDSHOWNETUP:
		{
			mm_Timer.KillTimer();
			if (IfExist(3))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETUP, MF_BYCOMMAND | MF_UNCHECKED);
				RemoveBandShow(3);
				bBandShowNetUp = false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWNETUP, MF_BYCOMMAND | MF_CHECKED);
				vBandShow.push_back(3);
				bBandShowNetUp = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	case IDM_BANDSHOWDISKTEM:
		{
			mm_Timer.KillTimer();
			if (IfExist(4))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWDISKTEM, MF_BYCOMMAND | MF_UNCHECKED);
				RemoveBandShow(4);
				bBandShowDiskTem = false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWDISKTEM, MF_BYCOMMAND | MF_CHECKED);
				vBandShow.push_back(4);
				bBandShowDiskTem = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	case IDM_BANDSHOWCPUTEM:
		{
			mm_Timer.KillTimer();
			if (IfExist(5))
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPUTEM, MF_BYCOMMAND | MF_UNCHECKED);
				RemoveBandShow(5);
				bBandShowCpuTem = false;
			}
			else
			{
				m_BandMenu.CheckMenuItem(IDM_BANDSHOWCPUTEM, MF_BYCOMMAND | MF_CHECKED);
				vBandShow.push_back(5);
				bBandShowCpuTem = true;
			}
			nNowBandShowIndex = 0;
			nCount = 0;
			mm_Timer.CreateTimer((DWORD)this,1000,TimerCallbackTemp);
		}
		break;
	case IDM_BANDFONTSIZE12:
		SetBandFontSize(12);
		break;
	case IDM_BANDFONTSIZE13:
		SetBandFontSize(13);
		break;
	case IDM_BANDFONTSIZE14:
		SetBandFontSize(14);
		break;
	case IDM_BANDFONTSIZE15:
		SetBandFontSize(15);
		break;
	case IDM_BANDFONTSIZE16:
		SetBandFontSize(16);
		break;
	case IDM_BANDFONTSIZE17:
		SetBandFontSize(17);
		break;
	case IDM_BANDWIDTH50:
		{
			SetBandWidth(50);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDWIDTH60:
		{
			SetBandWidth(60);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDWIDTH70:
		{
			SetBandWidth(70);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDWIDTH80:
		{
			SetBandWidth(80);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDHEIGHT20:
		{
			SetBandHeight(20);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDHEIGHT25:
		{
			SetBandHeight(25);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDHEIGHT30:
		{
			SetBandHeight(30);
			pcoControl->Show(true);
		}
		break;
	case IDM_BANDHEIGHT35:
		{
			SetBandHeight(35);
			pcoControl->Show(true);
		}
		break;
	//隐藏或显示主界面
	case IDM_SHOWHIDEWND:
		ShowHideWindow();
		break;
	case IDM_EXIT:
		OnExit();
		break;
	default:
		break;
	}
	SaveConfig();
	return 0;
}
void CpermoDlg::SetBandFontSize(int bandFontSize)
{
	nBandFontSize = bandFontSize;
	pcoControl->SetFontSize(bandFontSize);
	pcoControl->Invalidate(FALSE);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE12, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE13, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE14, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE15, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE16, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE17, MF_BYCOMMAND | MF_UNCHECKED);
	m_BandMenu.CheckMenuItem(IDM_BANDFONTSIZE12 + bandFontSize - 12, MF_BYCOMMAND | MF_CHECKED);
}

void CpermoDlg::SetBandWidth(unsigned int bandwidth)
{
	pcoControl->SetWidth(bandwidth);
	m_BandMenu.CheckMenuItem(IDM_BANDWIDTH50 + (nBandWidth-50)/10, MF_BYCOMMAND | MF_UNCHECKED);
	nBandWidth = bandwidth;
	m_BandMenu.CheckMenuItem(IDM_BANDWIDTH50 + (nBandWidth-50)/10, MF_BYCOMMAND | MF_CHECKED);
}

void CpermoDlg::SetBandHeight(unsigned int bandheight)
{
	pcoControl->SetHeight(bandheight);
	m_BandMenu.CheckMenuItem(IDM_BANDHEIGHT20 + (nBandHeight-20)/5, MF_BYCOMMAND | MF_UNCHECKED);
	nBandHeight = bandheight;
	m_BandMenu.CheckMenuItem(IDM_BANDHEIGHT20 + (nBandHeight-20)/5, MF_BYCOMMAND | MF_CHECKED);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 函数名称：IsForegroundFullscreen
 * 功能说明：判断当前正在与用户交互的当前激活窗口是否是全屏的。
 * 参数说明：无
 * 返回说明：true：是。
             false：否。
 * 线程安全：是
 * 调用样例：IsForegroundFullscreen ()，表示判断当前正在与用户交互的当前激活窗口是否是

全屏的。
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
bool CpermoDlg::IsForegroundFullscreen(void)
{
	bool bFullscreen = false;//存放当前激活窗口是否是全屏的，true表示是，false表示不是
	HWND hWnd;
	RECT rcApp;
	RECT rcDesk;

	hWnd = ::GetForegroundWindow ();//获取当前正在与用户交互的当前激活窗口句柄

	if ((hWnd != ::GetDesktopWindow ()) && (hWnd != ::GetShellWindow ()))//如果当前激活窗口不是桌面窗口，也不是控制台窗口
	{
		::GetWindowRect (hWnd, &rcApp);//获取当前激活窗口的坐标
		::GetWindowRect (::GetDesktopWindow(), &rcDesk);//根据桌面窗口句柄，获取整个屏幕的坐标

		if (rcApp.left <= rcDesk.left && //如果当前激活窗口的坐标完全覆盖住桌面窗口，就表示当前激活窗口是全屏的
			rcApp.top <= rcDesk.top &&
			rcApp.right >= rcDesk.right &&
			rcApp.bottom >= rcDesk.bottom)
		{

			TCHAR szTemp[100];

			if (::GetClassName (hWnd, szTemp, sizeof (szTemp)) > 0)//如果获取当前激活窗口的类名成功
			{
				if (wcscmp (szTemp, _T("WorkerW")) != 0)//如果不是桌面窗口的类名，就认为当前激活窗口是全屏窗口
					bFullscreen = true;
			}
			else bFullscreen = true;//如果获取失败，就认为当前激活窗口是全屏窗口
		}
	}//如果当前激活窗口是桌面窗口，或者是控制台窗口，就直接返回不是全屏

	return bFullscreen;
}

bool CpermoDlg::IfExist(int nVal)
{
	vector<int>::iterator iter = find(vBandShow.begin(), vBandShow.end(), nVal);
	if (vBandShow.end() == iter)
	{
		return false;
	}
	return true;
}

void CpermoDlg::RemoveBandShow(int nVal)
{
	vector<int>::iterator iter;
	for(iter=vBandShow.begin();iter!=vBandShow.end();)
	{  
		if (nVal == (*iter))
		{
			iter = vBandShow.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void CpermoDlg::GetDiskTem(void)
{
	hres = pSvc->ExecQuery(
		bstr_t("WQL"), 
		bstr_t("SELECT * FROM MSStorageDriver_ATAPISmartData"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		NULL,
		&pEnumerator);

	int nTemperature = 0;
	//int nTotalTime = 0;

	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
			&pclsObj, &uReturn);

		if(0 == uReturn)
		{
			break;
		}
		VARIANT vtProp;
		VariantInit(&vtProp);

		// Get the value of the VendorSpecific property
		hr = pclsObj->Get(L"VendorSpecific", 0, &vtProp, 0, 0);
		
		if (vtProp.vt == VT_ARRAY|VT_UI1 )
		{
			SAFEARRAY *pIn;
			pIn = vtProp.parray;
			VARTYPE vt;
			UINT dim;
			SafeArrayGetVartype(pIn,&vt); //Data type
			dim = SafeArrayGetDim(pIn); //Dimension
			long LBound; //The lower bound
			long UBound; //Upper bound
			SafeArrayGetLBound(pIn,1,&LBound); //To obtain lower bounds
			SafeArrayGetUBound(pIn,1,&UBound); //To obtain upper bounds
			BYTE *pdata = new BYTE[UBound-LBound+1];
			ZeroMemory(pdata,UBound-LBound+1);

			BYTE *buf;
			SafeArrayAccessData(pIn, (void **)&buf);
			memcpy(pdata,buf,UBound-LBound+1);
			SafeArrayUnaccessData(pIn);

			BYTE* pTemp = pdata+2;
			for(int i=2;i<UBound-LBound+1;i+=12)
			{
				pTemp = pdata+i;
				if (*pTemp == 0xc2)
				{
					//Beep(1000,200);
					nTemperature = *(pTemp+5);
				}

				//总的使用时间
				// 				if (*pTemp == 0x09)
				// 				{
				// 					//Beep(1000,200);
				// 					nTotalTime = (*(pTemp+5)) + (*(pTemp+6)<<8);
				// 				}

			}
			delete pdata;
		}
		//Release objects not owned
		pclsObj->Release();
		nTempDisk = nTemperature;
		VariantClear(&vtProp);
	}
	// All done.
	pEnumerator->Release();
}

void CpermoDlg::StopCapture(void)
{
	g_bCapture = false;
	WaitForSingleObject(g_hCaptureThread, INFINITE);
}

void CpermoDlg::StartCapture(void)
{
	g_bCapture = true;
	g_hCaptureThread = CreateThread(0, 0, CaptureThread, 0, 0, 0);
}

void CpermoDlg::DeleteFiles(void)
{
	TCHAR direc[256];
	::GetCurrentDirectory(256, direc);//获取当前目录函数
	TCHAR temp[256];
	wsprintf(temp, _T("%s\\npf.sys"), direc);
	DeleteFile(temp);
	wsprintf(temp, _T("%s\\wpcap.dll"), direc);
	DeleteFile(temp);
	wsprintf(temp, _T("%s\\Packet.dll"), direc);
	DeleteFile(temp);
	wsprintf(temp, _T("%s\\WinRing0.sys"), direc);
	DeleteFile(temp);
}

// win7以及以上系统添加自启动，通过任务计划
void CpermoDlg::AddWin7SchTasks(void)
{
	TCHAR Location[MAX_PATH];
	TCHAR cmd[255];

	DWORD TapLocLen = 0;

	TapLocLen = GetCurrentDirectory(sizeof(Location),Location);

	if (TapLocLen == 0) 
	{
		printf("GetCurrentDirectory failed!  Error is %d \n", GetLastError());
		return ;
	}
	
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(NULL, szModule, _MAX_PATH);//得到本程序自身的全路径

	wsprintf(cmd,_T("/C schtasks /Create /tn \"autorun permo\" /tr \"\\\"%s\\\"\" /sc onlogon /rl HIGHEST /F"), szModule);  

	SHELLEXECUTEINFO sei; 
	ZeroMemory (&sei, sizeof (SHELLEXECUTEINFO)); 
	sei.cbSize = sizeof (SHELLEXECUTEINFO); 
	sei.lpVerb = _T("OPEN");
	sei.lpFile = _T("cmd.exe");
	sei.lpParameters = cmd;
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&sei); 
	WaitForSingleObject(sei.hProcess, INFINITE);
	return;
}

void CpermoDlg::DelWin7SchTasks(void)
{
	TCHAR Location[MAX_PATH];
	TCHAR cmd[255];

	DWORD TapLocLen = 0;

	TapLocLen = GetCurrentDirectory(sizeof(Location),Location);

	if (TapLocLen == 0) 
	{
		printf("GetCurrentDirectory failed!  Error is %d \n", GetLastError());
		return ;
	}

	wcscpy(cmd, _T("/C schtasks /delete /tn \"autorun permo\" /F"));

	SHELLEXECUTEINFO sei; 
	ZeroMemory (&sei, sizeof (SHELLEXECUTEINFO)); 
	sei.cbSize = sizeof (SHELLEXECUTEINFO); 
	sei.lpVerb = _T("OPEN");
	sei.lpFile = _T("cmd.exe");
	sei.lpParameters = cmd;
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&sei); 
	WaitForSingleObject(sei.hProcess, INFINITE);
	return;
}

bool CpermoDlg::IsIntel(void)
{
	char    OEMString[13];
	_asm
	{       
		mov     eax,0       
		cpuid       
		mov     DWORD     PTR     OEMString,ebx       
		mov     DWORD     PTR     OEMString+4,edx       
		mov     DWORD     PTR     OEMString+8,ecx       
		mov     BYTE     PTR     OEMString+12,0       
	}       
	USES_CONVERSION; 
	CString strTmp = A2T(OEMString);
/*	MessageBox(strTmp);*/
	if (-1 == strTmp.Find(_T("Intel")))
	{
		gIsMsr = FALSE;
		return false;
	}
	gIsMsr = TRUE;
	return true;
}


DWORD CpermoDlg::GetCpuInfo(DWORD veax)
{
	DWORD deax, debx, decx, dedx;
	__asm
	{
		push eax;
		push ebx;
		push ecx;
		push edx;

		mov eax, veax;
		cpuid;
		mov deax, eax;
		mov debx, ebx;
		mov decx, ecx;
		mov dedx, edx;

		pop edx;
		pop ecx;
		pop ebx;
		pop eax;
	}
	return deax;
}

//读取msr寄存器
BOOL WINAPI Rdmsr(DWORD index, PDWORD eax, PDWORD edx)
{
	if(gHandle2 == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	if(eax == NULL || edx == NULL || gIsMsr == FALSE)
	{
		return FALSE;
	}
	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	BYTE	outBuf[8] = {0};
	result = DeviceIoControl(
		gHandle2,
		IOCTL_OLS_READ_MSR,
		&index,
		sizeof(index),
		&outBuf,
		sizeof(outBuf),
		&returnedLength,
		NULL
		);
	if(result)
	{
		memcpy(eax, outBuf, 4);
		memcpy(edx, outBuf + 4, 4);
	}
	if(result)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CpermoDlg::GetCpuTemp(void)
{
	bool bSupportDts = false;
	DWORD eax=0,edx=0;
	DWORD eax_in;
	DWORD eax_out;
	int activationTemperature;
	//ULONG result;
	int nMask = 1;
	int nTem=0;
	//以eax=0执行CPUID指令得到支持的最大指令数 如果小于6一定不支持dts
	eax_in= 0;
	eax_out= GetCpuInfo(eax_in);
	if (eax_out < 6)
	{
		bSupportDts = false;
	}
	else
	{
		//以eax=6执行CPUID指令，eax 第一位是否为1，如果为1表示CPU支持DTS
		eax_in= 6;
		eax_out= GetCpuInfo(eax_in);
		if (1 == (eax_out&0x00000001))
		{
			bSupportDts = true;
		}
		else
		{
			bSupportDts = false;
		}
	}
	if (!bSupportDts)
	{
		return;
	}
	//以eax=1执行CPUID指令, 将得到CPU Signature
	eax_in= 1;
	eax_out= GetCpuInfo(eax_in);
	// signature 为 0x6f1， 0x6f0的 CPU DTS 值直接代表当前温度而不用Tjunction 相减.
	//而 signature 小于等于 0x6f4 的 Tjunction 一直为100
	if (0x6f1 == eax_out || 0x6f0 == eax_out)
	{
		for (int i=0; i<processor_count_; i++)
		{
			SetThreadAffinityMask(GetCurrentThread(),nMask);
			Rdmsr(0x19c,&eax,&edx);
			nTem += ((eax&0x007f0000)>>16);
			nMask *= 2;
		}
		nTem /= processor_count_;
	}
	else
	{
		if (eax_out <= 0x6f4)
		{
			activationTemperature = 100;
		}
		else
		{
			Rdmsr(0x1a2,&eax,&edx);
			activationTemperature= (eax&0x007f0000)>>16;
			if (0 == activationTemperature)
			{
				Rdmsr(0xee,&eax,&edx);
				if (1 == ((eax&0x20000000)>>30))
				{
					activationTemperature = 85;
				}
				else
				{
					activationTemperature = 100;
				}
			}
		}
		for (int i=0; i<processor_count_; i++)
		{
			SetThreadAffinityMask(GetCurrentThread(),nMask);
			Rdmsr(0x19c,&eax,&edx);
			nTem += (activationTemperature-((eax&0x007f0000)>>16));
			nMask *= 2;
		}
		nTem /= processor_count_;
	}
	nTempCpu = nTem;
}

void CpermoDlg::get_processor_number(void)
{
	SYSTEM_INFO info;  
	GetSystemInfo(&info);  
	processor_count_ =  (int)info.dwNumberOfProcessors;
}

void CpermoDlg::TimerCallbackTemp(DWORD dwUser)
{
	CpermoDlg* p = (CpermoDlg *)dwUser; 
	if (gIsMsr)
	{
		p->GetCpuTemp();
	}

	if (!(p->bAutoHide && p->rCurPos.left < 0))
	{
		//获取CPU占用
		::GetSystemTimes(&(p->idleTime), &(p->kernelTime), &(p->userTime));
		int idle = (int)CompareFileTime(p->preidleTime, p->idleTime);
		int kernel = (int)CompareFileTime(p->prekernelTime, p->kernelTime);
		int user = (int)CompareFileTime(p->preuserTime, p->userTime);
		int cpu = (kernel + user - idle) * 100 / (kernel + user);
		//int cpuidle = (idle)* 100 / (kernel + user);
		p->preidleTime = p->idleTime;
		p->prekernelTime = p->kernelTime;
		p->preuserTime = p->userTime;
		if (cpu < 0)
		{
			cpu = -cpu;
		}
		p->nCPU = cpu;
		p->strCPU.Format(_T("%d%%"), p->nCPU);
	}
	{
		Lock();
		// Update Rate
		for(int i = 0; i < vecProInfo.size(); i++)
		{
			CProInfo *item = vecProInfo[i];

			item->prevTxRate = item->txRate;
			item->prevRxRate = item->rxRate;

			item->txRate = 0;
			item->rxRate = 0;
		}
		Unlock();
		p->fNetDown = 0;
		p->fNetUp = 0;
		for (int i=0; i<vecProInfo.size(); i++)
		{
			p->fNetDown += vecProInfo[i]->prevRxRate;
			p->fNetUp += vecProInfo[i]->prevTxRate;
			// 		printf("puid:%d  pid:%d\n", vProInfo[i].puid, vProInfo[i].pid);
			// 		printf("down:%d.%dK/s--", vProInfo[i].prevTxRate / 1024, (vProInfo[i].prevTxRate % 1024 + 51) / 108);
			// 		printf("up:%d.%dK/s\n", vProInfo[i].prevRxRate / 1024, (vProInfo[i].prevRxRate % 1024 + 51) / 108);
		}
		p->fNetDown /= 1024;
		p->fNetUp /= 1024;
	}
	if (!(p->bAutoHide && p->rCurPos.right > p->rWorkArea.right))
	{
		//获取内存使用率
		MEMORYSTATUSEX memStatex;
		memStatex.dwLength = sizeof(memStatex);
		::GlobalMemoryStatusEx(&memStatex);
		p->nMem = memStatex.dwMemoryLoad;
		p->strMem.Format(_T("%d%%"), p->nMem);
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
	if ((p->bShowBand && p->bBandShowDiskTem) || p->bInfoDlgShowing)
	{
		p->GetDiskTem();
	}

	CString strBandNetUp;
	CString strBandNetDown;

	if (p->fNetUp >= 1000)
	{
		p->strNetUp.Format(_T("%.2fMB/S"), p->fNetUp/1024.0);
		strBandNetUp.Format(_T("U:%.2fM/S"), p->fNetUp/1024.0);
	}
	else
	{
		if (p->fNetUp < 100)
		{
			p->strNetUp.Format(_T("%.1fKB/S"), p->fNetUp);
			strBandNetUp.Format(_T("U:%.1fK/S"), p->fNetUp);
		}
		else
		{
			p->strNetUp.Format(_T("%.0fKB/S"), p->fNetUp);
			strBandNetUp.Format(_T("U:%.0fK/S"), p->fNetUp);
		}
	}
	if (p->fNetDown >= 1000)
	{
		p->strNetDown.Format(_T("%.2fMB/S"), p->fNetDown/1024.0);
		strBandNetDown.Format(_T("D:%.2fM/S"), p->fNetDown/1024.0);
	}
	else
	{
		if (p->fNetDown < 100)
		{
			p->strNetDown.Format(_T("%.1fKB/S"), p->fNetDown);
			strBandNetDown.Format(_T("D:%.1fK/S"), p->fNetDown);
		}
		else
		{
			p->strNetDown.Format(_T("%.0fKB/S"), p->fNetDown);
			strBandNetDown.Format(_T("D:%.0fK/S"), p->fNetDown);
		}
	}
	if (p->bShowBand)
	{
		if (p->vBandShow.size() == 0)
		{
			nBandShow = 100;
			p->nCount = 0;
		}
		else
		{
			if (p->nCount%4 == 0)
			{
				nBandShow = p->vBandShow[p->nNowBandShowIndex++];
				if (p->vBandShow.size() == p->nNowBandShowIndex)
				{
					p->nNowBandShowIndex = 0;
				}
				p->nCount = 0;
			}
			p->nCount++;
		}
		if (0 == nBandShow)
		{
			p->pcoControl->SetPosEx(p->nCPU);
			p->pcoControl->ShowMyText(p->strCPU, true);
		}
		else if (1 == nBandShow)
		{
			p->pcoControl->SetPosEx(p->nMem);
			p->pcoControl->ShowMyText(p->strMem, true);
		}
		else if (2 == nBandShow)
		{
			p->pcoControl->SetPosEx(0);
			p->pcoControl->ShowMyText(strBandNetDown, true);
		}
		else if (3 == nBandShow)
		{
			p->pcoControl->SetPosEx(0);
			p->pcoControl->ShowMyText(strBandNetUp, true);
		}
		else if (4 == nBandShow)
		{
			CString strDiskTem;
			strDiskTem.Format(_T("硬盘:%d°"), nTempDisk);
			p->pcoControl->SetPosEx(0);
			p->pcoControl->ShowMyText(strDiskTem, true);
		}
		else if (5 == nBandShow)
		{
			CString strCpuTem;
			strCpuTem.Format(_T("CPU:%d°"), nTempCpu);
			p->pcoControl->SetPosEx(0);
			p->pcoControl->ShowMyText(strCpuTem, true);
		}
		else
		{
			p->pcoControl->SetPosEx(0);
			p->pcoControl->ShowMyText(_T("未选择"), true);
		}
	}
	p->Invalidate(FALSE);
}
