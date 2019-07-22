// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASTERNODEMAN_H
#define MASTERNODEMAN_H

#include "base58.h"
#include "key.h"
#include "main.h"
#include "masternode.h"
#include "net.h"
#include "sync.h"
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "masternodeconfig.h"

#include <boost/filesystem/path.hpp>
#include "leveldbwrapper.h"
#include <boost/asio.hpp>
#include "hash.h"


#define MASTERNODES_DUMP_SECONDS (15 * 60)
#define MASTERNODES_DSEG_SECONDS (3 * 60 * 60)

#define MINIMUM_PROTOCOL_VERSION_OLD_PING 70003

using namespace std;

class CMasternodeMan;
extern CMasternodeMan mnodeman;

class CColdStakingList;
extern CColdStakingList colstaklist;
extern map<uint256, string> mapAliasesHash;

void DumpMasternodes();

/** Access to the MN database (mncache.dat)
 */
class CMasternodeDB
{
private:
    boost::filesystem::path pathMN;
    std::string strMagicMessage;

public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CMasternodeDB();
    bool Write(const CMasternodeMan& mnodemanToSave);
    ReadResult Read(CMasternodeMan& mnodemanToLoad, bool fDryRun = false);
};

class CColdStaking
{
public:
    CScript pubKeyCollateralAddress;
    int64_t sigTime;
    int64_t payTime = GetTime();
    int protocolVersion;
    CTxIn vin;
    std::string strAddress;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(sigTime);
        READWRITE(vin);
        READWRITE(payTime);
    }

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << sigTime;
        ss << vin;
        return ss.GetHash();
    }
    void Relay()
    {
        CInv inv(MSG_COLDSTAKER_ANNOUNCE, GetHash());
//        LogPrintf("Relay cold staking lastwin, Hash vin %s  %s %s\n", std::to_string(sigTime), GetHash().ToString(), vin.ToString());
        RelayInv(inv);
    }
    std::string GetStatus(){
        return "ENABLED";
    }
};

class CColdStakingList
{
public:
    map<uint256, CColdStaking> mapColdStaking;
    CColdStaking* getWinningColdStaker()
    {
        int64_t lastTime = GetTime();
        CColdStaking winner;
        uint256 winnerHash;
        CColdStaking* winnerpointer = NULL;
        BOOST_FOREACH(const PAIRTYPE(uint256, CColdStaking)& coldstak, mapColdStaking)
        {
          if (coldstak.second.payTime < lastTime) {
              lastTime = coldstak.second.payTime;
              winner = coldstak.second;
              winnerpointer = &winner;
              winnerHash = coldstak.first;
          }
        }
        if (winnerpointer) {
            colstaklist.mapColdStaking[winnerHash].payTime =  GetTime();
            LogPrintf("Win cold staking vin %s \n", winner.vin.ToString());
            colstaklist.mapColdStaking[winnerHash].Relay();
        }
        return winnerpointer;
    }
    CColdStaking* Find(const CTxIn& vin)
    {
        BOOST_FOREACH(PAIRTYPE(const uint256, CColdStaking)& coldstak, mapColdStaking) {
            if (coldstak.second.vin.prevout == vin.prevout)
                return &coldstak.second;
        }
//        CColdStaking clsr;
//        clsr.vin                        = vin;
//        clsr.sigTime                    = GetTime();
//        clsr.pubKeyCollateralAddress    = vin.prevPubKey;
//        mapColdStaking.insert(make_pair(clsr.GetHash(), clsr));
//        return &mapColdStaking[clsr.GetHash()];
        return NULL;
    }
    CColdStaking* FindorAdd(const CTxIn& vin, int64_t sigTime = GetTime(), int64_t payTime = GetTime())
    {
        BOOST_FOREACH(PAIRTYPE(const uint256, CColdStaking)& coldstak, mapColdStaking) {
            if (coldstak.second.vin == vin) {
                colstaklist.mapColdStaking[coldstak.first].payTime = payTime;
                return &coldstak.second;
            }
        }
        CColdStaking clsr;
        clsr.vin                        = vin;
        clsr.sigTime                    = sigTime;
        clsr.payTime                    = payTime;
        CTransaction tx;
        uint256 hashBlock = 0;
        if (!GetTransaction(vin.prevout.hash, tx, hashBlock, true))
            return NULL;
        if (tx.vout[vin.prevout.n].nValue == 1000 * COIN) { //exactly
			CTxDestination address1;
			CScript pubScript;
			pubScript = tx.vout[vin.prevout.n].scriptPubKey;
			ExtractDestination(pubScript, address1);
			//example CScript scriptPubKey = GetScriptForDestination(CTxDestination DecodeDestination(StdString));        
			LogPrintf("Add cold staking script address %s coin %s\n", EncodeDestination(address1), tx.vout[vin.prevout.n].nValue);
			clsr.pubKeyCollateralAddress    = GetScriptForDestination(address1);
			clsr.strAddress = EncodeDestination(address1);
			colstaklist.mapColdStaking.insert(make_pair(clsr.GetHash(), clsr));
			pwalletMain->LockCoin(colstaklist.mapColdStaking[clsr.GetHash()].vin.prevout);
			colstaklist.mapColdStaking[clsr.GetHash()].Relay();
			return &colstaklist.mapColdStaking[clsr.GetHash()];
        }
        return NULL;
    }
    void DeleteHash(uint256 fHash)
    {
        // relay delete
        pwalletMain->UnlockCoin(mapColdStaking[fHash].vin.prevout);
        coldstakConfig.del(mapColdStaking[fHash].vin.prevout.hash.ToString(), std::to_string(mapColdStaking[fHash].vin.prevout.n));
        mapColdStaking.erase(fHash);
        //send message drop cold staking
    }
    void CheckOld()
    {
        int64_t threemonths = 60 * 60 * 24 * 30 * 3;
        BOOST_FOREACH(PAIRTYPE(const uint256, CColdStaking)& coldstak, mapColdStaking) {
            if (coldstak.second.sigTime > GetTime() + threemonths)
                DeleteHash(coldstak.first);
        }
    }

};

class CMasternodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // critical section to protect the inner data structures specifically on messaging
    mutable CCriticalSection cs_process_message;

    // map to hold all MNs
    std::vector<CMasternode> vMasternodes;
    // who's asked for the Masternode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForMasternodeList;
    // who we asked for the Masternode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForMasternodeList;
    // which Masternodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForMasternodeListEntry;

public:
    // Keep track of all broadcasts I've seen
    map<uint256, CMasternodeBroadcast> mapSeenMasternodeBroadcast;
    // Keep track of all pings I've seen
    map<uint256, CMasternodePing> mapSeenMasternodePing;

    // keep track of dsq count to prevent masternodes from gaming obfuscation queue
    int64_t nDsqCount;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        LOCK(cs);
        READWRITE(vMasternodes);
        READWRITE(mAskedUsForMasternodeList);
        READWRITE(mWeAskedForMasternodeList);
        READWRITE(mWeAskedForMasternodeListEntry);
        READWRITE(nDsqCount);

        READWRITE(mapSeenMasternodeBroadcast);
        READWRITE(mapSeenMasternodePing);
    }

    CMasternodeMan();
    CMasternodeMan(CMasternodeMan& other);

    /// Add an entry
    bool Add(CMasternode& mn);

    /// Ask (source) node for mnb
    void AskForMN(CNode* pnode, CTxIn& vin);

    /// Check all Masternodes
    void Check();

    /// Check all Masternodes and remove inactive
    void CheckAndRemove(bool forceExpiredRemoval = false);

    /// Clear Masternode vector
    void Clear();

    int CountEnabled(int protocolVersion = -1);

    void CountNetworks(int protocolVersion, int& ipv4, int& ipv6, int& onion);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CMasternode* Find(const CScript& payee);
    CMasternode* Find(const CTxIn& vin);
    CMasternode* Find(const CPubKey& pubKeyMasternode);

    /// Find an entry in the masternode list that is next to be paid
    CMasternode* GetNextMasternodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount);

    /// Find a random entry
    CMasternode* FindRandomNotInVec(std::vector<CTxIn>& vecToExclude, int protocolVersion = -1);

    /// Get the current winner for this block
    CMasternode* GetCurrentMasterNode(int mod = 1, int64_t nBlockHeight = 0, int minProtocol = 0);

    std::vector<CMasternode> GetFullMasternodeVector()
    {
        Check();
        return vMasternodes;
    }

    std::vector<pair<int, CMasternode> > GetMasternodeRanks(int64_t nBlockHeight, int minProtocol = 0);
    int GetMasternodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol = 0, bool fOnlyActive = true);
    CMasternode* GetMasternodeByRank(int nRank, int64_t nBlockHeight, int minProtocol = 0, bool fOnlyActive = true);

    void ProcessMasternodeConnections();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Return the number of (unique) Masternodes
    int size() { return vMasternodes.size(); }

    /// Return the number of Masternodes older than (default) 8000 seconds
    int stable_size ();

    std::string ToString() const;

    void Remove(CTxIn vin);

    int GetEstimatedMasternodes(int nBlock);

    /// Update masternode list and maps using provided CMasternodeBroadcast
    void UpdateMasternodeList(CMasternodeBroadcast mnb);
};

class CAlias
{
public:
    std::string pubKey;
    int64_t sigTime = GetTime();
    std::string Hash;
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(sigTime);
        READWRITE(pubKey);
        READWRITE(Hash);
    }

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << sigTime;
        ss << pubKey;
        ss << Hash;
        return ss.GetHash();
    }
    void Relay()
    {
        CInv inv(MSG_ALIASE_ANNOUNCE, GetHash());
        mapAliasesHash.insert(make_pair(GetHash(), Hash));
        RelayInv(inv);
    }
};



/** Access to the aliases database (aliases) */
class CAliasesDB : public CLevelDBWrapper
{
public:
    CAliasesDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

private:
    CAliasesDB(const CAliasesDB&);
    void operator=(const CAliasesDB&);

public:
    bool WriteKey(const string Hash, const CAlias& alias);
    CAlias ReadKey(const string Hash);
    bool KeyExists(const string Hash);
    void ReadAll(map<string, CAlias> &mapAliases);
};

class CKyc
{
private:
    string Auth();

public:
    string pubKey;
    int64_t sigTime = GetTime();
    string hash;
    int idk;
    string email;
    string fio;
    string address;
    string verified;
    // add this fio and other KYC info
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(sigTime);
        READWRITE(pubKey);
        READWRITE(hash);
        READWRITE(idk);
        READWRITE(email);
        READWRITE(fio);
        READWRITE(address);
        READWRITE(verified);
        // add this fio and other KYC info
    }

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << sigTime;
        ss << pubKey;
        return ss.GetHash();
    }

    void CheckMSG();
    void Verify();
    void SendInvoice(std::string uri, std::string invoiceEmail);
    void Invite(std::string invite);
    void Writedb();
    void Readdb();
    std::string Buffer_to_string(const boost::asio::streambuf &buffer);
    string Postssl(std::string path, std::string postdata);
};


extern CKyc kycinfo;

#endif
