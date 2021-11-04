#ifndef ASSETREWARDSHISTORYTABLEMODEL_H
#define ASSETREWARDSHISTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class CAssetHistoryPriv;
class CWallet;
class WalletModel;


/** UI model for the transaction table of a wallet.
 */
class AssetRewardsHistoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AssetRewardsHistoryTableModel(CWallet* wallet, WalletModel* parent = 0);
    ~AssetRewardsHistoryTableModel();

    enum ColumnIndex {
        Label = 0,
        Type,
        Start,
        End,
        Balance,
        Percent,
        Amount
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    /*@}*/

private:
    CWallet* wallet;
    WalletModel* walletModel;

    CAssetHistoryPriv* priv;
    QStringList columns;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public Q_SLOTS:
    void updateTransaction(const QString& hash, int status);

    friend class CAssetHistoryPriv;
};

#endif // ASSETREWARDSHISTORYTABLEMODEL_H
