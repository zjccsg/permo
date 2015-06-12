#include <MMSYSTEM.H>   
#pragma comment(lib, "WINMM.LIB")  

class CHighResolutionTimer  
{  
	typedef void (*TIMERCALLBACK)(DWORD);  

private:  
	DWORD m_dwUser;  
	MMRESULT m_nIDTimer;  
	UINT m_uDelay;  
	TIMERCALLBACK m_pfnCallback;  

// 	void KillTimer()  
// 	{  
// 		if(m_nIDTimer != NULL)  
// 		{  
// 			timeKillEvent(m_nIDTimer);  
// 			m_nIDTimer = NULL;  
// 		}  
// 	}  

public:   
	void KillTimer()  
	{  
		if(m_nIDTimer != NULL)  
		{  
			timeKillEvent(m_nIDTimer);  
			m_nIDTimer = NULL;  
		}  
	}  

	CHighResolutionTimer()  
	{  
		m_nIDTimer = NULL;  
		m_uDelay = 0;  
		m_pfnCallback = 0;  
	}  

	virtual ~CHighResolutionTimer()  
	{  
		KillTimer();  
	}  

	static void CALLBACK TimeProc(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1, DWORD dw2)  
	{  
		// dwUser contains ptr to Timer object        
		CHighResolutionTimer * ptimer = (CHighResolutionTimer *) dwUser;  
		// Call user-specified callback and pass back user specified data  
		(ptimer->m_pfnCallback) (ptimer->m_dwUser);  
	}  

	BOOL CreateTimer(DWORD dwUser,UINT uDelay,TIMERCALLBACK lpTimeProc)  
	{  
		ASSERT(dwUser);  
		ASSERT(lpTimeProc);  

		m_dwUser = dwUser;  
		m_pfnCallback = lpTimeProc;       
		BOOL ret = FALSE;  
		KillTimer();  
		m_nIDTimer = timeSetEvent (uDelay, 0, TimeProc, (DWORD)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);  

		if(m_nIDTimer != NULL)  
			ret = TRUE;  
		return(ret);  
	}  
};  