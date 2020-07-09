#include "masternodelist.h"
#include "ui_masternodelist.h"

#include "activemasternode.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "init.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "sync.h"
#include "net.h"
#include "wallet.h"
#include "walletmodel.h"
#include "utilmoneystr.h"


#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

CCriticalSection cs_masternodes;

MasternodeList::MasternodeList(QWidget* parent) : QWidget(parent),
                                                  ui(new Ui::MasternodeList),
                                                  clientModel(0),
                                                  walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

    ui->tableWidgetMyMasternodes->setAlternatingRowColors(true);
    ui->tableWidgetMyMasternodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction* startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    fStartNode = false;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    // Fill MN list
    fFilterUpdated = true;
    nTimeFilterUpdated = GetTime();
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void MasternodeList::showContextMenu(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMyMasternodes->itemAt(point);
    if (item) contextMenu->exec(QCursor::pos());
}

void MasternodeList::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        if (mne.getAlias() == strAlias) {
            std::string strError;
            CMasternodeBroadcast mnb;

            bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if (fSuccess) {
                strStatusHtml += "<br>Successfully started masternode.";
                mnodeman.UpdateMasternodeList(mnb);
                mnb.Relay();
            } else {
                strStatusHtml += "<br>Failed to start masternode.<br>Error: " + strError;
            }
            break;
        }
    }
    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        std::string strError;
        CMasternodeBroadcast mnb;

        int nIndex;
        if(!mne.castOutputIndex(nIndex))
            continue;

        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(nIndex));
        CMasternode* pmn = mnodeman.Find(txin);

        if (strCommand == "start-missing" && pmn) continue;

        bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if (fSuccess) {
            nCountSuccessful++;
            mnodeman.UpdateMasternodeList(mnb);
            mnb.Relay();
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d masternodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::updateMyMasternodeInfo(QString strAlias, QString strAddr, CMasternode* pmn)
{
    LOCK(cs_mnlistupdate);
    bool fOldRowFound = false;
    int nNewRow = 0;

    for (int i = 0; i < ui->tableWidgetMyMasternodes->rowCount(); i++) {
        if (ui->tableWidgetMyMasternodes->item(i, 0)->text() == strAlias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if (nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyMasternodes->rowCount();
        ui->tableWidgetMyMasternodes->insertRow(nNewRow);
    }

    QTableWidgetItem* aliasItem = new QTableWidgetItem(strAlias);
    QTableWidgetItem* addrItem = new QTableWidgetItem(pmn ? QString::fromStdString(pmn->addr.ToString()) : strAddr);
    QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->GetStatus() : "MISSING"));
    GUIUtil::DHMSTableWidgetItem* activeSecondsItem = new GUIUtil::DHMSTableWidgetItem(pmn ? (GetTime() - pmn->sigTime) : 0);
    QTableWidgetItem* lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? pmn->lastPing.sigTime : 0)));
    QTableWidgetItem* pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? EncodeDestination(CTxDestination(pmn->pubKeyCollateralAddress.GetID())) : ""));

    ui->tableWidgetMyMasternodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 5, lastSeenItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 6, pubkeyItem);
}

void MasternodeList::updateMyNodeList(bool fForce)
{
    static int64_t nTimeMyListUpdated = 0;
    if (fStartNode) startnode();

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if (nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMyMasternodes->setSortingEnabled(false);
    BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        int nIndex;
        if(!mne.castOutputIndex(nIndex))
            continue;

        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(nIndex));
        CMasternode* pmn = mnodeman.Find(txin);
        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), pmn);
    }
    ui->tableWidgetMyMasternodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void MasternodeList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) return;

    QModelIndex index = selected.at(0);
    int nSelectedRow = index.row();
    std::string strAlias = ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString();

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm masternode start"),
        tr("Are you sure you want to start masternode %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if (encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void MasternodeList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all masternodes start"),
        tr("Are you sure you want to start ALL masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if (encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void MasternodeList::on_startMissingButton_clicked()
{
    if (!masternodeSync.IsMasternodeListSynced()) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this,
        tr("Confirm missing masternodes start"),
        tr("Are you sure you want to start MISSING masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if (encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void MasternodeList::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if (ui->tableWidgetMyMasternodes->selectedItems().count() > 0) {
        ui->startButton->setEnabled(true);
    }
}

void MasternodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void MasternodeList::on_newmasternode_clicked()
{
    std::string strStatus;
    QMessageBox msg;
    
    if (!masternodeSync.IsBlockchainSynced()) {
        strStatus = "Sync in progress. Must wait until sync is complete to start AnkerNode";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }
    if (pwalletMain->IsLocked()) {
        strStatus = "Error: Please enter the wallet passphrase with walletpassphrase first.";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }
    if ((4000 * COIN) > pwalletMain->GetBalance()) {
        strStatus = "Error: Insufficient funds";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }

    // Display message box
    QMessageBox::StandardButton askbox = QMessageBox::question(this, tr("Confirm New AnkerNode start"),
        tr("Are you sure you want to start new AnkerNode?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (askbox != QMessageBox::Yes) return;

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Local OR Remote AnkerNode"),
        tr("Do you want to start a local AnkerNode?"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (retval == QMessageBox::Cancel) return;
    
    IPtext = QString::fromStdString(vNodes.front()->addrLocal.ToStringIP() + ":12365");
    fLocalNode = false;
    
    if (retval == QMessageBox::No) {
        strStatus = "Starting an AnkerNode on a VPS running independently and will be controlled from this wallet";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();        
        bool dialogResult;
        IPtext = QInputDialog::getText(this,"VPS IP address", "Enter your unique public IP address for your VPS and AnkerNode port", QLineEdit::Normal,
                                       IPtext, &dialogResult);
    }
    if (retval == QMessageBox::Yes) {
        if ( GetBoolArg("-masternode", false) ) {
            strStatus = "Error: You are already running a local AnkerNode";
            msg.setText(QString::fromStdString(strStatus));
            msg.exec();
            return;
        }
        fLocalNode = true;
    }
    
    alias = "AnkerNode_" + IPtext.toStdString();
    //getnewaddress
    OutputType output_type = g_address_type;
    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey)) {
        strStatus = "Error: Keypool ran out, please call keypoolrefill first";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }
    
    pwalletMain->LearnRelatedScripts(newKey, output_type);
    CTxDestination dest = GetDestinationForKey(newKey, output_type);
    pwalletMain->SetAddressBook(dest, alias, "receive");
        
    //sendmoney
    // Amount
    CAmount nAmount = 4000 * COIN;

    // Wallet comments
    CWalletTx wtx;
    wtx.mapValue["comment"] = "Setup " + alias;

    // Parse Anker address
    CScript scriptPubKey = GetScriptForDestination(dest);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    if (!pwalletMain->CreateTransaction(scriptPubKey, nAmount, wtx, reservekey, nFeeRequired, strStatus, NULL, ALL_COINS, false, (CAmount)0)) {
        if (nAmount + nFeeRequired > pwalletMain->GetBalance()) {
            strStatus = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
            msg.setText(QString::fromStdString(strStatus));
            msg.exec();
        } else {
            strStatus = "Error: Something went wrong. Cant create transaction!";
            msg.setText(QString::fromStdString(strStatus));
            msg.exec();
        }
        return;
    }
    if (!pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX)) {
        strStatus = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }
    
    ui->autoupdate_label->setText(QString::fromStdString("Waiting for transaction confirmation"));
    
    fStartNode = true;
    ui->newmasternode->setEnabled(false);
    strStatus = "Waiting for transaction confirmation";
    msg.setText(QString::fromStdString(strStatus));
    msg.exec();        
}
void MasternodeList::startnode()
{
    fStartNode = false;
    bool collateral_found = false;
    //masternode genkey
    std::string txHash;
    std::string outputIndex;

    //boost::this_thread::sleep( boost::posix_time::milliseconds(310) );
    // Retrieve all possible outputs
    vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();
    BOOST_FOREACH (COutput& out, possibleCoins) {
        if (!masternodeConfig.Find(out.tx->GetHash().ToString(), std::to_string(out.i))) {
            collateral_found    = true;
            txHash              = out.tx->GetHash().ToString();
            outputIndex         = std::to_string(out.i);
        }
    }


    ui->secondsLabel->setText(QString::fromStdString("..."));
    if (collateral_found) {
        CKey secret;
        secret.MakeNewKey(false);
        std::string privKey = CBitcoinSecret(secret).ToString();
        masternodeConfig.add(alias, IPtext.toStdString(), privKey, txHash, outputIndex);
        // masternodeConfig save all to file
        masternodeConfig.save();

        if (fLocalNode) {
            boost::filesystem::path pathConfigFile = GetConfigFile();
            FILE* configFile = fopen(pathConfigFile.string().c_str(), "a");
            if (configFile != NULL) {
                std::string strHeader ="\n"
                                "rpcallowip=127.0.0.1\n"
                                "rpcbind=127.0.0.1\n"
                                "server=1\n"
                                "listen=1\n"
                                "maxconnections=256\n"
                                "masternode=1\n"
                                "masternodeprivkey=" + privKey + "\n"
                                "masternodeaddr=" + IPtext.toStdString() + "\n";

                fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
                fclose(configFile);
            }
            std::string strStatus;
            QMessageBox msg;
            strStatus = "AnkerNode successfully added. Restart your AnkerWallet and press Start AnkerNode";
            msg.setText(QString::fromStdString(strStatus));
            msg.exec();        

        } else {
            QString ankerconfigfile = QString::fromStdString("rpcallowip=127.0.0.1\n"
                                "rpcbind=127.0.0.1\n"
                                "server=1\n"
                                "listen=1\n"
                                "maxconnections=256\n"
                                "masternode=1\n"
                                "masternodeprivkey=" + privKey + "\n"
                                "masternodeaddr=" + IPtext.toStdString() + "\n");
            QMessageBox msg;
            msg.setText(QString::fromStdString("AnkerNode successfully added. You need add this line to your anker.conf file on our VPS and restart wallet"));
            msg.setDetailedText(ankerconfigfile);
            msg.exec();
            QString filename = QFileDialog::getSaveFileName(this, "Save file", "anker.conf", ".conf");
            QFile qFile( filename );
            if (qFile.open(QIODevice::WriteOnly)) {
                QTextStream out(&qFile); out << ankerconfigfile;
                qFile.close();
            }
        }

        StartAll("start-missing");
        ui->autoupdate_label->setText(QString::fromStdString("Status will be updated automatically in (sec):"));
        fStartNode = false;
        ui->newmasternode->setEnabled(true);
        updateMyNodeList(true);
    } else {
       fStartNode = true; 
    }
}

