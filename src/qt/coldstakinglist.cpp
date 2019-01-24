#include "coldstakinglist.h"
#include "ui_coldstakinglist.h"

#include "activemasternode.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "init.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "sync.h"
#include "wallet.h"
#include "walletmodel.h"
#include "utilmoneystr.h"

#include <boost/thread/thread.hpp>

#include <QMessageBox>
#include <QTimer>
#include <QString>


ColdstakingList::ColdstakingList(QWidget* parent) : QWidget(parent),
                                                  ui(new Ui::ColdstakingList)
{
    ui->setupUi(this);

    ui->progressBar->hide();
    int columnHashWidth = 360;
    int columnHashIdWidth = 60;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 90;

    ui->tableWidgetMyColdstaking->setColumnWidth(0, columnHashWidth);
    ui->tableWidgetMyColdstaking->setColumnWidth(1, columnHashIdWidth);
    ui->tableWidgetMyColdstaking->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyColdstaking->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyColdstaking->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyColdstaking->setColumnWidth(5, columnLastSeenWidth);



    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyCSList()));
    timer->start(1000);

    // Fill MN list
    fFilterUpdated = true;
    nTimeFilterUpdated = GetTime();
}

ColdstakingList::~ColdstakingList()
{
    delete ui;
}



void ColdstakingList::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;
    BOOST_FOREACH (CColdStakConfig::CColdStakEntry mne, coldstakConfig.getEntries()) {
        std::string strError;

        int nIndex;
        if(!mne.castOutputIndex(nIndex))
            continue;
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(nIndex));
        CColdStaking* pmn = colstaklist.FindorAdd(txin);
//        pmn->Relay();
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = "Successfully started Cold Satking";

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyCSList(true);
}

void ColdstakingList::updateMyCSInfo(QString strHash, QString strHashId, CColdStaking* pmn)
{
    LOCK(cs_mnlistupdate);
    bool fOldRowFound = false;
    int nNewRow = 0;

    for (int i = 0; i < ui->tableWidgetMyColdstaking->rowCount(); i++) {
        if (ui->tableWidgetMyColdstaking->item(i, 0)->text() == strHash) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if (nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyColdstaking->rowCount();
        ui->tableWidgetMyColdstaking->insertRow(nNewRow);
    }

    QTableWidgetItem* hashItem = new QTableWidgetItem(strHash);
    QTableWidgetItem* hashidItem = new QTableWidgetItem(strHashId);
    QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->GetStatus() : "MISSING"));
    GUIUtil::DHMSTableWidgetItem* activeSecondsItem = new GUIUtil::DHMSTableWidgetItem(pmn ? (pmn->sigTime - pmn->payTime) : 0);
    QTableWidgetItem* lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? pmn->payTime : 0)));

    ui->tableWidgetMyColdstaking->setItem(nNewRow, 0, hashItem);
    ui->tableWidgetMyColdstaking->setItem(nNewRow, 1, hashidItem);
    ui->tableWidgetMyColdstaking->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMyColdstaking->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMyColdstaking->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMyColdstaking->setItem(nNewRow, 5, lastSeenItem);
}

void ColdstakingList::updateMyCSList(bool fForce)
{
    static int64_t nTimeMyListUpdated = 0;

    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_COLDSTAKINGLIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if (nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMyColdstaking->setSortingEnabled(false);
    BOOST_FOREACH (CColdStakConfig::CColdStakEntry mne, coldstakConfig.getEntries()) {
        int nIndex;
        if(!mne.castOutputIndex(nIndex))
            continue;
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(nIndex));
        CColdStaking* pmn = colstaklist.Find(txin);

        updateMyCSInfo(QString::fromStdString(mne.getTxHash()), QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetMyColdstaking->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}


void ColdstakingList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all Cold Satking start"),
        tr("Are you sure you want to start ALL Cold Staking?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;


    if (pwalletMain->IsLocked()) {
        std::string returnObj = "Error. Please enter the wallet passphrase with walletpassphrase first.";

        QMessageBox msg;
        msg.setText(QString::fromStdString(returnObj));
        msg.exec();

        return; // Unlock wallet was cancelled
    }

    StartAll();
}



void ColdstakingList::on_UpdateButton_clicked()
{
    updateMyCSList(true);
}

void ColdstakingList::on_newcoldstaking_clicked()
{
    std::string strErrorRet;
    QMessageBox msg;
    
    if (!masternodeSync.IsBlockchainSynced()) {
        strErrorRet = "Sync in progress. Must wait until sync is complete to start Cold Staking";
        msg.setText(QString::fromStdString(strErrorRet));
        msg.exec();
        return;
    }
    if (pwalletMain->IsLocked()) {
        strErrorRet = "Error: Please enter the wallet passphrase with walletpassphrase first.";
        msg.setText(QString::fromStdString(strErrorRet));
        msg.exec();
        return;
    }
    if ((1000 * COIN) > pwalletMain->GetBalance()) {
        strErrorRet = "Error: Insufficient funds";
        msg.setText(QString::fromStdString(strErrorRet));
        msg.exec();
        return;
    }
    bool collateral_found = false;
    vector<COutput> vCoins;
    vector<COutput> possibleCoins;
    CTxIn txvin;
    std::string txHash;
    std::string outputIndex;
    std::string alias;

    pwalletMain->AvailableCoins(vCoins);
    BOOST_FOREACH (const COutput& out, vCoins) {
        if (out.tx->vout[out.i].nValue == 1000 * COIN) { //exactly
            possibleCoins.push_back(out);
        }
    }
    
    BOOST_FOREACH (COutput& out, possibleCoins) {
        txvin = CTxIn(out.tx->GetHash(), uint32_t(out.i));
        if (!colstaklist.Find(txvin)) {
            collateral_found    = true;
            txHash              = out.tx->GetHash().ToString();
            outputIndex         = std::to_string(out.i);
        }
    }

    if (!collateral_found) {

           //getnewaddress
        OutputType output_type = g_address_type;
        if (!pwalletMain->IsLocked())
            pwalletMain->TopUpKeyPool();

        // Generate a new key that is added to wallet
        CPubKey newKey;
        if (!pwalletMain->GetKeyFromPool(newKey)) {
            strErrorRet = "Error: Keypool ran out, please call keypoolrefill first";
            msg.setText(QString::fromStdString(strErrorRet));
            msg.exec();
            return;
        }
     
        pwalletMain->LearnRelatedScripts(newKey, output_type);
        CTxDestination dest = GetDestinationForKey(newKey, output_type);
        pwalletMain->SetAddressBook(dest, alias, "receive");
            
        //sendmoney
        // Amount
        CAmount nAmount = 1000 * COIN;

        // Wallet comments
        CWalletTx wtx;
        wtx.mapValue["comment"] = "Setup Cold Staking";

        // Parse Anker address
        CScript scriptPubKey = GetScriptForDestination(dest);

        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        if (!pwalletMain->CreateTransaction(scriptPubKey, nAmount, wtx, reservekey, nFeeRequired, strErrorRet, NULL, ALL_COINS, false, (CAmount)0)) {
            if (nAmount + nFeeRequired > pwalletMain->GetBalance()) {
                strErrorRet = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
                msg.setText(QString::fromStdString(strErrorRet));
                msg.exec();
            }
            return;
        }
        if (!pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX)) {
            strErrorRet = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
            msg.setText(QString::fromStdString(strErrorRet));
            msg.exec();
            return;
        }
        //wait until Cold Staking outputs
        while (!collateral_found) {
            boost::this_thread::sleep( boost::posix_time::milliseconds(310) );
            // Retrieve all possible outputs
            pwalletMain->AvailableCoins(vCoins);
            BOOST_FOREACH (const COutput& out, vCoins) {
                if (out.tx->vout[out.i].nValue == 1000 * COIN) { //exactly
                    possibleCoins.push_back(out);
                }
            }
            BOOST_FOREACH (COutput& out, possibleCoins) {
                txvin = CTxIn(out.tx->GetHash(), uint32_t(out.i));
                if (!colstaklist.Find(txvin)) {
                    collateral_found    = true;
                    txHash              = out.tx->GetHash().ToString();
                    outputIndex         = std::to_string(out.i);
                }
            }
        }
    }
    // coldstakConfig add entry
    coldstakConfig.add(txHash, outputIndex);
    // coldstakConfig save all to file
    updateMyCSList(true);
}
