/*=========================================================================

   Program: ParaView
   Module:  pqScaleByButton.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
  foreach (double sf, scaleFactors)
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
  this->setToolTip("Scale by ...");
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
