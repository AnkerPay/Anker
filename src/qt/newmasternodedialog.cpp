// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "newmasternodedialog.h"
#include "masternodeconfig.h"
#include "activemasternode.h"
#include "guiutil.h"
#include "init.h"
#include "wallet.h"
#include "utilmoneystr.h"
#include "ui_newmasternodedialog.h"
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <QLineEdit>

#include <QModelIndex>
#include <QSettings>
#include <QString>

NewmasternodeDialog::NewmasternodeDialog(QWidget* parent) : QDialog(parent),
                                                                                        ui(new Ui::NewmasternodeDialog)
{
    ui->setupUi(this);

    /* Open CSS when configured */
    ui->progressBar->hide();
    this->setStyleSheet(GUIUtil::loadStyleSheet());
}

NewmasternodeDialog::~NewmasternodeDialog()
{
    delete ui;
}


void NewmasternodeDialog::accept()
{
    std::string strStatus;
    QMessageBox msg;
    ui->progressBar->show();
    
    if (pwalletMain->IsLocked()) {
        strStatus = "Error: Please enter the wallet passphrase with walletpassphrase first.";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        QDialog::accept();
        return;
    }
    if ((4000 * COIN) > pwalletMain->GetBalance()) {
        strStatus = "Error: Insufficient funds";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        QDialog::accept();
        return;
    }
    if ( GetBoolArg("-masternode", false) && ui->localnode->isChecked() ) {
        strStatus = "Error: It already have local Masternode";
        msg.setText(QString::fromStdString(strStatus));
        msg.exec();
        return;
    }
    //masternode genkey
    CKey secret;
    secret.MakeNewKey(false);
    std::string privKey = CBitcoinSecret(secret).ToString();
    std::string ip = ui->lineEditIP->text().toStdString() + ":" + ui->lineEditPort->text().toStdString();
    std::string txHash;
    std::string outputIndex;
    std::string alias;
    alias = "Masternode_" + ui->lineEditIP->text().toStdString();
    
    ui->progressBar->setValue(5);

    //check if collateral exist and not used
    bool collateral_found = false;
    vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();
//     BOOST_FOREACH (COutput& out, possibleCoins) {
//         if (!masternodeConfig.Find(out.tx->GetHash().ToString(), std::to_string(out.i))) {
//             collateral_found    = true;
//             txHash              = out.tx->GetHash().ToString();
//             outputIndex         = std::to_string(out.i);
//         }
//     }

    if (!collateral_found) {
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
            QDialog::accept();
            return;
        }
     
        pwalletMain->LearnRelatedScripts(newKey, output_type);
        CTxDestination dest = GetDestinationForKey(newKey, output_type);
        pwalletMain->SetAddressBook(dest, alias, "receive");
            
        ui->progressBar->setValue(10);
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
            QDialog::accept();
            return;
        }
        if (!pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX)) {
            strStatus = "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.";
            msg.setText(QString::fromStdString(strStatus));
            msg.exec();
            QDialog::accept();
            return;
        }
        //wait until masternode outputs
        int stepbar = ui->progressBar->value();
        while (!collateral_found) {
            QCoreApplication::processEvents();
            boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
            QCoreApplication::processEvents();
            stepbar += 5;
            ui->progressBar->setValue(stepbar);
            possibleCoins = activeMasternode.SelectCoinsMasternode();
            BOOST_FOREACH (COutput& out, possibleCoins) {
                if (!masternodeConfig.Find(out.tx->GetHash().ToString(), std::to_string(out.i))) {
                    collateral_found    = true;
                    txHash              = out.tx->GetHash().ToString();
                    outputIndex         = std::to_string(out.i);
                }
            }
            QCoreApplication::processEvents();
        }
    }

    ui->progressBar->setValue(90);
    if (ui->localnode->isChecked()) {
        // masternodeConfig add entry
        masternodeConfig.add(alias, ip, privKey, txHash, outputIndex);
        // masternodeConfig save all to file
        masternodeConfig.save();
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
                            "masternodeaddr=" + ip + "\n";

            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        
        msg.setText(QString::fromStdString("This line will be added to your anker.conf file and you need restart wallet"));
        msg.setDetailedText(QString::fromStdString("rpcallowip=127.0.0.1\n"
                            "rpcbind=127.0.0.1\n"
                            "server=1\n"
                            "listen=1\n"
                            "maxconnections=256\n"
                            "masternode=1\n"
                            "masternodeprivkey=" + privKey + "\n"
                            "masternodeaddr=" + ip + "\n"));
        msg.exec();
    } else {
        // masternodeConfig add entry
        masternodeConfig.add(alias, ip, privKey, txHash, outputIndex);
        // masternodeConfig save all to file
        masternodeConfig.save();
        msg.setText(QString::fromStdString("You need add this line to your anker.conf file on our VPS and restart daemon"));
        msg.setDetailedText(QString::fromStdString("rpcallowip=127.0.0.1\n"
                            "rpcbind=127.0.0.1\n"
                            "server=1\n"
                            "listen=1\n"
                            "daemon=1\n"
                            "maxconnections=256\n"
                            "masternode=1\n"
                            "masternodeprivkey=" + privKey + "\n"
                            "masternodeaddr=" + ip + "\n"));
        msg.exec();
    }
    ui->progressBar->setValue(100);
    QDialog::accept();
}
