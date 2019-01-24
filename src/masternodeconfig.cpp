// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// clang-format off
#include "net.h"
#include "masternodeconfig.h"
#include "util.h"
#include "ui_interface.h"
#include "base58.h"
#include "init.h"
#include "wallet.h"
// clang-format on

CMasternodeConfig masternodeConfig;
CColdStakConfig coldstakConfig;

void CMasternodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex)
{
    CMasternodeEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CMasternodeConfig::read(std::string& strErr)
{
    int linenumber = 1;
    boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
    boost::filesystem::ifstream streamConfig(pathMasternodeConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathMasternodeConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            std::string strHeader = "# Masternode config file\n"
                                    "# Format: alias IP:port masternodeprivkey collateral_output_txid collateral_output_index\n"
                                    "# Example: mn1 127.0.0.2:12365 A3HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0\n";
            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        return true; // Nothing to read, so just return
    }

    for (std::string line; std::getline(streamConfig, line); linenumber++) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string comment, alias, ip, privKey, txHash, outputIndex;

        if (iss >> comment) {
            if (comment.at(0) == '#') continue;
            iss.str(line);
            iss.clear();
        }

        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            iss.str(line);
            iss.clear();
            if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                strErr = _("Could not parse masternode.conf") + "\n" +
                         strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }

        if (Params().NetworkID() == CBaseChainParams::MAIN) {
            if (CService(ip).GetPort() != 12365) {
                strErr = _("Invalid port detected in masternode.conf") + "\n" +
                         strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                         _("(must be 12365 for mainnet)");
                streamConfig.close();
                return false;
            }
        } else if (CService(ip).GetPort() == 12365) {
            strErr = _("Invalid port detected in masternode.conf") + "\n" +
                     strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                     _("(12365 could be used only on mainnet)");
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}

bool CMasternodeConfig::CMasternodeEntry::castOutputIndex(int &n)
{
    try {
        n = std::stoi(outputIndex);
    } catch (const std::exception e) {
        LogPrintf("%s: %s on getOutputIndex\n", __func__, e.what());
        return false;
    }

    return true;
}

void CMasternodeConfig::save()
{
    boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
    FILE* configFile = fopen(pathMasternodeConfigFile.string().c_str(), "w");
    if (configFile != NULL) {
        std::string strHeader = "# Masternode config file\n"
                                "# Format: alias IP:port masternodeprivkey collateral_output_txid collateral_output_index\n"
                                "# Example: mn1 127.0.0.2:12365 A3HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0\n";
        BOOST_FOREACH (CMasternodeEntry e, entries) {
            if (e.getAlias() != "") {
                strHeader += e.getAlias() + " " + e.getIp() + " " + e.getPrivKey() + " " + e.getTxHash() + " " + e.getOutputIndex() + "\n";
            };
        }
        fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
        fclose(configFile);
    }
}

    

void CColdStakConfig::add(std::string txHash, std::string outputIndex)
{
    CColdStakEntry cme(txHash, outputIndex);
    entries.push_back(cme);
    //COutPoint outpoint = COutPoint(uint256S(txHash), std::stoi(outputIndex));
    //pwalletMain->LockCoin(outpoint);
    coldstakConfig.save();

}

void CColdStakConfig::del(std::string txHash, std::string outputIndex)
{
    CColdStakEntry cme(txHash, outputIndex);
//    std::vector<CColdStakEntry>::iterator i = std::find(entries.begin(), entries.end(), &cme);
//    entries.erase(i);
    COutPoint outpoint = COutPoint(uint256S(txHash), std::stoi(outputIndex));
    pwalletMain->UnlockCoin(outpoint);
    save();

}


bool CColdStakConfig::read(std::string& strErr)
{
    int linenumber = 1;
    boost::filesystem::path pathColdStakConfigFile = GetColdStakConfigFile();
    boost::filesystem::ifstream streamConfig(pathColdStakConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathColdStakConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            std::string strHeader = "# Cold Staking config file\n"
                                    "# Format: collateral_output_txid collateral_output_index\n"
                                    "# Example: 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0\n";
            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        return true; // Nothing to read, so just return
    }
    COutPoint outpoint;
    for (std::string line; std::getline(streamConfig, line); linenumber++) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string comment, txHash, outputIndex;

        if (iss >> comment) {
            if (comment.at(0) == '#') continue;
            iss.str(line);
            iss.clear();
        }

        if (!(iss >> txHash >> outputIndex)) {
            iss.str(line);
            iss.clear();
            if (!(iss >> txHash >> outputIndex)) {
                strErr = _("Could not parse coldstaking.conf") + "\n" +
                         strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }
        coldstakConfig.add(txHash, outputIndex);
        LogPrintf("Read cold staking txHash, outputIndex %s  %s\n", txHash, outputIndex);
        outpoint = COutPoint(uint256S(txHash), std::stoi(outputIndex));
        LogPrintf("Read cold staking txHash, outputIndex, vin %s  %s  %s\n", txHash, outputIndex, outpoint.ToString());
        //pwalletMain->LockCoin(outpoint);
    }

    streamConfig.close();
    return true;
}

bool CColdStakConfig::CColdStakEntry::castOutputIndex(int &n)
{
    try {
        n = std::stoi(outputIndex);
    } catch (const std::exception e) {
        LogPrintf("%s: %s on getOutputIndex\n", __func__, e.what());
        return false;
    }

    return true;
}

void CColdStakConfig::save()
{
    boost::filesystem::path pathColdStakConfigFile = GetColdStakConfigFile();
    FILE* configFile = fopen(pathColdStakConfigFile.string().c_str(), "w");
    if (configFile != NULL) {
        std::string strHeader = "# Cold Staking config file\n"
                                "# Format: collateral_output_txid collateral_output_index\n"
                                "# Example: 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0\n";
        BOOST_FOREACH (CColdStakEntry e, entries) {
            if (e.getTxHash() != "") {
                strHeader += e.getTxHash() + " " + e.getOutputIndex() + "\n";
            };
        }
        fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
        fclose(configFile);
    }
}

