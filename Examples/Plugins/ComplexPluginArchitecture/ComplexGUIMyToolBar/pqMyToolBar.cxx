// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqMyToolBar.h"

#include "vtkSharedUtils.h"

#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QStyle>

//-----------------------------------------------------------------------------
pqMyToolBar::pqMyToolBar(const QString& title, QWidget* parentW)
  : Superclass(title, parentW)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqMyToolBar::pqMyToolBar(QWidget* parentW)
  : Superclass(parentW)
{
  this->setWindowTitle(tr("My Toolbar (Examples)"));
  this->constructor();
}

//-----------------------------------------------------------------------------
void pqMyToolBar::constructor()
{
  this->setObjectName("MyToolBar");
  this->addWidget(new QLabel(tr("Custom Toolbar"), this));
  this->addAction(
    qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("My Action"), []() {
      QMessageBox::information(nullptr, tr("MyAction"),
        tr("Did you know that Pi value in degrees is %1 ?")
          .arg(QString::number(vtkSharedUtils::DegreesFromRadians(vtkSharedUtils::Pi()))));
    });
}
