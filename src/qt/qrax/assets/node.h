// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MULTIMININGTREENODE_H
#define MULTIMININGTREENODE_H

#include "uint256.h"
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsObject>
#include "qt/qrax/assets/widget.h"
#include "qt/qrax/assets/edge.h"

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

class AssetsWidget;
class QAssetsGraphicsScene;

class QmultiMiningTreeNode : public QGraphicsItem
{

public:

	QmultiMiningTreeNode();

	/*QmultiMiningTreeNode() {
        this->rx = 25;
        this->ry = 25;
	};*/

    ~QmultiMiningTreeNode()
    {

    }

    QmultiMiningTreeNode(int rX, int rY) {
        this->rx = rX;
        this->ry = rY;
	};

    void setLevel(const uint8_t &level){
        this->level = level;
    };

    uint8_t getLevel()
    {
        return this->level;
    };

    void setPosition(const uint16_t &position) {
        this->position = position;
    }

    uint16_t getPosition()
    {
        return this->position;
    };

    CKeyID getKeyid() const;
    void setKeyid(const CKeyID &value);

    CKeyID getParentId() const;
    void setParentId(const CKeyID &value);

    void AddEdge(QAssetEdge *edge)
    {
        edgeList << edge;
        edge->adjust();
    }

    void AddChildEdge(QAssetEdge *edge)
    {
        childEdges << edge;
        edge->adjust();
    }

    uint16_t getChildCount() const;
    void setChildCount(const uint16_t &value);

    QmultiMiningTreeNode *getParentNode() const;
    void setParentNode(QmultiMiningTreeNode *value);

    void addChildNode(QmultiMiningTreeNode *value);

    QVector<QmultiMiningTreeNode *> getChildNodes() const;
    void moveSiblingNodes(int direction = 1);

private:
    int rx;
    int ry;
    CKeyID keyid;
    CKeyID parentId;

    uint8_t level;
    uint16_t position;
    uint16_t childCount;

    QVector<QAssetEdge *> childEdges;
    QVector<QAssetEdge *> edgeList;

    QmultiMiningTreeNode *parentNode{nullptr};
    QVector<QmultiMiningTreeNode *> childNodes;

    QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;


protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;


};

#endif // MULTIMININGTREENODE_H
