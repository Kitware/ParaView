// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPopOutWidget.h"

#include "pqApplicationCore.h"
#include "pqDialog.h"
#include "pqSettings.h"

#include <QIcon>
#include <QLayout>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QWidget>

#include <cassert>

class pqPopOutWidget::pqInternal
{
public:
  pqSettings* Settings;
  QString Title;
  QPointer<QHBoxLayout> Layout;
  QPointer<QWidget> WidgetToPopOut;
  QPointer<pqDialog> Dialog;
  QPointer<QPushButton> Button;
  int Index;
  bool WidgetIsInDialog;
  pqInternal()
    : Settings(nullptr)
    , Layout(nullptr)
    , WidgetToPopOut(nullptr)
    , Dialog(nullptr)
    , Button(nullptr)
    , Index(-1)
    , WidgetIsInDialog(false)
  {
    Settings = pqApplicationCore::instance()->settings();
  }
};

//------------------------------------------------------------------------------
pqPopOutWidget::pqPopOutWidget(QWidget* widgetToPopOut, const QString& dialogTitle, QWidget* p)
  : QWidget(p)
  , Internals(new pqInternal)
{
  this->Internals->Title = dialogTitle;
  this->Internals->Layout = new QHBoxLayout(this);
  this->Internals->Layout->setContentsMargins(0, 0, 0, 0);
  this->Internals->Layout->setSpacing(0);
  this->Internals->WidgetToPopOut = widgetToPopOut;
  this->Internals->Layout->addWidget(this->Internals->WidgetToPopOut);
  this->Internals->Dialog = new pqDialog(this);
  this->Internals->Dialog->setWindowTitle(dialogTitle);
  this->Internals->Dialog->setLayout(new QHBoxLayout(this->Internals->Dialog));
  this->connect(
    this->Internals->Dialog, SIGNAL(finished(int)), this, SLOT(moveWidgetBackToParent()));
  this->Internals->Index = 0;
}

//------------------------------------------------------------------------------
pqPopOutWidget::~pqPopOutWidget()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
void pqPopOutWidget::setPopOutButton(QPushButton* button)
{
  // It could handle multiple buttons, but doesn't right now
  assert(this->Internals->Button.isNull());
  this->Internals->Button = button;
  if (this->Internals->WidgetIsInDialog)
  {
    this->Internals->Button->setIcon(this->style()->standardIcon(QStyle::SP_TitleBarNormalButton));
  }
  else
  {
    this->Internals->Button->setIcon(this->style()->standardIcon(QStyle::SP_TitleBarMaxButton));
  }
  this->connect(this->Internals->Button, SIGNAL(clicked()), this, SLOT(toggleWidgetLocation()));
}

//------------------------------------------------------------------------------
void pqPopOutWidget::toggleWidgetLocation()
{
  if (this->Internals->WidgetIsInDialog)
  {
    this->moveWidgetBackToParent();
  }
  else
  {
    this->moveWidgetToDialog();
  }
}

//------------------------------------------------------------------------------
void pqPopOutWidget::moveWidgetToDialog()
{
  if (!this->Internals->WidgetIsInDialog)
  {
    this->Internals->Dialog->layout()->addWidget(this->Internals->WidgetToPopOut);
    this->Internals->WidgetIsInDialog = true;
    this->Internals->Settings->restoreState(
      this->Internals->Title, *this->Internals->Dialog.data());
    this->Internals->Dialog->show();
    if (this->Internals->Button)
    {
      this->Internals->Button->setIcon(
        this->style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    }
  }
}

//------------------------------------------------------------------------------
void pqPopOutWidget::moveWidgetBackToParent()
{
  if (this->Internals->Dialog->isVisible())
  {
    this->Internals->Dialog->hide();
  }
  if (this->Internals->WidgetIsInDialog)
  {
    this->Internals->Settings->saveState(*this->Internals->Dialog.data(), this->Internals->Title);
    this->Internals->Layout->insertWidget(this->Internals->Index, this->Internals->WidgetToPopOut);
    this->Internals->WidgetIsInDialog = false;
    if (this->Internals->Button)
    {
      this->Internals->Button->setIcon(this->style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    }
  }
}
