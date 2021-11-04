// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef EDGE_H
#define EDGE_H

#include <QWidget>
#include <QGraphicsItem>

class QmultiMiningTreeNode;

class QAssetEdge : public QGraphicsItem
{
public:
    QAssetEdge(QmultiMiningTreeNode *parent, QmultiMiningTreeNode *child);

    QmultiMiningTreeNode *parentNode() const;
    QmultiMiningTreeNode *childNode() const;

    ~QAssetEdge(){};
    void adjust();
    void moveChild();

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QmultiMiningTreeNode *parent, *child;
    QPointF sourcePoint, prevSourcePoint;
    QPointF destPoint;

};

#endif // EDGE_H
