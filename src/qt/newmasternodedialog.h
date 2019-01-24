// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NEWMASTERNODEDIALOG_H
#define BITCOIN_QT_NEWMASTERNODEDIALOG_H

#include <QDialog>

namespace Ui
{
class NewmasternodeDialog;
}

class NewmasternodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewmasternodeDialog(QWidget* parent = 0);
    ~NewmasternodeDialog();
    void accept();

private:
    Ui::NewmasternodeDialog* ui;
};

#endif // BITCOIN_QT_NEWMASTERNODEDIALOG_H
