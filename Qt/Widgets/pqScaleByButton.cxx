// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqScaleByButton.h"

#include <QAction>

//-----------------------------------------------------------------------------
pqScaleByButton::pqScaleByButton(QWidget* parentObject)
  : Superclass(parentObject)
{
  QMap<double, QString> amap;
  amap[0.5] = "0.5X";
  amap[2] = "2X";
  this->constructor(amap);
}

//-----------------------------------------------------------------------------
pqScaleByButton::pqScaleByButton(
  const QList<double>& scaleFactors, const QString& suffix, QWidget* parentObject)
  : Superclass(parentObject)
{
  QMap<double, QString> amap;
  Q_FOREACH (double sf, scaleFactors)
  {
    amap[sf] = QString("%1%2").arg(sf).arg(suffix);
  }
  this->constructor(amap);
}

//-----------------------------------------------------------------------------
pqScaleByButton::pqScaleByButton(
  const QMap<double, QString>& scaleFactorsAndLabels, QWidget* parentObject)
  : Superclass(parentObject)
{
  this->constructor(scaleFactorsAndLabels);
}

//-----------------------------------------------------------------------------
pqScaleByButton::~pqScaleByButton() = default;

//-----------------------------------------------------------------------------
void pqScaleByButton::constructor(const QMap<double, QString>& scaleFactors)
{
  this->setToolButtonStyle(Qt::ToolButtonIconOnly);
  this->setToolTip(tr("Scale by ..."));
  this->setPopupMode(QToolButton::InstantPopup);

  int index = 0;
  for (auto iter = scaleFactors.begin(); iter != scaleFactors.end(); ++iter, ++index)
  {
    QAction* actn = new QAction(QIcon(":/QtWidgets/Icons/pqMultiply.png"), iter.value(), this);
    actn->setObjectName(QString("%1X").arg(iter.key()));
    actn->setData(iter.key());
    this->connect(actn, SIGNAL(triggered()), SLOT(scaleTriggered()));
    this->addAction(actn);
    if (index == 0)
    {
      this->setDefaultAction(actn);
    }
  }
}

//-----------------------------------------------------------------------------
void pqScaleByButton::scaleTriggered()
{
  if (QAction* senderAction = qobject_cast<QAction*>(this->sender()))
  {
    Q_EMIT this->scale(senderAction->data().toDouble());
  }
}
