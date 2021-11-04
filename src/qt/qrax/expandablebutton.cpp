// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/qrax/expandablebutton.h"
#include "qt/qrax/forms/ui_expandablebutton.h"
#include "qt/qrax/qtutils.h"
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStyle>
#include <QToolTip>
#include <iostream>

ExpandableButton::ExpandableButton(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExpandableButton),
    isAnimating(true)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());
    ui->pushButton->setCheckable(false);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    sync = new QMovie("://ic-check-sync-progress");

    connect(sync, &QMovie::frameChanged, [=]{
      ui->pushButton->setIcon(sync->currentPixmap());
    });

    connect(ui->pushButton, &QPushButton::clicked, this, &ExpandableButton::innerMousePressEvent);
}

void ExpandableButton::setButtonClassStyle(const char *name, const QVariant &value, bool forceUpdate)
{
    ui->pushButton->setProperty(name, value);
    if (forceUpdate) {
        updateStyle(ui->pushButton);
    }
}
void ExpandableButton::syncIconVisible(bool visible)
{
    if (visible) {
        sync->start();
    } else {
        sync->stop();
    }
}

void ExpandableButton::setIcon(QString path)
{
    ui->pushButton->setIcon(QIcon(path));
}

void ExpandableButton::setButtonText(const QString& _text)
{
    this->text = _text;
    this->setToolTip(_text);
    if (this->isExpanded) {
        ui->pushButton->setText(_text);
    }
}

void ExpandableButton::setNoIconText(const QString& _text)
{
    notExpandedText = _text;
    if (!this->isExpanded)
        ui->pushButton->setText(_text);
}

void ExpandableButton::setText2(QString text2)
{
    this->text = text2;
    ui->pushButton->setText(text2);
}

ExpandableButton::~ExpandableButton()
{
    delete ui;
}

bool ExpandableButton::isChecked()
{
    return ui->pushButton->isChecked();
}

void ExpandableButton::setChecked(bool check)
{
    ui->pushButton->setChecked(check);
}

void ExpandableButton::setSmall()
{
    ui->pushButton->setText(notExpandedText);
    this->setMaximumWidth(36);
    this->isExpanded = false;
    update();
}

void ExpandableButton::setExpanded()
{
    this->setMaximumWidth(100);
    ui->pushButton->setText(text);
    this->isExpanded = true;
}

void ExpandableButton::enterEvent(QEvent *event)
{
    ui->pushButton->setToolTip(getText());
    update();
}

void ExpandableButton::leaveEvent(QEvent *)
{

}


void ExpandableButton::innerMousePressEvent()
{
    Q_EMIT Mouse_Pressed();
}

void ExpandableButton::on_pushButton_clicked(bool checked)
{
    // TODO: Add callback event
}
