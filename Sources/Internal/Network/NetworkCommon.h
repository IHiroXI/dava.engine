/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.
 
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
 
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.
 
    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#ifndef __DAVAENGINE_NETWORKCOMMON_H__
#define __DAVAENGINE_NETWORKCOMMON_H__

#include <Base/BaseTypes.h>

namespace DAVA
{

// Transport types, used by TransportFactory to create transport objects
enum eTransportType
{
    TRANSPORT_TCP,      // Transport based on TCP
    TRANSPORT_RDP,      // Transport based on reliable UDP
    TRANSPORT_UDP       // Transport based on UDP as is
};

// Transport roles
enum eTransportRole
{
    TRANSPORT_SERVER_ROLE,
    TRANSPORT_CLIENT_ROLE
};

// Possible reasons for deactivation of transports and channels
enum eDeactivationReason
{
    REASON_REQUEST,     // Deactivated by user request
    REASON_OTHERSIDE,   // Deactivated by closing connection from other side
    REASON_INITERROR,   // Deactivated on initialization error
    REASON_NETERROR,    // Deactivated on network error
    REASON_TIMEOUT,     // Deactivated on timeout
    REASON_PACKETERROR  // Deactivated on invalid packet
};

// Default channel ID is suitable for cases when ChannelManager's client simply wants
// to send data with no concern of real channel ID he has been registered to
const uint32 DEFAULT_CHANNEL_ID = 0;

}   // namespace DAVA

#endif  // __DAVAENGINE_NETWORKCOMMON_H__
