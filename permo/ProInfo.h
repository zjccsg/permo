#pragma once
typedef long long           int64_t;  
typedef unsigned long long  uint64_t;  

class CProInfo
{
public:
	CProInfo(void);
	~CProInfo(void);
	//The process identifier.
	DWORD id;
	//The name of the executable file for the process.
	TCHAR szExeFile[MAX_PATH];
	TCHAR szExeFilePath[MAX_PATH+1];
	//LARGE_INTEGER lastCPUTime;
	//LARGE_INTEGER lastCheckCPUTime;
	//上一次的时间
	int64_t last_time_;  
	int64_t last_system_time_; 
	int cpu;
	BOOL bExit;
	double mem;
	double netup;
	double netdown;
	HICON hIcon;
};
