// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCheckBoxPixMaps.h"

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
