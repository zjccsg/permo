#include "stdafx.h"

#include "NOperatingSystem.h"

CNOperatingSystem::CNOperatingSystem()
{
}

CNOperatingSystem::~CNOperatingSystem()
{
}


int CNOperatingSystem::GetOS()
{
	int nOs = -1;

	if (nOs == -1) // first time
	{
		OSVERSIONINFO vinfo;
		vinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			
		BOOL rslt = GetVersionEx(&vinfo);
			
		if (rslt)
		{
			switch (vinfo.dwPlatformId)
			{
			case VER_PLATFORM_WIN32_NT:
				switch (vinfo.dwMajorVersion)
				{
					case 3: // nt351
						ASSERT (0); // not supported						
						break;
						
					case 4: // nt4
						nOs = SBOS_NT4;
						break;
						
					case 5: // >= w2k
						switch (vinfo.dwMinorVersion)
						{
						case 0: // w2k
							nOs = SBOS_2K;
							break;
							
						case 1: // xp
							nOs = SBOS_XP;
							break;
							
						default: // > xp
							nOs = SBOS_XPP;
							break;
						}
						break;
						
						default: // > xp
							nOs = SBOS_XPP;
							break;
					}
					break;
					
					case VER_PLATFORM_WIN32_WINDOWS:
						ASSERT (vinfo.dwMajorVersion == 4);
						
						switch (vinfo.dwMinorVersion)
						{
						case 0: // nt4
							nOs = SBOS_95;
							break;
							
						case 10: // xp
							nOs = SBOS_98;
							break;
							
						case 90: // > xp
							nOs = SBOS_ME;
							break;
							
						default:
							ASSERT (0);
							break;
						}
						break;
						
						default:
							ASSERT (0);
							break;
			}
		}
	}	
	return nOs;
}