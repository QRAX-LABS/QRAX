// Copyright (c) 2011-2013 The Bitcoin developers
// Copyright (c) 2019 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_INTERNALTRANSACTIONTABLEMODEL_H
#define BITCOIN_QT_INTERNALTRANSACTIONTABLEMODEL_H

#include "bitcoinunits.h"

#include <QAbstractTableModel>
#include <QStringList>

#include <memory>

namespace interfaces {
class Handler;
}

class TransactionRecord;
class InternalTransactionTablePriv;
class WalletModel;

class CWallet;

/** UI model for the transaction table of a wallet.
 */
class InternalTransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit InternalTransactionTableModel(CWallet* wallet, WalletModel* parent = 0);
    ~InternalTransactionTableModel();

    enum ColumnIndex {
	Status = 0,
	Watchonly = 1,
	Date = 2,
	Type = 3,
	ToAddress = 4,
	Amount = 5
    };

    /** Roles to get specific information from a transaction row.
	These are independent of column.
    */
    enum RoleIndex {
	/** Type of transaction */
	TypeRole = Qt::UserRole,
	/** Date and time this transaction was created */
	DateRole,
	/** Watch-only boolean */
	WatchonlyRole,
	/** Watch-only icon */
	WatchonlyDecorationRole,
	/** Address of transaction */
	AddressRole,
	/** Label of address related to transaction */
	LabelRole,
	/** Net amount of transaction */
	AmountRole,
	/** Transaction hash */
	TxHashRole,
	/** Is transaction confirmed? */
	ConfirmedRole,
	/** Formatted amount, without brackets when unconfirmed */
	FormattedAmountRole,
	/** Transaction status (TransactionRecord::Status) */
	StatusRole,
	/** Credit amount of transaction */
	ShieldedCreditAmountRole,
	/** Fee Transaction */
	FeeAmountRole,
	/** Transaction size in bytes */
	SizeRole
    };

    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    int size() const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    bool processingQueuedTransactions() { return fProcessingQueuedTransactions; }

Q_SIGNALS:
    void txArrived(const QString& hash, const bool& isCoinStake, const bool& isCSAnyType);

private:
    // Listeners
    std::unique_ptr<interfaces::Handler> m_handler_transaction_changed;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;

    CWallet* wallet{nullptr};
    WalletModel* walletModel{nullptr};
    QStringList columns{};
	InternalTransactionTablePriv* priv{nullptr};
    bool fProcessingQueuedTransactions{false};

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    QString lookupAddress(const std::string& address, bool tooltip) const;
    QVariant addressColor(const TransactionRecord* wtx) const;
    QString formatTxStatus(const TransactionRecord* wtx) const;
    QString formatTxDate(const TransactionRecord* wtx) const;
    QString formatTxType(const TransactionRecord* wtx) const;
    QString formatTxToAddress(const TransactionRecord* wtx, bool tooltip) const;
    QString formatTxAmount(const TransactionRecord* wtx, bool showUnconfirmed = true, BitcoinUnits::SeparatorStyle separators = BitcoinUnits::separatorStandard) const;
    QString formatTooltip(const TransactionRecord* rec) const;
    QVariant txStatusDecoration(const TransactionRecord* wtx) const;
    QVariant txWatchonlyDecoration(const TransactionRecord* wtx) const;
    QVariant txAddressDecoration(const TransactionRecord* wtx) const;

public Q_SLOTS:
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString& hash, int status, bool showTransaction);
    void updateConfirmations();
    void updateDisplayUnit();
    /** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
    void updateAmountColumnTitle();
    /* Needed to update fProcessingQueuedTransactions through a QueuedConnection */
    void setProcessingQueuedTransactions(bool value) { fProcessingQueuedTransactions = value; }

	friend class InternalTransactionTablePriv;
};

#endif // BITCOIN_QT_INTERNALTRANSACTIONTABLEMODEL_H