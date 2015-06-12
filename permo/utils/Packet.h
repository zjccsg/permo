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

#ifndef PACKET_H
#define PACKET_H

#define NET_IPv4  0x0800
#define NET_ARP   0x0806
#define NET_IPv6  0x86DD
#define NET_OTHER 0x0000

#define TRA_TCP   6
#define TRA_UDP   17
#define TRA_ICMP  1
#define TRA_OTHER 0

#define DIR_UP    1
#define DIR_DOWN  2

typedef struct tagPacketInfo
{
    int     networkProtocol;
    int     trasportProtocol;

    int     local_port;    // Only for TCP & UDP
    int     remote_port;

    int     dir;     
    int     size;    // In Bytes

    int     time_s;  // Time in Secode
    int     time_us; // Time in Microseconds
} PacketInfo;

typedef struct tagPacketInfoEx
{
    // Part 1 -----------------------------------
    int     networkProtocol;
    int     trasportProtocol;

    int     local_port;    // Only for TCP & UDP
    int     remote_port;

    int     dir;     
    int     size;    // In Bytes

    int     time_s;  // Time in Secode
    int     time_us; // Time in Microseconds

    // Part 2 -----------------------------------
    int     pid;
    int     puid;    // Process UID
    int     pauid;   // Process Activity UID
    TCHAR   name[MAX_PATH];
    TCHAR   fullPath[MAX_PATH];
} PacketInfoEx;

#endif