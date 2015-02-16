//Download by http://www.cctry.com
#include "stdafx.h"
#include "MFNetTraffic.h"
#include "float.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#include "winperf.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MFNetTraffic::MFNetTraffic()
{
	lasttraffic = 0.0;
	CurrentInterface = -1;
	CurrentTrafficType = AllTraffic;
	GetInterfaces();
}

MFNetTraffic::~MFNetTraffic()
{
	
}


PERF_OBJECT_TYPE *FirstObject(PERF_DATA_BLOCK *dataBlock)
{
  return (PERF_OBJECT_TYPE *) ((BYTE *)dataBlock + dataBlock->HeaderLength);
}

PERF_OBJECT_TYPE *NextObject(PERF_OBJECT_TYPE *act)
{
  return (PERF_OBJECT_TYPE *) ((BYTE *)act + act->TotalByteLength);
}

PERF_COUNTER_DEFINITION *FirstCounter(PERF_OBJECT_TYPE *perfObject)
{
  return (PERF_COUNTER_DEFINITION *) ((BYTE *) perfObject + perfObject->HeaderLength);
}

PERF_COUNTER_DEFINITION *NextCounter(PERF_COUNTER_DEFINITION *perfCounter)
{
  return (PERF_COUNTER_DEFINITION *) ((BYTE *) perfCounter + perfCounter->ByteLength);
}

PERF_COUNTER_BLOCK *GetCounterBlock(PERF_INSTANCE_DEFINITION *pInstance)
{
  return (PERF_COUNTER_BLOCK *) ((BYTE *)pInstance + pInstance->ByteLength);
}

PERF_INSTANCE_DEFINITION *FirstInstance (PERF_OBJECT_TYPE *pObject)
{
  return (PERF_INSTANCE_DEFINITION *)  ((BYTE *) pObject + pObject->DefinitionLength);
}

PERF_INSTANCE_DEFINITION *NextInstance (PERF_INSTANCE_DEFINITION *pInstance)
{
  PERF_COUNTER_BLOCK  *pCtrBlk = GetCounterBlock(pInstance);

  return (PERF_INSTANCE_DEFINITION *) ((BYTE *)pInstance + pInstance->ByteLength + pCtrBlk->ByteLength);
}

char *WideToMulti(wchar_t *source, char *dest, int size)
{
  WideCharToMultiByte(CP_ACP, 0, source, -1, dest, size, 0, 0);

  return dest;
}
/*
Window NT/2000: When calling the RegQueryValueEx function with hKey set to the 
HKEY_PERFORMANCE_DATA handle and a value string of a specified object, the 
returned data structure sometimes has unrequested objects. Don't be surprised;
this is normal behavior. When calling the RegQueryValueEx function, you should 
always expect to walk the returned data structure to look for the requested object. 
*/
//得到接口号为interfaceNumber的接口流量
double MFNetTraffic::GetTraffic(int interfaceNumber)
{
	try
	{
#define DEFAULT_BUFFER_SIZE 40960L
		
		POSITION pos;
		CString InterfaceName;
		pos = Interfaces.FindIndex(interfaceNumber);
		if(pos==NULL)
			return 0.0;
		//得到当前的接口名字
		InterfaceName = Interfaces.GetAt(pos);
		
		//为性能数据缓冲
		unsigned char *data = new unsigned char [DEFAULT_BUFFER_SIZE];
		// 从RegQueryValueEx返回的值:本例中忽略改变量
		DWORD type;
		// 缓冲的尺寸
		DWORD size = DEFAULT_BUFFER_SIZE;
		// RegQueryValueEx返回的值
		DWORD ret;
		
		// 从网络对象(索引是510)查询性能数据
		/*
		HKEY_PERFORMANCE_DATA:
		Windows NT/2000/XP: Registry entries subordinate to this key allow you to 
		access performance data. The data is not actually stored in the registry; 
		the registry functions cause the system to collect the data from its source.
		*/
		/*
		*/
		while((ret = RegQueryValueEx(
			HKEY_PERFORMANCE_DATA, _T("510"), NULL, &type, data, &size)) != ERROR_SUCCESS) 
		{
			if(ret == ERROR_MORE_DATA) 
			{
				// 缓冲尺寸太小，增加内存分配
				size += DEFAULT_BUFFER_SIZE;		
				delete [] data;
				data = new unsigned char [size];
			} 
			else 
			{
				// 未定义的错误
				return 1;
			}
		}
		//性能数据块
		PERF_DATA_BLOCK *dataBlockPtr = (PERF_DATA_BLOCK *)data;
		
		//枚举链表中第一个对象
		PERF_OBJECT_TYPE *objectPtr = FirstObject(dataBlockPtr);
		
		//遍历链表 
		for(int a=0 ; a<(int)dataBlockPtr->NumObjectTypes ; a++) 
		{
			char nameBuffer[255];
		
			// 判断是否是网络对象索引号是510
			if(objectPtr->ObjectNameTitleIndex == 510) 
			{
				// 偏移变量
				DWORD processIdOffset = ULONG_MAX;
				// 找到第一个计数器
				PERF_COUNTER_DEFINITION *counterPtr = FirstCounter(objectPtr);
				// 遍历链表
				for(int b=0 ; b<(int)objectPtr->NumCounters ; b++) 
				{
					// 判断接收的数据类型是否是我们需要的
					if((int)counterPtr->CounterNameTitleIndex == CurrentTrafficType)
						processIdOffset = counterPtr->CounterOffset;
			
					// 下一个计数器
					counterPtr = NextCounter(counterPtr);
				}

				//数据类型不是我们需要的
				if(processIdOffset == ULONG_MAX) {
					delete [] data;
					return 1;
				}
				
				// 找到第一个实列(instance)
				PERF_INSTANCE_DEFINITION *instancePtr = FirstInstance(objectPtr);
				
				DWORD fullTraffic;
				DWORD traffic;
				//遍历整个实列
				for(int b=0 ; b<objectPtr->NumInstances ; b++) 
				{
					wchar_t *namePtr = (wchar_t *) ((BYTE *)instancePtr + instancePtr->NameOffset);		
					// 得到这个实列的PERF_COUNTER_BLOCK
					PERF_COUNTER_BLOCK *counterBlockPtr = GetCounterBlock(instancePtr);
					
					// 现在我们得到了接口的名字
					char *pName = WideToMulti(namePtr, nameBuffer, sizeof(nameBuffer));
					CString iName;
					//iName.Format(_T("%s"),pName);
					USES_CONVERSION;
					iName = A2W(pName);
					//or iName = A2T(pName)
				
					POSITION pos = TotalTraffics.FindIndex(b);
					if(pos!=NULL)
					{
						fullTraffic = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));
						TotalTraffics.SetAt(pos,fullTraffic);
					}

					// 如果当前的接口就是我们选择的接口
					if(InterfaceName == iName)
					{
						traffic = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));
						double acttraffic = (double)traffic;
						double trafficdelta;
						// 判断处理的接口是否是新的
						if(CurrentInterface != interfaceNumber)
						{
							lasttraffic = acttraffic;
							trafficdelta = 0.0;
							CurrentInterface = interfaceNumber;
						}
						else
						{
							trafficdelta = acttraffic - lasttraffic;
							lasttraffic = acttraffic;
						}
						delete [] data;
						return(trafficdelta);
					}
					
					//下一个实列
					instancePtr = NextInstance(instancePtr);
				}
			}
			
			// 下一个对象
			objectPtr = NextObject(objectPtr);
		}
		
		delete [] data;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}

//枚举安装的接口
BOOL MFNetTraffic::GetInterfaces()
{
	try
	{
#define DEFAULT_BUFFER_SIZE 40960L
		
		Interfaces.RemoveAll();
		unsigned char *data = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);
		DWORD type;
		DWORD size = DEFAULT_BUFFER_SIZE;
		DWORD ret;
		
		TCHAR s_key[4096];
		wsprintf( s_key , _T("%d") , 510 );
		//RegQueryValueEx的固定调用格式		
		while ((ret = RegQueryValueEx(HKEY_PERFORMANCE_DATA, s_key, 0, &type, data, &size)) != ERROR_SUCCESS) {
			while(ret == ERROR_MORE_DATA) 
			{
				size += DEFAULT_BUFFER_SIZE;
				data = (unsigned char*) realloc(data, size);
			} 
			if(ret != ERROR_SUCCESS)
			{
				return FALSE;
			}
		}
		//得到数据块		
		PERF_DATA_BLOCK	 *dataBlockPtr = (PERF_DATA_BLOCK *)data;
		//得到第一个对象
		PERF_OBJECT_TYPE *objectPtr = FirstObject(dataBlockPtr);
		
		for(int a=0 ; a<(int)dataBlockPtr->NumObjectTypes ; a++) 
		{
			char nameBuffer[255];
			if(objectPtr->ObjectNameTitleIndex == 510) 
			{
				DWORD processIdOffset = ULONG_MAX;
				PERF_COUNTER_DEFINITION *counterPtr = FirstCounter(objectPtr);
				
				for(int b=0 ; b<(int)objectPtr->NumCounters ; b++) 
				{
					if(counterPtr->CounterNameTitleIndex == 520)
						processIdOffset = counterPtr->CounterOffset;
					
					counterPtr = NextCounter(counterPtr);
				}
				
				if(processIdOffset == ULONG_MAX) {
					free(data);
					return 1;
				}
				
				PERF_INSTANCE_DEFINITION *instancePtr = FirstInstance(objectPtr);
				
				for(int b=0 ; b<objectPtr->NumInstances ; b++) 
				{
					wchar_t *namePtr = (wchar_t *) ((BYTE *)instancePtr + instancePtr->NameOffset);
					PERF_COUNTER_BLOCK *counterBlockPtr = GetCounterBlock(instancePtr);
					char *pName = WideToMulti(namePtr, nameBuffer, sizeof(nameBuffer));
					DWORD bandwith = *((DWORD *) ((BYTE *)counterBlockPtr + processIdOffset));				
					DWORD tottraff = 0;

					Interfaces.AddTail(CString(pName));
					Bandwidths.AddTail(bandwith);
					TotalTraffics.AddTail(tottraff);  // initial 0, just for creating the list
					
					instancePtr = NextInstance(instancePtr);
				}
			}
			objectPtr = NextObject(objectPtr);
		}
		free(data);
		
		return TRUE;
	}
	catch(...)
	{
		return FALSE;
	}
}

//返回安装的接口数
int MFNetTraffic::GetNetworkInterfacesCount()
{
	return Interfaces.GetCount()-1;
}


//返回接口名称
BOOL MFNetTraffic::GetNetworkInterfaceName(CString *InterfaceName, int index)
{
	POSITION pos = Interfaces.FindIndex(index);
	if(pos==NULL)
		return FALSE;

	InterfaceName->Format(_T("%s"),Interfaces.GetAt(pos));
	return TRUE;
}


//返回接口带宽
DWORD MFNetTraffic::GetInterfaceBandwidth(int index)
{
	POSITION pos = Bandwidths.FindIndex(index);
	if(pos==NULL)
		return 0;

	else 
		return Bandwidths.GetAt(pos) / 8;

}

//具体接口接收和发送的数据量
DWORD MFNetTraffic::GetInterfaceTotalTraffic(int index)
{
	DWORD		totaltraffic = 0;
	POSITION	pos;
	pos= TotalTraffics.FindIndex(index);
	if(pos!=NULL)
	{
		totaltraffic = TotalTraffics.GetAt(pos); 
		if(totaltraffic == 0.0)
		{
			GetTraffic(index);
			pos= TotalTraffics.FindIndex(index);
			if(pos!=NULL)
			{
				totaltraffic = TotalTraffics.GetAt(pos); 
			}
		}
	}

	return(totaltraffic);
}
void MFNetTraffic::SetTrafficType(int trafficType)
{
	CurrentTrafficType = trafficType;
}
