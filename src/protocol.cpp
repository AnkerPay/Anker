// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"

#include "chainparams.h"
#include "util.h"
#include "utilstrencodings.h"

#ifndef WIN32
#include <arpa/inet.h>
#endif

namespace NetMsgType {
const char *VERSION="version";
const char *VERACK="verack";
const char *ADDR="addr";
const char *ALERT="alert";
const char *INV="inv";
const char *GETDATA="getdata";
const char *MERKLEBLOCK="merkleblock";
const char *GETBLOCKS="getblocks";
const char *GETHEADERS="getheaders";
const char *TX="tx";
const char *MNW="mnw";
const char *MPROP="mprop";
const char *MVOTE="mvote";
const char *TXLVOTE="txlvote";
const char *IX="ix";
const char *DSTX="dstx";
const char *FBS="fbs";
const char *MNB="mnb";
const char *CSTK="cstk";
const char *AANS="aans";
const char *MNP="mnp";
const char *MNGET="mnget";
const char *DSEEP="dseep";
const char *DSEE="dsee";
const char *DSEG="dseg";
const char *DSSU="dssu";
const char *DSS="dss";
const char *DSA="dsa";
const char *DSQ="dsq";
const char *DSF="dsf";
const char *DSI="dsi";
const char *DSC="dsc";
const char *DSR="dsr";
const char *SPORK="spork";
const char *GETSPORKS="getsporks";
const char *GETSPORK="getspork";
const char *FBVOTE="fbvote";
const char *SSC="ssc";
const char *MNVS="mnvs";
const char *HEADERS="headers";
const char *BLOCK="block";
const char *GETADDR="getaddr";
const char *MEMPOOL="mempool";
const char *PING="ping";
const char *PONG="pong";
const char *NOTFOUND="notfound";
const char *FILTERLOAD="filterload";
const char *FILTERADD="filteradd";
const char *FILTERCLEAR="filterclear";
const char *REJECT="reject";
const char *SENDHEADERS="sendheaders";
const char *FEEFILTER="feefilter";
const char *SENDCMPCT="sendcmpct";
const char *CMPCTBLOCK="cmpctblock";
const char *GETBLOCKTXN="getblocktxn";
const char *BLOCKTXN="blocktxn";
} // namespace NetMsgType

/** All known message types. Keep this in the same order as the list of
 * messages above and in protocol.h.
 */
const static std::string allNetMessageTypes[] = {
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::ADDR,
    NetMsgType::ALERT,
    NetMsgType::INV,
    NetMsgType::GETDATA,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETHEADERS,
    NetMsgType::TX,
    NetMsgType::MNW,
    NetMsgType::MPROP,
    NetMsgType::MVOTE,
    NetMsgType::TXLVOTE,
    NetMsgType::IX,
    NetMsgType::CSTK,
    NetMsgType::AANS,
    NetMsgType::DSTX,
    NetMsgType::FBS,
    NetMsgType::MNB,
    NetMsgType::MNP,
    NetMsgType::MNGET,
    NetMsgType::DSEEP,
    NetMsgType::DSEE,
    NetMsgType::DSEG,
    NetMsgType::DSSU,
    NetMsgType::DSS,
    NetMsgType::DSA,
    NetMsgType::DSQ,
    NetMsgType::DSF,
    NetMsgType::DSI,
    NetMsgType::DSC,
    NetMsgType::DSR,
    NetMsgType::SPORK,
    NetMsgType::GETSPORKS,
    NetMsgType::GETSPORK,
    NetMsgType::FBVOTE,
    NetMsgType::SSC,
    NetMsgType::MNVS,
    NetMsgType::HEADERS,
    NetMsgType::BLOCK,
    NetMsgType::GETADDR,
    NetMsgType::MEMPOOL,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::NOTFOUND,
    NetMsgType::FILTERLOAD,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::REJECT,
    NetMsgType::SENDHEADERS,
    NetMsgType::FEEFILTER,
    NetMsgType::SENDCMPCT,
    NetMsgType::CMPCTBLOCK,
    NetMsgType::GETBLOCKTXN,
    NetMsgType::BLOCKTXN
};

static const char* ppszTypeName[] =
    {
        "ERROR",
        "tx",
        "block",
        "filtered block",
        "tx lock request",
        "tx lock vote",
        "spork",
        "mn winner",
        "mn scan error",
        "mn budget vote",
        "mn budget proposal",
        "mn budget finalized",
        "mn budget finalized vote",
        "mn quorum",
        "mn announce",
        "mn ping",
        "cstk",
        "aans",
        "dstx"};

CMessageHeader::CMessageHeader()
{
    memcpy(pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    nMessageSize = -1;
    nChecksum = 0;
}

CMessageHeader::CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
    nMessageSize = nMessageSizeIn;
    nChecksum = 0;
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsValid() const
{
    // Check start string
    if (memcmp(pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE) != 0)
        return false;

    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; p1++) {
        if (*p1 == 0) {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; p1++)
                if (*p1 != 0)
                    return false;
        } else if (*p1 < ' ' || *p1 > 0x7E)
            return false;
    }

    // Message size
    if (nMessageSize > MAX_SIZE) {
        LogPrintf("CMessageHeader::IsValid() : (%s, %u bytes) nMessageSize > MAX_SIZE\n", GetCommand(), nMessageSize);
        return false;
    }

    return true;
}


CAddress::CAddress() : CService()
{
    Init();
}

CAddress::CAddress(CService ipIn, uint64_t nServicesIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
}

void CAddress::Init()
{
    nServices = NODE_NETWORK;
    nTime = 100000000;
    nLastTry = 0;
}

CInv::CInv()
{
    type = 0;
    hash = 0;
}

CInv::CInv(int typeIn, const uint256& hashIn)
{
    type = typeIn;
    hash = hashIn;
}

CInv::CInv(const std::string& strType, const uint256& hashIn)
{
    unsigned int i;
    for (i = 1; i < ARRAYLEN(ppszTypeName); i++) {
        if (strType == ppszTypeName[i]) {
            type = i;
            break;
        }
    }
    if (i == ARRAYLEN(ppszTypeName))
        LogPrint("net", "CInv::CInv(string, uint256) : unknown type '%s'", strType);
    hash = hashIn;
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    int masked = type & MSG_TYPE_MASK;
    return (masked >= 1 && masked <= MSG_TYPE_MAX);
}

bool CInv::IsMasterNodeType() const{
 	return (type >= 6);
}

const char* CInv::GetCommand() const
{
    if (!IsKnownType())
        LogPrint("net", "CInv::GetCommand() : type=%d unknown type", type);

    if (type < (int)sizeof(ppszTypeName))
        return ppszTypeName[type];
    else
        return "unknown";
}

std::string CInv::ToString() const
{
    return strprintf("%s %s", GetCommand(), hash.ToString());
}