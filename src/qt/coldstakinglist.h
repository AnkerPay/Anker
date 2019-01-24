#ifndef COLDSTAKINGLIST_H
#define COLDSTAKINGLIST_H

#include "masternode.h"
#include "masternodeman.h"
#include "platformstyle.h"
#include "sync.h"
#include "util.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MY_COLDSTAKINGLIST_UPDATE_SECONDS 60
#define COLDSTAKINGLIST_UPDATE_SECONDS 15
#define COLDSTAKINGLIST_FILTER_COOLDOWN_SECONDS 3

namespace Ui
{
class ColdstakingList;
}


QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class ColdstakingList : public QWidget
{
    Q_OBJECT

public:
    explicit ColdstakingList(QWidget* parent = 0);
    ~ColdstakingList();

    void StartAll(std::string strCommand = "start-all");

private:
    int64_t nTimeFilterUpdated;
    bool fFilterUpdated;

public Q_SLOTS:
    void updateMyCSInfo(QString strHash, QString strHashId, CColdStaking* cstk);
    void updateMyCSList(bool fForce = false);

Q_SIGNALS:

private:
    QTimer* timer;
    Ui::ColdstakingList* ui;
    CCriticalSection cs_mnlistupdate;
    QString strCurrentFilter;

private Q_SLOTS:
    void on_newcoldstaking_clicked();
    void on_startAllButton_clicked();
    void on_UpdateButton_clicked();
};
#endif // COLDSTAKINGLIST_H
