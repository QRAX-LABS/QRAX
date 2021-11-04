// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/qrax/settings/settingsexportcsv.h"
#include "qt/qrax/settings/forms/ui_settingsexportcsv.h"
#include <QFile>
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "qt/qrax/qtutils.h"
#include "guiinterface.h"

SettingsExportCSV::SettingsExportCSV(QRAXGUI* _window, QWidget *parent) :
    PWidget(_window, parent),
    ui(new Ui::SettingsExportCSV)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());

    /* Containers */
    ui->left->setProperty("cssClass", "container");
    ui->left->setContentsMargins(10,10,10,10);

    // Title
    ui->labelTitle->setProperty("cssClass", "text-title-screen");
    ui->labelSubtitle1->setProperty("cssClass", "text-subtitle");

    // Location
    ui->labelSubtitleLocation->setProperty("cssClass", "text-title");
    ui->labelSubtitleLocationAddress->setProperty("cssClass", "text-title");
    ui->labelSubtitleLocationAsset->setProperty("cssClass", "text-title");

    setCssProperty({ui->pushButtonDocuments, ui->pushButtonAddressDocuments, ui->pushButtonAssetRewards}, "btn-edit-primary-folder");
    setShadow(ui->pushButtonDocuments);
    setShadow(ui->pushButtonAddressDocuments);
    setShadow(ui->pushButtonAssetRewards);

    ui->labelDivider->setProperty("cssClass", "container-divider");
    ui->labelDivider2->setProperty("cssClass", "container-divider");

    SortEdit* lineEdit = new SortEdit(ui->comboBoxSort);
    connect(lineEdit, &SortEdit::Mouse_Pressed, [this](){ui->comboBoxSort->showPopup();});
    setSortTx(ui->comboBoxSort, lineEdit);

    SortEdit* lineEditType = new SortEdit(ui->comboBoxSortType);
    connect(lineEditType, &SortEdit::Mouse_Pressed, [this](){ui->comboBoxSortType->showPopup();});
    setSortTxTypeFilter(ui->comboBoxSortType, lineEditType);
    ui->comboBoxSortType->setCurrentIndex(0);

    SortEdit* lineEditAddressBook = new SortEdit(ui->comboBoxSortAddressType);
    connect(lineEditAddressBook, &SortEdit::Mouse_Pressed, [this](){ui->comboBoxSortAddressType->showPopup();});
    setFilterAddressBook(ui->comboBoxSortAddressType, lineEditAddressBook);
    ui->comboBoxSortAddressType->setCurrentIndex(0);

    connect(ui->pushButtonDocuments, &QPushButton::clicked, [this](){selectFileOutput(EXPORT_TX);});
    connect(ui->pushButtonAddressDocuments, &QPushButton::clicked, [this](){selectFileOutput(EXPORT_ADDRESS);});
    connect(ui->pushButtonAssetRewards, &QPushButton::clicked, [this](){selectFileOutput(EXPORT_ASSETS);});

}

void SettingsExportCSV::selectFileOutput(const ExportType& exportType)
{
    QString filename;
    switch (exportType) {
        case EXPORT_TX:
            filename = GUIUtil::getSaveFileName(this, tr("Export Transactions List"), QString(), tr("QRAX Transactions list (*.csv)"), nullptr);
            if (!filename.isEmpty()) {
                ui->pushButtonDocuments->setText(filename);
                exportTxes(filename);
            } else {
                ui->pushButtonDocuments->setText(tr("Select folder..."));
            }
            break;
        case EXPORT_ADDRESS:
            filename = GUIUtil::getSaveFileName(this, tr("Export Address List"), QString(), tr("QRAX Address list (*.csv)"), nullptr);
            if (!filename.isEmpty()) {
                ui->pushButtonAddressDocuments->setText(filename);
                exportAddrs(filename);
            } else {
                ui->pushButtonAddressDocuments->setText(tr("Select folder..."));
            }
            break;
        case EXPORT_ASSETS:
            filename = GUIUtil::getSaveFileName(this, tr("Export Asset History"), QString(), tr("QRAX Asset History (*.csv)"), nullptr);
            if (!filename.isEmpty()) {
                ui->pushButtonAssetRewards->setText(filename);
                exportAssetHistory(filename);
            } else {
                ui->pushButtonAssetRewards->setText(tr("Select folder..."));
            }
            break;
        default:
            break;
    }
}

void SettingsExportCSV::exportTxes(const QString& filename)
{
    if (filename.isNull()) {
        inform(tr("Please select a folder to export the csv file first."));
        return;
    }

    CSVModelWriter writer(filename);
    bool fExport = false;

    if (walletModel) {

        if (!txFilter) {
            // Set up transaction list
            txFilter = new TransactionFilterProxy();
            txFilter->setDynamicSortFilter(true);
            txFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
            txFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
            txFilter->setSortRole(Qt::EditRole);
			txFilter->setSourceModel(walletModel->getInternalTransactionTableModel());
        }

        // First type filter
        txFilter->setTypeFilter(ui->comboBoxSortType->itemData(ui->comboBoxSortType->currentIndex()).toInt());

        // Second tx filter.
		int columnIndex = InternalTransactionTableModel::Date;
        Qt::SortOrder order = Qt::DescendingOrder;
        switch (ui->comboBoxSort->itemData(ui->comboBoxSort->currentIndex()).toInt()) {
            case SortTx::DATE_ASC:{
                columnIndex = TransactionTableModel::Date;
                order = Qt::AscendingOrder;
                break;
            }
            case SortTx::DATE_DESC:{
                columnIndex = TransactionTableModel::Date;
                break;
            }
            case SortTx::AMOUNT_ASC:{
                columnIndex = TransactionTableModel::Amount;
                order = Qt::AscendingOrder;
                break;
            }
            case SortTx::AMOUNT_DESC:{
                columnIndex = TransactionTableModel::Amount;
                break;
            }

        }
        txFilter->sort(columnIndex, order);

        // name, column, role
        writer.setModel(txFilter);
        writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
        if (walletModel->haveWatchOnly())
            writer.addColumn(tr("Watch-only"), TransactionTableModel::Watchonly);
        writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
        writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
        writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
        writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
        writer.addColumn(BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit()), 0, TransactionTableModel::FormattedAmountRole);
        writer.addColumn(tr("ID"), 0, TransactionTableModel::TxHashRole);
        fExport = writer.write();
    }

    if (fExport) {
        inform(tr("Exporting Successful\nThe transaction history was successfully saved to %1.").arg(filename));
    } else {
        inform(tr("Exporting Failed\nThere was an error trying to save the transaction history to %1.").arg(filename));
    }
}

void SettingsExportCSV::exportAddrs(const QString& filename)
{
    if (filename.isNull()) {
        inform(tr("Please select a folder to export the csv file first."));
        return;
    }

    bool fExport = false;
    if (walletModel) {

        if (!addressFilter) {
            addressFilter = new QSortFilterProxyModel(this);
            addressFilter->setSourceModel(walletModel->getAddressTableModel());
            addressFilter->setDynamicSortFilter(true);
            addressFilter->setFilterRole(AddressTableModel::TypeRole);
        }

        // Filter by type
        QString filterBy = ui->comboBoxSortAddressType->itemData(ui->comboBoxSortAddressType->currentIndex()).toString();
        addressFilter->setFilterFixedString(filterBy);

        if (addressFilter->rowCount() == 0) {
            inform(tr("No available addresses to export under the selected filter"));
            return;
        }

        CSVModelWriter writer(filename);
        // name, column, role
        writer.setModel(addressFilter);
        writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
        writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);
        writer.addColumn("Type", AddressTableModel::Type, Qt::DisplayRole);
        fExport = writer.write();
    }

    if (fExport) {
        inform(tr("Exporting Successful\nThe address book was successfully saved to %1.").arg(filename));
    } else {
        inform(tr("Exporting Failed\nThere was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}

void SettingsExportCSV::exportAssetHistory(const QString& filename)
{
    if (filename.isNull()) {
        inform(tr("Please select a folder to export the csv file first."));
        return;
    }

    bool fExport = false;
    if (walletModel) {

        if (!assetFilter) {
            assetFilter = new QSortFilterProxyModel(this);
            assetFilter->setSourceModel(walletModel->getAssetRewardTableModel());
            assetFilter->setDynamicSortFilter(true);
            assetFilter->setFilterRole(AssetRewardsHistoryTableModel::TypeRole);
        }

        // Filter by type
        //QString filterBy = ui->comboBoxSortAddressType->itemData(ui->comboBoxSortAddressType->currentIndex()).toString();
        //assetFilter->setFilterFixedString(filterBy);

        if (assetFilter->rowCount() == 0) {
            inform(tr("No available addresses to export under the selected filter"));
            return;
        }

        CSVModelWriter writer(filename);
        // name, column, role
        writer.setModel(assetFilter);
        writer.addColumn("txHash", AssetRewardsHistoryTableModel::Label, Qt::DisplayRole);
        writer.addColumn("Type", AssetRewardsHistoryTableModel::Type, Qt::DisplayRole);
        writer.addColumn("Start Time", AssetRewardsHistoryTableModel::Start, Qt::DisplayRole);
        writer.addColumn("End Time", AssetRewardsHistoryTableModel::End, Qt::DisplayRole);
        writer.addColumn("Balance", AssetRewardsHistoryTableModel::Balance, Qt::DisplayRole);
        writer.addColumn("Percent", AssetRewardsHistoryTableModel::Percent, Qt::DisplayRole);
        writer.addColumn("Amount", AssetRewardsHistoryTableModel::Amount, Qt::DisplayRole);

        fExport = writer.write();
    }

    if (fExport) {
        inform(tr("Exporting Successful\nThe address book was successfully saved to %1.").arg(filename));
    } else {
        inform(tr("Exporting Failed\nThere was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}

SettingsExportCSV::~SettingsExportCSV()
{
    delete ui;
}
