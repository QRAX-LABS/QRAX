#include "assetrewardshistorytablemodel.h"

#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet/wallet.h"


#include <algorithm>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

struct AssetHistoryRecord {
    enum Type {
        TYPE_ASSET,
        TYPE_DPOS
    };

    Type type;
    uint256 txHash;
    int start;
    int end;
    CAmount balance;
    double percent;
    int n;
    CAmount amount;

    AssetHistoryRecord(){};

};

// Comparison operator for sort/binary search of model tx list
struct TxLessThan {
    bool operator()(const AssetHistoryRecord& a, const AssetHistoryRecord& b) const
    {
        return a.txHash < b.txHash;
    }
    bool operator()(const AssetHistoryRecord& a, const uint256& b) const
    {
        return a.txHash < b;
    }
    bool operator()(const uint256& a, const AssetHistoryRecord& b) const
    {
        return a < b.txHash;
    }
};

class CAssetHistoryPriv {
public:

    CAssetHistoryPriv(CWallet* wallet, AssetRewardsHistoryTableModel* parent) : wallet(wallet),
                                                                           parent(parent)
    {
    };

    CWallet* wallet;
    AssetRewardsHistoryTableModel* parent;
    int lastHeight = 0;

    QList<AssetHistoryRecord> cachedHistoryList;

    AssetHistoryRecord* index(int idx)
    {
        if (idx >= 0 && idx < cachedHistoryList.size()) {
            AssetHistoryRecord* rec = &cachedHistoryList[idx];
            return rec;
        }
        return 0;
    }

    void loadHistory() {

        cachedHistoryList.clear();

        std::vector<CAssetNodeHistoryValue> newList = wallet->GetAssetHistoryList(lastHeight);

        LOCK(cs_main);
        CBlockIndex * pindex = chainActive.Tip();
        if (pindex != NULL)
            lastHeight = pindex->nHeight;

        for (unsigned int i = 0; i < newList.size(); i++)
        {
            AssetHistoryRecord rec;
            rec.type = AssetHistoryRecord::TYPE_ASSET;
            rec.txHash = newList[i].rewardTx;
            rec.start = newList[i].blockHeightStart;
            rec.end = newList[i].blockHeightEnd;
            rec.balance = newList[i].balance;
            rec.percent = newList[i].percent;
            rec.n = newList[i].n;
            rec.amount = newList[i].amount;
            cachedHistoryList.append(rec);
        }

    }

    void updateHistory(const uint256& hash, int status)
    {

        std::vector<CAssetNodeHistoryValue> newList = wallet->GetAssetHistoryList(lastHeight);

        for (unsigned int i = 0; i < newList.size(); i++)
        {
            AssetHistoryRecord rec;
            rec.type = AssetHistoryRecord::TYPE_ASSET;
            rec.txHash = newList[i].rewardTx;
            rec.start = newList[i].blockHeightStart;
            rec.end = newList[i].blockHeightEnd;
            rec.balance = newList[i].balance;
            rec.percent = newList[i].percent;
            rec.n = newList[i].n;
            rec.amount = newList[i].amount;
            cachedHistoryList.append(rec);
        }

        LOCK(cs_main);
        CBlockIndex * pindex = chainActive.Tip();
        if (pindex != NULL)
            lastHeight = pindex->nHeight;

    }

    int size() { return cachedHistoryList.size(); }

};


AssetRewardsHistoryTableModel::AssetRewardsHistoryTableModel(CWallet* wallet, WalletModel* parent) :
    QAbstractTableModel(parent), wallet(wallet), walletModel(parent), priv(new CAssetHistoryPriv(wallet, this))
{
    columns << tr("tx") << tr("Type") << tr("Start Block") << tr("End Block") << tr("Balance") << tr("Percent") << tr("Amount");
    priv->loadHistory();
	subscribeToCoreSignals();
}

AssetRewardsHistoryTableModel::~AssetRewardsHistoryTableModel()
{
	unsubscribeFromCoreSignals();
    delete priv;
}

int AssetRewardsHistoryTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AssetRewardsHistoryTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AssetRewardsHistoryTableModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid())
        return QVariant();

    AssetHistoryRecord* rec = static_cast<AssetHistoryRecord*>(index.internalPointer());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case Label:
            return QString::fromStdString(rec->txHash.ToString());
        case Type:
            return tr("Asset Reward");
        case Start:
            return QString::number(rec->start);
        case End:
            return QString::number(rec->end);
        case Balance:
            return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->balance, false, BitcoinUnits::separatorAlways);
        case Percent:
            {
                int nAssetPercentPeriod = walletModel->getOptionsModel()->getAssetPercentPeriod();
                double percent = rec->percent;
                switch (nAssetPercentPeriod) {
                    case 1:
                        percent = rec->percent * 30.41;
                        break;
                    case 2:
                        percent = rec->percent * 365;
                        break;
                    default:
                        break;
                }
                return GUIUtil::formatPercent(percent);
            }
        case Amount:
            return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->amount, false, BitcoinUnits::separatorAlways);
        }
    }

    return QVariant();
}

QModelIndex AssetRewardsHistoryTableModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    AssetHistoryRecord* data = priv->index(row);
    if (data) {
        return createIndex(row, column, data);
    }
    return QModelIndex();
}

void AssetRewardsHistoryTableModel::updateTransaction(const QString& hash, int status)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());
    priv->updateHistory(updated, status);

}

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct AssetsTransactionNotification {
public:
	AssetsTransactionNotification() {}
	AssetsTransactionNotification(uint256 hash, ChangeType status, int showTransaction) : hash(hash), status(status), showTransaction(showTransaction) {}

    void invoke(QObject* ttm)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyTransactionChanged : " + strHash + " status= " + QString::number(status);
        QMetaObject::invokeMethod(ttm, "updateTransaction", Qt::QueuedConnection,
            Q_ARG(QString, strHash),
            Q_ARG(int, status));
    }

private:
    uint256 hash;
    ChangeType status;
	int showTransaction;
};

static bool fQueueNotifications = false;
static std::vector<AssetsTransactionNotification> vQueueNotifications;

static void NotifyTransactionChanged(AssetRewardsHistoryTableModel* ttm, const uint256& hash, ChangeType status, int showTransaction)
{

	AssetsTransactionNotification notification(hash, status, showTransaction);

    if (fQueueNotifications)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(ttm);
}


void AssetRewardsHistoryTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    //wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void AssetRewardsHistoryTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    //wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}
