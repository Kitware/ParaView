/*=========================================================================

   Program: ParaView
   Module:    pqCheckBoxPixMaps.cxx

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
#include "pqCheckBoxPixMaps.h"

#include "assert.h"

#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionButton>
#include <QWidget>

#include <cassert>

//-----------------------------------------------------------------------------
pqCheckBoxPixMaps::pqCheckBoxPixMaps(QWidget* parentWidget)
  : Superclass(parentWidget)
{
  assert(parentWidget != 0);

  // Initialize the pixmaps. The following style array should
  // correspond to the PixmapStateIndex enum.
  const QStyle::State PixmapStyle[] = { QStyle::State_On | QStyle::State_Enabled,
    QStyle::State_NoChange | QStyle::State_Enabled, QStyle::State_Off | QStyle::State_Enabled,
    QStyle::State_On | QStyle::State_Enabled | QStyle::State_Active,
    QStyle::State_NoChange | QStyle::State_Enabled | QStyle::State_Active,
    QStyle::State_Off | QStyle::State_Enabled | QStyle::State_Active };

  QStyleOptionButton option;
  QRect r =
    parentWidget->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, parentWidget);
  option.rect = QRect(QPoint(0, 0), r.size());
  for (int i = 0; i < pqCheckBoxPixMaps::PixmapCount; i++)
  {
    this->Pixmaps[i] = QPixmap(r.size());
    this->Pixmaps[i].fill(QColor(0, 0, 0, 0));
    QPainter painter(&this->Pixmaps[i]);
    option.state = PixmapStyle[i];
    parentWidget->style()->drawPrimitive(
      QStyle::PE_IndicatorCheckBox, &option, &painter, parentWidget);
  }
}

//-----------------------------------------------------------------------------
QPixmap pqCheckBoxPixMaps::getPixmap(Qt::CheckState state, bool active) const
{
  int offset = active ? 3 : 0;
  switch (state)
  {
    case Qt::Checked:
      return this->Pixmaps[offset + Checked];

    case Qt::Unchecked:
      return this->Pixmaps[offset + UnChecked];

    case Qt::PartiallyChecked:
      return this->Pixmaps[offset + PartiallyChecked];
  }

  return QPixmap();
}
