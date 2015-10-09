// permoDlg.h : 头文件
//

#pragma once
#include "InfoDlg.h"
#include "NProgressBar.h"
#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#include "MyTimer.h"
# pragma comment(lib, "wbemuuid.lib")

// CpermoDlg 对话框
class CpermoDlg : public CDialog
{
// 构造
public:
	CpermoDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CpermoDlg();

// 对话框数据
	enum { IDD = IDD_PERMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnBandMenu(WPARAM wparam,LPARAM lparam);
	afx_msg LRESULT OnReconnect(WPARAM wparam,LPARAM lparam);
	DECLARE_MESSAGE_MAP()

public:
	void InitSize();								//初始化窗口大小和位置
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);	
	void DrawBackground(CDC* pDC);							//绘制界面背景
	void DrawInfo(CDC* pDC);								//绘制界面信息
	// CPU占用
	unsigned int nCPU;
	// 内存占用
	unsigned int nMem;
	unsigned int nTrans;									//透明度默认255
	double fNetUp;											//上传速度
	double fNetDown;										//下载速度
	bool bInfoDlgShowing;									//明细窗口当前是否在显示

	bool bShowBand;			//是否显示标尺

	CString strNetUp;
	CString strNetDown;

	CString strCPU;
	CString strMem;

	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;

	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnTopmost();
	BOOL bTopmost;
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	CMenu m_Menu;				//右键弹出菜单
	CMenu m_SubMenu_Skin;			//右键弹出二级菜单(皮肤风格二级菜单)
	CMenu m_SubMenu_NetPort;
	CMenu m_SubMenu_ShowWay;		//明细窗口显示方式默认0鼠标移过显示1单击显示2不显示
	CMenu m_SubMenu_Trans;
	CMenu m_SubMenu_FontSize;
	afx_msg void OnGreen();
	afx_msg void OnBlue();
	afx_msg void OnBlack();
	afx_msg void OnRed();
	afx_msg void OnOrange();
	// 工作区的大小（除去任务栏）
	RECT rWorkArea;
	// 窗口当前位置
	CRect rCurPos;
	afx_msg void OnExit();
	void InitPopMenu(int nCount);
	//BOOL GetNetStatus();
	void OpenConfig();
	BOOL SaveConfig();
//	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	BOOL bAutoHide;
	void OnAutoHide(void);
	void OnTrans0(void);
	void OnTrans10(void);
	void OnTrans20(void);
	void OnTrans30(void);
	void OnTrans40(void);
	void OnTrans50(void);
	void OnTrans60(void);
	void OnTrans70(void);
	void OnTrans80(void);
	void OnTrans90(void);
	void SetAutoRun(void);
	void IfAutoRun(void);
	CInfoDlg *pInfoDlg;
	void CreateInfoDlg(void);
	void MoveInfoDlg(void);
	BOOL SetWorkDir(void);
	unsigned int nShowWay; //详情窗口弹出方式0默认悬停即弹出1单击鼠标左键弹出2为不弹出
	void SetShowWay(int nIndex);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	BOOL _bMouseTrack;
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	void ShowNetInfo(void);
	void SetFontSize(UINT fontSize);

private:
	CNProgressBar *pcoControl;
public:
	void ShowBand(void);
	// 显示或者隐藏悬浮窗（标尺中调用）
	void ShowHideWindow(void);
	void SetBandFontSize(int nBandFontSize);
	//标尺上文字大小
	int nBandFontSize;
	bool bLockWndPos;
	unsigned int nBandWidth;
	unsigned int nBandHeight;
	void SetBandWidth(unsigned int bandwidth);
	void SetBandHeight(unsigned int bandheight);
	bool IsForegroundFullscreen(void);
	bool bIsWndVisable;	//记录窗口当前是否处于可见状态
	bool bFullScreen;	//记录是否开启全屏免打扰
	bool bHideWndSides;	//隐藏悬浮窗两侧信息，仅显示中间的流量信息
	vector<int> vBandShow;
	bool IfExist(int nVal);
	void RemoveBandShow(int nVal);
	unsigned int nNowBandShowIndex;
	bool bBandShowCpu;
	bool bBandShowMem;
	bool bBandShowNetUp;
	bool bBandShowNetDown;
	bool bBandShowDiskTem;
	bool bBandShowCpuTem;
	unsigned int nCount;

	HRESULT hres;
	IWbemLocator *pLoc;
	IWbemServices *pSvc;
	IEnumWbemClassObject* pEnumerator;
	void GetDiskTem(void);
	void StopCapture(void);
	void StartCapture(void);
	bool bHadWinpcap;
	void DeleteFiles(void);
	// win7以及以上系统添加自启动，通过任务计划
	void AddWin7SchTasks(void);
	void DelWin7SchTasks(void);
	bool bIsWindowsVistaOrGreater;
	bool bShowOneSideInfo;
	bool IsIntel(void);
	void GetCpuTemp(void);
	DWORD GetCpuInfo(DWORD veax);
	void get_processor_number(void);
	CHighResolutionTimer mm_Timer;
	static void TimerCallbackTemp(DWORD dwUser);
	void ShowTempInfo(void);
};
