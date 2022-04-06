// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "edge.h"
#include "node.h"

QAssetEdge::QAssetEdge(QmultiMiningTreeNode *parent, QmultiMiningTreeNode *child)
    : parent(parent), child(child)
{
    setFlag(ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::NoButton);
	setCacheMode(DeviceCoordinateCache);
    parent->AddEdge(this);
    child->AddEdge(this);
    //adjust();
}

QmultiMiningTreeNode *QAssetEdge::parentNode() const
{
    return parent;
}

QmultiMiningTreeNode *QAssetEdge::childNode() const
{
    return child;
}

void QAssetEdge::adjust()
{
    if (!parent || !child)
        return;

    QLineF line(mapFromItem(parent, 0, 0), mapFromItem(child, 0, 0));
    prepareGeometryChange();

    prevSourcePoint = sourcePoint;

    sourcePoint = line.p1();// + edgeOffset;
    destPoint = line.p2();// - edgeOffset;

}

void QAssetEdge::moveChild()
{
    if (!parent || !child)
        return;

    child->setPos(child->x() + sourcePoint.x() - prevSourcePoint.x(), child->y());
}

QRectF QAssetEdge::boundingRect() const
{
    if (!parent || !child)
        return QRectF();

    qreal penWidth = 2;
    qreal extra = penWidth + 30 / 2.0;

    return QRectF(sourcePoint, QSizeF(destPoint.x() - sourcePoint.x(),
                                      destPoint.y() - sourcePoint.y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}

void QAssetEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!parent || !child)
        return;

    QLineF line(sourcePoint, destPoint);
    if (qFuzzyCompare(line.length(), qreal(0.)))
        return;

    painter->setPen(QPen(Qt::darkGreen, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(line);


}

QVariant QAssetEdge::itemChange(GraphicsItemChange change, const QVariant &value)
{

    return QGraphicsItem::itemChange(change, value);
}
