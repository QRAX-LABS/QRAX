// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/guiutil.h"
#include "qt/qrax/qtutils.h"
#include "qt/walletmodel.h"
#include "qt/qrax/assets/graphicsscene.h"
#include "qt/qrax/assets/node.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QTransform>

class AssetsWidget;
class QmultiMiningTreeNode;

QmultiMiningTreeNode::QmultiMiningTreeNode(AssetsWidget *widget)
    : widget(widget)
{
    setFlag(ItemIsMovable, false);
    setFlag(ItemIsFocusable);
    setFlag(ItemSendsGeometryChanges);
	setCacheMode(ItemCoordinateCache);
    //setZValue(-1);
    this->rx = 25;
    this->ry = 25;
}

CKeyID QmultiMiningTreeNode::getKeyid() const
{
    return keyid;
}

void QmultiMiningTreeNode::setKeyid(const CKeyID &value)
{
    keyid = value;
}

CKeyID QmultiMiningTreeNode::getParentId() const
{
    return parentId;
}

void QmultiMiningTreeNode::setParentId(const CKeyID &value)
{
    parentId = value;
}

uint16_t QmultiMiningTreeNode::getChildCount() const
{
    return childCount;
}

void QmultiMiningTreeNode::setChildCount(const uint16_t &value)
{
    childCount = value;
}

QmultiMiningTreeNode *QmultiMiningTreeNode::getParentNode() const
{
    return parentNode;
}

void QmultiMiningTreeNode::setParentNode(QmultiMiningTreeNode *value)
{
    parentNode = value;
}

void QmultiMiningTreeNode::addChildNode(QmultiMiningTreeNode *value)
{
    childNodes << value;
}

QVector<QmultiMiningTreeNode *> QmultiMiningTreeNode::getChildNodes() const
{
    return childNodes;
}

QRectF QmultiMiningTreeNode::boundingRect() const
{
    return QRectF(-30,-30,60,60);
}

void QmultiMiningTreeNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{

        QPainterPath circle_path;
        painter->setBrush(Qt::darkBlue);
        painter->setPen(QPen(Qt::darkGreen));
        circle_path.addEllipse(-this->rx, -this->ry, this->rx * 2, this->ry * 2);

        QFont myFont;
        QPointF baseline(-this->rx / 3, this->ry / 4);

        circle_path.addText(baseline, myFont, QString::fromStdString(this->keyid.ToString().substr(0, 4)));
        painter->drawPath(circle_path);
        Q_UNUSED(option);
        Q_UNUSED(widget);
}

void QmultiMiningTreeNode::moveSiblingNodes(int direction)
{
    if (scene()) {
        QList<QGraphicsItem *> allItems = scene()->items(Qt::AscendingOrder);
        QVector<QmultiMiningTreeNode *> nodes;
        for (QGraphicsItem* item: allItems)
        {
            if (QmultiMiningTreeNode *nodeItem = dynamic_cast<QmultiMiningTreeNode*>(item)) {

                if (nodeItem->getParentId() == getParentId()) {
                    if (direction > 0 && nodeItem->getPosition() > getPosition())
                    {
                        nodes << nodeItem;
                    } else if (direction < 0 && nodeItem->getPosition() < getPosition())
                    {
                        nodes << nodeItem;
                    }
                }
            }
        }

        for (QmultiMiningTreeNode * node: nodes) {
            node->moveBy(1 * direction, 0);
        }

    }
}

QVariant QmultiMiningTreeNode::itemChange(GraphicsItemChange change, const QVariant &value)
{

    switch (change) {
    case ItemPositionHasChanged:
        for (QAssetEdge *edge : qAsConst(edgeList))
            edge->adjust();

        for (QAssetEdge *edge : qAsConst(childEdges))
            edge->moveChild();

        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}
