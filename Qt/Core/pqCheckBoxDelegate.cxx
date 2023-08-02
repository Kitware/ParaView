// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCheckBoxDelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include <cmath>
#include <iostream>

namespace
{
QRect CheckBoxRect(const QStyleOptionViewItem& option)
{
  QStyleOptionButton checkBoxOption;
  QRect checkBoxRect =
    QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxOption);
  QPoint checkBoxPoint(option.rect.x() + option.rect.width() / 2 - checkBoxRect.width() / 2,
    option.rect.y() + option.rect.height() / 2 - checkBoxRect.height() / 2);
  return QRect(checkBoxPoint, checkBoxRect.size());
}
};

struct pqCheckBoxDelegate::pqInternals
{
  pqInternals()
  {
    const double PI = 3.141592653589793238463;
    this->ContractedPolygon << QPointF(1.0, 0.0) << QPointF(cos(2 * PI / 3), sin(2 * PI / 3))
                            << QPointF(cos(-2 * PI / 3), sin(-2 * PI / 3));
    this->ExpandedPolygon << QPointF(0.0, 1.0) << QPointF(cos(-PI / 6), sin(-PI / 6))
                          << QPointF(cos(-5 * PI / 6), sin(-5 * PI / 6));
  }
  QPolygonF ContractedPolygon;
  QPolygonF ExpandedPolygon;
};

pqCheckBoxDelegate::pqCheckBoxDelegate(QObject* _parent)
  : QStyledItemDelegate(_parent)
{
  this->Internals = new pqInternals();
}

pqCheckBoxDelegate::~pqCheckBoxDelegate()
{
  delete this->Internals;
}

void pqCheckBoxDelegate::paint(
  QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  QVariant d = index.model()->data(index, Qt::DisplayRole);
  if (!d.isNull())
  {
    int checked = d.toInt();
    QRect rect = CheckBoxRect(option);
    QPalette palette = qApp->palette();
    if (checked == pqCheckBoxDelegate::NOT_EXPANDED_DISABLED)
    {
      checked = pqCheckBoxDelegate::NOT_EXPANDED;
      painter->setBrush(palette.brush(QPalette::Disabled, QPalette::ButtonText));
    }
    else
    {
      painter->setBrush(palette.brush(QPalette::Active, QPalette::ButtonText));
    }
    double PaintingScaleFactor = ((double)rect.bottomLeft().y() - rect.topLeft().y()) * 0.5;
    painter->save();
    painter->translate(rect.center());
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    switch (checked)
    {
      case pqCheckBoxDelegate::EXPANDED:
        painter->drawPolygon(this->Internals->ExpandedPolygon, Qt::WindingFill);
        break;
      case pqCheckBoxDelegate::NOT_EXPANDED:
        painter->drawPolygon(this->Internals->ContractedPolygon, Qt::WindingFill);
        break;
    }
    painter->restore();
  }
}

bool pqCheckBoxDelegate::editorEvent(QEvent* _event, QAbstractItemModel* model,
  const QStyleOptionViewItem& option, const QModelIndex& index)
{
  QVariant d = index.model()->data(index, Qt::DisplayRole);
  if (!d.isNull())
  {
    int checked = d.toInt();
    if (checked != pqCheckBoxDelegate::NOT_EXPANDED_DISABLED)
    {
      if ((_event->type() == QEvent::MouseButtonRelease) ||
        (_event->type() == QEvent::MouseButtonDblClick))
      {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(_event);
        if (mouse_event->button() != Qt::LeftButton ||
          !CheckBoxRect(option).contains(mouse_event->pos()))
        {
          return false;
        }
        if (_event->type() == QEvent::MouseButtonDblClick)
        {
          return true;
        }
      }
      else if (_event->type() == QEvent::KeyPress)
      {
        if (static_cast<QKeyEvent*>(_event)->key() != Qt::Key_Space &&
          static_cast<QKeyEvent*>(_event)->key() != Qt::Key_Select)
        {
          return false;
        }
      }
      else
      {
        return false;
      }

      return model->setData(index, !checked, Qt::EditRole);
    }
  }
  return false;
}
