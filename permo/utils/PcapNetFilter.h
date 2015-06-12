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

#ifndef PCAP_NET_FILTER_H
#define PCAP_NET_FILTER_H

#include "../Pcap/Pcap.h"

#include "Packet.h"

class PcapNetFilter
{
    #pragma region Protocol Headers

    #pragma pack(1)

    // MACv2 Header
    typedef struct tagMacHeader 
    {
        unsigned char  dst[6];
        unsigned char  src[6];
        unsigned short protocol;
    } MacHeader;

    /*
    00 26 18 b1 cb d3 dc d2 fc 98 64 f9 88 64 11 00 21 6c 05 ca 00 21
    ~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~ ~~~~~ ~~ ~~ ~~~~~ ~~~~~ ~~~~~
    Destination Addr  Source Address    Ethernet Type           PPP Protocol: 0x0021* (IP)
                                        0x8862 / 0x8864*
    ---------- MAC Packet ---------------------+-- PPPoE Packet -+-----
                                              Version / Type: 0x11*
                                                 Code: 0x00*
                                                    Session ID
                                                          Length (1482)
    */

    typedef struct tagPppoeHeader
    {
        unsigned ver      : 4;
        unsigned type     : 4;
        unsigned code     : 8;
        unsigned session  : 16;
        unsigned length   : 16;
        unsigned protocol : 16;
    } PppoeHeader;

    // IPv4 Header
    typedef struct tagIpHeader
    {
        unsigned ver      : 4;
        unsigned hdr_len  : 4;
        unsigned          : 8;
        unsigned len      : 16;

        unsigned          : 16;
        unsigned          : 3;
        unsigned          : 13;

        unsigned          : 8;
        unsigned protocol : 8;
        unsigned          : 16;

        unsigned src      : 32;
        unsigned dst      : 32;
    } IpHeader;

    // UdpHeader
    typedef struct tagUdpHeader
    {
        unsigned short src_port;
        unsigned short dst_port;
        unsigned short len;
        unsigned short checksum;
    } UdpHeader;

    // TcpHeader
    typedef struct tagTcpHeader
    {
        unsigned src_port      : 16;
        unsigned dst_port      : 16;

        unsigned               : 32;

        unsigned               : 32;

        unsigned data_offset   : 4;
        unsigned               : 6;
        unsigned flags         : 6;
        unsigned window        : 16;

        unsigned checksum      : 16;
        unsigned emergency_ptr : 16;
    } TcpHeader;

    #pragma pack()

    #pragma endregion

    #pragma region Pcap Function Pointer Definition

    typedef pcap_t *(* pcap_open_live_proc)(const char *, int, int, int, char *);
    typedef void    (* pcap_close_proc)(pcap_t *);
    typedef int     (* pcap_next_ex_proc)(pcap_t *, struct pcap_pkthdr **, const u_char **);
    typedef int        (* pcap_findalldevs_proc)(pcap_if_t **, char *);
    typedef void    (* pcap_freealldevs_proc)(pcap_if_t *);

    #pragma endregion

protected:
    unsigned char _macAddr[6];

    pcap_open_live_proc   pcap_open_live;
    pcap_close_proc       pcap_close;
    pcap_findalldevs_proc pcap_findalldevs;
    pcap_freealldevs_proc pcap_freealldevs;
    pcap_next_ex_proc     pcap_next_ex;

    HMODULE    _hWinPcap;

    pcap_t    *_fp;

    pcap_if_t *_devices;
    int        _numDevices;

public:
    bool Init();
    void End();

    int  FindDevices();
    TCHAR *GetName(int i);
    bool Select(int i);
    bool ReConnect(int i);

    bool Capture(PacketInfo *pi, bool *capture);
};

#endif