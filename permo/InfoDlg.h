#pragma once
#include "afxcmn.h"
#include <TlHelp32.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

bool comT(CProInfo* pProInfo);
// 自定义排序函数
bool SortByCpu(const CProInfo * v1, const CProInfo * v2);
bool SortByMem(const CProInfo * v1, const CProInfo * v2);

// CInfoDlg 对话框

class CInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CInfoDlg)

public:
	CInfoDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInfoDlg();

// 对话框数据
	enum { IDD = IDD_INFO_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
//	virtual BOOL DestroyWindow();
protected:
	virtual void PostNcDestroy();
	virtual void OnCancel();
public:
	CWnd *m_pParent;
	void DrawInfo(CDC * pDC);
	
	// 提升权限
	bool DebugPrivilege(bool bEnable);
	static uint64_t file_time_2_utc(const FILETIME* ftime);
	double get_cpu_usage(HANDLE hProcess, CProInfo* pProInfo);
	void GetProInfoVec(void);
	// 清理内存
	void FreeVec(void);
	void RemoveExitPro(void);
	// 获取进程完整路径 
	BOOL GetProcessFullPath(HANDLE hProcess, TCHAR * pszFullPath);
	BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	// 进程总数
	unsigned int nProNum;

};
