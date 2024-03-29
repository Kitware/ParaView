// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHeaderView.h"

#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqHeaderView::pqHeaderView(Qt::Orientation orientation, QWidget* parentObject)
  : Superclass(orientation, parentObject)
  , ToggleCheckStateOnSectionClick(false)
  , CustomIndicatorShown(false)
{
  if (orientation == Qt::Vertical)
  {
    qDebug("pqHeaderView currently doesn't support vertical headers. "
           "You may encounter rendering artifacts.");
  }
}

//-----------------------------------------------------------------------------
pqHeaderView::~pqHeaderView() = default;

//-----------------------------------------------------------------------------
void pqHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
  if (!rect.isValid())
  {
    return;
  }

  auto amodel = this->model();
  const QVariant hdata = amodel->headerData(logicalIndex, this->orientation(), Qt::CheckStateRole);
  if (!hdata.isValid())
  {
    // if the model is not giving any header data for CheckStateRole, then we
    // don't have any check support; nothing to do.
    this->Superclass::paintSection(painter, rect, logicalIndex);
    return;
  }

  // fill background for the whole section.
  QStyleOptionHeader hoption;
  this->initStyleOption(&hoption);
  hoption.section = logicalIndex;
  hoption.rect = rect;
  this->style()->drawControl(QStyle::CE_HeaderSection, &hoption, painter, this);

  // draw the checkbox.
  QStyleOptionViewItem coption;
  coption.initFrom(this);
  coption.features = QStyleOptionViewItem::HasCheckIndicator | QStyleOptionViewItem::HasDisplay;
  coption.viewItemPosition = QStyleOptionViewItem::OnlyOne;
  QRect checkRect =
    this->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &coption, this);
  checkRect.moveLeft(rect.x() + checkRect.x());
  coption.rect = checkRect;
  coption.state = coption.state & ~QStyle::State_HasFocus;
  switch (hdata.value<Qt::CheckState>())
  {
    case Qt::Checked:
      coption.state |= QStyle::State_On;
      break;
    case Qt::PartiallyChecked:
      coption.state |= QStyle::State_NoChange;
      break;
    case Qt::Unchecked:
    default:
      coption.state |= QStyle::State_Off;
      break;
  }
  this->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &coption, painter, this);

  // finally draw the header in offset by size of the checkbox.
  // let's determine the location where the "label" would be typically drawn in
  // an item when a check. we'll place the header text at that location.
  QStyleOptionViewItem vioption;
  vioption.initFrom(this);
  vioption.features = QStyleOptionViewItem::HasCheckIndicator | QStyleOptionViewItem::HasDisplay;
  vioption.viewItemPosition = QStyleOptionViewItem::OnlyOne;
  const QRect r2 = this->style()->subElementRect(QStyle::SE_ItemViewItemText, &vioption, this);

  QRect newrect = rect;
  newrect.setLeft(newrect.x() + r2.x());
  if (this->CustomIndicatorShown)
  {
    // adjust width so that sort indicator doesn't overlap with the custom
    // indicators.
    newrect.setRight(
      newrect.right() - (checkRect.width() * static_cast<int>(this->CustomIndicatorIcons.size())));
  }

  painter->save();
  this->Superclass::paintSection(painter, newrect, logicalIndex);
  painter->restore();

  // let's save the checkbox rect to handle clicks.
  this->CheckRect = checkRect;

  this->CustomIndicatorRects.clear();
  if (this->CustomIndicatorShown)
  {
    int leftpos = newrect.right();
    for (auto iter = this->CustomIndicatorIcons.rbegin(); iter != this->CustomIndicatorIcons.rend();
         ++iter)
    {
      const auto& icon = iter->first;
      const auto& role = iter->second;

      QRect ciRect = checkRect;
      ciRect.moveTo(leftpos, ciRect.y());
      this->style()->drawItemPixmap(painter, ciRect, Qt::AlignCenter, icon.pixmap(ciRect.size()));
      this->CustomIndicatorRects.push_back(std::make_pair(ciRect, role));

      leftpos += checkRect.width();
    }
  }
}

//-----------------------------------------------------------------------------
void pqHeaderView::mousePressEvent(QMouseEvent* evt)
{
  this->PressPosition = evt->pos();
}

//-----------------------------------------------------------------------------
void pqHeaderView::mouseReleaseEvent(QMouseEvent* evt)
{
  bool handled = false;
  if ((evt->pos() - this->PressPosition).manhattanLength() < 3)
  {
    handled = this->mouseClickEvent(evt);
  }
  this->PressPosition = QPoint();
  if (!handled)
  {
    // allow superclass events to process, so header sorting works.
    this->Superclass::mousePressEvent(evt);
    this->Superclass::mouseReleaseEvent(evt);
  }
}

//-----------------------------------------------------------------------------
bool pqHeaderView::mouseClickEvent(QMouseEvent* evt)
{
  if (evt->button() != Qt::LeftButton)
  {
    return false;
  }

  if (auto amodel = this->model())
  {
    int logicalIndex = this->logicalIndexAt(evt->pos());
    if (this->CustomIndicatorShown)
    {
      for (const auto& pair : this->CustomIndicatorRects)
      {
        const auto& rect = pair.first;
        const auto& role = pair.second;
        if (rect.contains(this->PressPosition))
        {
          Q_EMIT this->customIndicatorClicked(logicalIndex, rect.bottomLeft(), role);
          return true;
        }
      }
    }

    const QVariant hdata =
      amodel->headerData(logicalIndex, this->orientation(), Qt::CheckStateRole);
    if (hdata.isValid())
    {
      if (this->ToggleCheckStateOnSectionClick == true ||
        (this->CheckRect.isValid() && this->CheckRect.contains(evt->pos())))
      {
        QVariant newdata;
        // supports toggling checkstate.
        switch (hdata.value<Qt::CheckState>())
        {
          case Qt::Checked:
            newdata.setValue(Qt::Unchecked);
            break;
          default:
            newdata.setValue(Qt::Checked);
            break;
        }
        amodel->setHeaderData(logicalIndex, this->orientation(), newdata, Qt::CheckStateRole);
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqHeaderView::setCustomIndicatorShown(bool val)
{
  if (this->CustomIndicatorShown != val)
  {
    this->CustomIndicatorShown = val;
    this->update();
  }
}

//-----------------------------------------------------------------------------
void pqHeaderView::addCustomIndicatorIcon(const QIcon& icon, const QString& role)
{
  for (auto& pair : this->CustomIndicatorIcons)
  {
    if (pair.second == role)
    {
      pair.first = icon;
      return;
    }
  }

  this->CustomIndicatorIcons.push_back(std::make_pair(icon, role));
  this->update();
}

//-----------------------------------------------------------------------------
void pqHeaderView::removeCustomIndicatorIcon(const QString& role)
{
  for (auto iter = this->CustomIndicatorIcons.begin(); iter != this->CustomIndicatorIcons.end();)
  {
    if (iter->second == role)
    {
      iter = this->CustomIndicatorIcons.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

//-----------------------------------------------------------------------------
QIcon pqHeaderView::customIndicatorIcon(const QString& role) const
{
  for (auto& pair : this->CustomIndicatorIcons)
  {
    if (pair.second == role)
    {
      return pair.first;
    }
  }
  return QIcon();
}
