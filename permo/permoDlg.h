// permoDlg.h : 头文件
//

#pragma once
#include "MFNetTraffic.h"
#include "MenuEx.h"



// CpermoDlg 对话框
class CpermoDlg : public CDialog
{
// 构造
public:
	CpermoDlg(CWnd* pParent = NULL);	// 标准构造函数

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
	DECLARE_MESSAGE_MAP()

public:
	int SelectedInterface;
	void InitSize(BOOL bInit);								//初始化窗口大小和位置
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);	
	void DrawBackground(CDC* pDC);							//绘制界面背景
	void DrawInfo(CDC* pDC);								//绘制界面信息
	// CPU占用
	unsigned int nCPU;
	// 内存占用
	unsigned int nMem;
	double fNetUp;											//上传速度
	double fNetDown;										//下载速度

	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;

	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	MEMORYSTATUSEX memStatex;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnTopmost();
	BOOL bTopmost;
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	CMenuEx m_Menu;				//右键弹出菜单
	CMenuEx m_SubMenu_Skin;			//右键弹出二级菜单(皮肤风格二级菜单)
	unsigned int nSkin;			//皮肤编号
	afx_msg void OnGreen();
	afx_msg void OnBlue();
	afx_msg void OnBlack();
	afx_msg void OnRed();
	afx_msg void OnOrange();
	MFNetTraffic m_cTrafficClassUp;
	MFNetTraffic m_cTrafficClassDown;
	// 总的上传流量
	DWORD total_net_up;
	// 总的下载流量
	DWORD total_net_down;
	BOOL isOnline;
	// 工作区的大小（除去任务栏）
	RECT rWorkArea;
	afx_msg void OnMove(int x, int y);
	// 窗口当前位置
	RECT rCurPos;
	afx_msg void OnExit();
	void InitPopMenu(int nCount);
	MENUITEM mi1;
	MENUITEM mi2;
	MENUITEM mi3;
	MENUITEM mi4;
	MENUITEM mi5;
	MENUITEM mi6;
	MENUITEM mi7;
	MENUITEM mi8;
	MENUITEM mi9;
	//BOOL GetNetStatus();
	BOOL OpenConfig();
	BOOL SaveConfig();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	BOOL bAutoHide;
	void OnAutoHide(void);
};
