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
#include "PortCache.h"


PortCache::PortCache()
{
	::RtlZeroMemory(_tcpPortTable, sizeof(_tcpPortTable));
	::RtlZeroMemory(_udpPortTable, sizeof(_udpPortTable));
}

int PortCache::GetTcpPortPid(int port)
{
    if( _tcpPortTable[port] != 0 )
    {
        return _tcpPortTable[port];
    }
    else
    {
        // Rebuild Cache
        RebuildTcpTable();
        
        // Return
        return _tcpPortTable[port];
    }
}

int PortCache::GetUdpPortPid(int port)
{
    if( _udpPortTable[port] != 0 )
    {
        return _udpPortTable[port];
    }
    else
    {
        // Rebuild Cache
        RebuildUdpTable();

        // Return
        return _udpPortTable[port];
    }
}

void PortCache::RebuildTcpTable()
{
    // Clear the table
	RtlZeroMemory(_tcpPortTable, sizeof(_tcpPortTable));

    // Rebuild the table
    MIB_TCPTABLE_OWNER_PID table;
    table.dwNumEntries = sizeof(table) / sizeof(table.table[0]);

    DWORD tableSize = sizeof(table);
	if( GetExtendedTcpTable((void *)&table, &tableSize, 
        FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)== NO_ERROR )
    {
        for(unsigned int i = 0; i < table.dwNumEntries; i++)
        {
            _tcpPortTable[ntohs((unsigned short)table.table[i].dwLocalPort)] = 
                table.table[i].dwOwningPid;
        }
    }
}

void PortCache::RebuildUdpTable()
{
    // Clear the table
	::RtlZeroMemory(_udpPortTable, sizeof(_udpPortTable));

    // Rebuild the table
    MIB_UDPTABLE_OWNER_PID table;
    table.dwNumEntries = sizeof(table) / sizeof(table.table[0]);

    DWORD tableSize = sizeof(table);

	if( ::GetExtendedUdpTable((void *)&table, &tableSize, 
        FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
    {
        for(unsigned int i = 0; i < table.dwNumEntries; i++)
        {
            _udpPortTable[ntohs((unsigned short)table.table[i].dwLocalPort)] = 
                table.table[i].dwOwningPid;
        }
    }
}