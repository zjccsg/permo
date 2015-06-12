// Copyright (C) 2012-2014 F32 (feng32tc@gmail.com)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

#include "stdafx.h"
#include "Utils.h"
#include "PcapNetFilter.h"

bool PcapNetFilter::Init()
{
    // Load wpcap.dll
	_hWinPcap = ::LoadLibrary(TEXT("wpcap.dll"));

    if( _hWinPcap != NULL )
    {
		pcap_open_live   = (pcap_open_live_proc)   ::GetProcAddress(_hWinPcap, "pcap_open_live");
		pcap_close       = (pcap_close_proc)       ::GetProcAddress(_hWinPcap, "pcap_close");
		pcap_findalldevs = (pcap_findalldevs_proc) ::GetProcAddress(_hWinPcap, "pcap_findalldevs");
		pcap_freealldevs = (pcap_freealldevs_proc) ::GetProcAddress(_hWinPcap, "pcap_freealldevs");
		pcap_next_ex     = (pcap_next_ex_proc)     ::GetProcAddress(_hWinPcap, "pcap_next_ex");

        if( pcap_open_live == NULL ||
            pcap_close == NULL ||
            pcap_findalldevs == NULL ||
            pcap_freealldevs == NULL ||
            pcap_next_ex == NULL )
        {
            // "wpcap.dll" has an unexpected version
			::FreeLibrary(_hWinPcap);
			AfxMessageBox(_T("an unexpected version"));
            return false;
        }
    }
    else
    {
// 		DWORD err = GetLastError();
// 		CString strerr;
// 		strerr.Format(_T("load dll failed %u"), err);
// 		AfxMessageBox(strerr);
        return false;
    }

    _fp = 0;

    return true;
}
void PcapNetFilter::End()
{
    if( _fp != 0 )
    {
        pcap_close(_fp);
    }

	::FreeLibrary(_hWinPcap);
}

int PcapNetFilter::FindDevices()
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Get All Devices
    if (pcap_findalldevs(&_devices, errbuf) == -1)
    {
        return -1;
    }

    // Get Number of Devices
    _numDevices = 0;

    for(pcap_if_t *d = _devices; d; d = d->next)
    {
        _numDevices += 1;
    }

    return _numDevices;
}

TCHAR *PcapNetFilter::GetName(int i) // Return ANSI string
{
    static char name[256];
    static TCHAR tName[256];

    // Get the Name of Npf Device
    pcap_if_t *dev = _devices;

    for(int t = 0; t < i; t++)
    {
        dev = dev->next;
    }

    strcpy_s(name, sizeof(name), dev->name);
    Utils::AnsiToUtf16(name, tName, 256);

    // Scan Real Adapters
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (PIP_ADAPTER_INFO) malloc(sizeof(IP_ADAPTER_INFO));

    // - first call
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
    {
        free(pAdapterInfo);
        pAdapterInfo = (PIP_ADAPTER_INFO) malloc(ulOutBufLen);
    }

    GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);

    // - traverse the list
    for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
    {
        char *adapterName = pAdapter->AdapterName;

        // Optimize result if possible
        if( strstr(name, adapterName) != NULL )
        {
            strcpy_s(name, sizeof(name), pAdapter->Description);
            Utils::AnsiToUtf16(name, tName, 256);
        }
    }

    return tName;
}

bool PcapNetFilter::Select(int i)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Get the Name of Npf Device
    pcap_if_t *dev = _devices;

    for(int t = 0; t < i; t++)
    {
        dev = dev->next;
    }

    // Open Device
    _fp = pcap_open_live(dev->name, 64, 0, 20, errbuf);

    if( _fp == NULL )
    {
        return false;
    }

    // Save its Mac Address
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (PIP_ADAPTER_INFO) malloc(sizeof(IP_ADAPTER_INFO));

    // - first call
	if (::GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
    {
        free(pAdapterInfo);
        pAdapterInfo = (PIP_ADAPTER_INFO) malloc(ulOutBufLen);
    }

	::GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);

    // - traverse the list
    for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
    {
        char *adapterName = pAdapter->AdapterName;

        if( strstr(dev->name, adapterName) != NULL )
        {
            memcpy(_macAddr, pAdapter->Address, 6);
        }
    }

    return true;
}

bool PcapNetFilter::ReConnect(int i)
{
    End();
    Sleep(20000);
    Init();
    FindDevices();
    return Select(i);
}

bool PcapNetFilter::Capture(PacketInfo *pi, bool *capture)
{
// #ifdef DEBUG
//     if (*capture)
//     {
//         pi->size = 1460;
//         pi->time_s = (int)time(0);
//         pi->time_us = 0;
//         pi->networkProtocol = NET_IPv4;
//         pi->trasportProtocol = TRA_TCP;
//         pi->dir = DIR_DOWN;
//         pi->local_port = 0;
//         pi->remote_port = 0;
// 
//         return true;
//     }
//     else
//     {
//         return true;
//     }
// #endif

    int res;
    struct pcap_pkthdr *header;
    const unsigned char *pkt_data;

    MacHeader   *mh = 0;
    PppoeHeader *ph = 0;
    IpHeader    *ih = 0;
    UdpHeader   *uh = 0;
    TcpHeader   *th = 0;

    // Capture one packet
    while( true )
    {
        res = pcap_next_ex(_fp, &header, &pkt_data);

        //Timeout
        if( res == 0 ) 
        {
            if( * capture )
            {
                continue; 
            }
            else
            {
                return true;
            }
        }

        // Error occurred
        if(res == -1)
        {
            return false;
        }

        // Filter Group Packets
        if( res > 0 )
        {
            mh = (MacHeader *)(pkt_data);

            if (mh->protocol == htons(0x8864)) // PPPoE session stage
            {
                ph = (PppoeHeader *)(pkt_data + sizeof(MacHeader));
                if (ph->ver != 1 || ph->type != 1 || ph->code != 0) // code = 0: session data
                {
                    continue; // Unknown PPPoE packet
                }

                ih = (IpHeader  *)
                    (pkt_data + sizeof(PppoeHeader) + sizeof(MacHeader));
                uh = (UdpHeader *)
                    (pkt_data + sizeof(PppoeHeader) + sizeof(MacHeader) + sizeof(IpHeader));
                th = (TcpHeader *)
                    (pkt_data + sizeof(PppoeHeader) + sizeof(MacHeader) + sizeof(IpHeader));
            }
            else
            {
                ih = (IpHeader  *)(pkt_data + sizeof(MacHeader));
                uh = (UdpHeader *)(pkt_data + sizeof(MacHeader) + sizeof(IpHeader));
                th = (TcpHeader *)(pkt_data + sizeof(MacHeader) + sizeof(IpHeader));
            }

            if( memcmp(_macAddr, mh->dst, 6) == 0 || 
                memcmp(_macAddr, mh->src, 6) == 0 )
            {
                break;
            }
        }
    }

    // Fill Packet Info
    //  - size and Time
    if (mh->protocol == htons(0x8864))
    {
        pi->size = header->len - sizeof(MacHeader) - sizeof(PppoeHeader);
    }
    else
    {
        pi->size = header->len - sizeof(MacHeader);
    }
    pi->time_s  = header->ts.tv_sec; 
    pi->time_us = header->ts.tv_usec;

    //  - protocol
    pi->networkProtocol  = NET_OTHER;
    pi->trasportProtocol = TRA_OTHER;

    if (mh->protocol == htons(0x0800) || // IPv4
        (mh->protocol == htons(0x8864) && ph->protocol == htons(0x0021))) // PPPoE IPv4
    {
        pi->networkProtocol = NET_IPv4;

        if( ih->protocol == 6 )
        {
            pi->trasportProtocol = TRA_TCP;
        }
        else if( ih->protocol == 17 )
        {
            pi->trasportProtocol = TRA_UDP;
        }
        else if( ih->protocol == 1 )
        {
            pi->trasportProtocol = TRA_ICMP;
        }
    }
    else if( mh->protocol == htons(0x0806))
    {
        pi->networkProtocol = NET_ARP;
    }
    else if( mh->protocol == htons(0x86DD))
    {
        pi->networkProtocol = NET_IPv6;
    }

    //  - port, dir and puid
    pi->dir = 0;
    pi->local_port = 0;
    pi->remote_port = 0;

    if( memcmp(_macAddr, mh->dst, 6) == 0 )
    {
        pi->dir = DIR_DOWN;

        if( pi->trasportProtocol == TRA_UDP )
        {
            pi->remote_port = ntohs(uh->src_port);
            pi->local_port  = ntohs(uh->dst_port);
        }
        else if( pi->trasportProtocol == TRA_TCP )
        {
            pi->remote_port = ntohs(th->src_port);
            pi->local_port  = ntohs(th->dst_port);
        }
    }
    else if( memcmp(_macAddr, mh->src, 6) == 0 )
    {
        pi->dir = DIR_UP;

        if( pi->trasportProtocol == TRA_UDP )
        {
            pi->remote_port = ntohs(uh->dst_port);
            pi->local_port  = ntohs(uh->src_port);
        }
        else if( pi->trasportProtocol == TRA_TCP )
        {
            pi->remote_port = ntohs(th->dst_port);
            pi->local_port  = ntohs(th->src_port);
        }
    }

    return true;
}