// MFNetTraffic.h: interface for the MFNetTraffic class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_MFNETTRAFFIC_H__9CA9C41F_F929_4F26_BD1F_2B5827090494__INCLUDED_)
#define AFX_MFNETTRAFFIC_H__9CA9C41F_F929_4F26_BD1F_2B5827090494__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>


class MFNetTraffic  
{
public:
	enum TrafficType //流量类型
	{
		AllTraffic		= 388,//总的流量
		IncomingTraffic	= 264,//输入流量
		OutGoingTraffic	= 506 //输出流量
	};

	void	SetTrafficType(int trafficType);		//设置流量类型
	DWORD	GetInterfaceTotalTraffic(int index);	//得到index索引接口的总流量
	BOOL	GetNetworkInterfaceName(CString *InterfaceName, int index);//得到网络接口名字
	int		GetNetworkInterfacesCount();			//得到接口的数目
	double	GetTraffic(int interfaceNumber);		//得到流量

	DWORD	GetInterfaceBandwidth(int index);		//得到index索引接口的带宽
	MFNetTraffic();
	virtual ~MFNetTraffic();
private:
	BOOL		GetInterfaces();
	double		lasttraffic;
	CStringList Interfaces;		
	CList < DWORD, DWORD &>		Bandwidths;	//带宽
	CList < DWORD, DWORD &>		TotalTraffics;//总的流量
	int CurrentInterface;
	int CurrentTrafficType;
};

#endif // !defined(AFX_MFNETTRAFFIC_H__9CA9C41F_F929_4F26_BD1F_2B5827090494__INCLUDED_)
