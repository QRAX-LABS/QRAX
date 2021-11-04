// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QWidget>
#include <QObject>
#include <QGraphicsView>

class QAssetGraphics : public QGraphicsView
{
public:
    QAssetGraphics(QWidget *parent = nullptr);
    virtual ~QAssetGraphics(){};

};

#endif // GRAPHICSVIEW_H
