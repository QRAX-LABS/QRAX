// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SETTINGSEXPORTCSV_H
#define SETTINGSEXPORTCSV_H

#include <QWidget>
#include "qt/qrax/pwidget.h"
#include "transactionfilterproxy.h"
#include <QSortFilterProxyModel>

namespace Ui {
class SettingsExportCSV;
}

class SettingsExportCSV : public PWidget
{
    Q_OBJECT

public:
    explicit SettingsExportCSV(QRAXGUI* _window, QWidget *parent = nullptr);
    ~SettingsExportCSV();


private:
    Ui::SettingsExportCSV *ui;
    TransactionFilterProxy* txFilter{nullptr};
    QSortFilterProxyModel* addressFilter{nullptr};
    QSortFilterProxyModel* assetFilter{nullptr};

    enum ExportType {
        EXPORT_TX,
        EXPORT_ADDRESS,
        EXPORT_ASSETS
    };

private Q_SLOTS:
    void selectFileOutput(const ExportType& isTxExport);
    void exportTxes(const QString& filename);
    void exportAddrs(const QString& filename);
    void exportAssetHistory(const QString& filename);

};

#endif // SETTINGSEXPORTCSV_H
