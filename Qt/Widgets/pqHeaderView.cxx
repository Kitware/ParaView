/*=========================================================================

   Program: ParaView
   Module:  pqHeaderView.cxx

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
#include "pqHeaderView.h"

#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QStyle>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqHeaderView::pqHeaderView(Qt::Orientation orientation, QWidget* parentObject)
  : Superclass(orientation, parentObject)
  , ToggleCheckStateOnSectionClick(false)
{
  if (orientation == Qt::Vertical)
  {
    qDebug("pqHeaderView currently doesn't support vertical headers. "
           "You may encounter rendering artifacts.");
  }
}

//-----------------------------------------------------------------------------
pqHeaderView::~pqHeaderView()
{
}

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
  this->style()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &coption, painter, this);

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
  this->Superclass::paintSection(painter, newrect, logicalIndex);

  // let's save the checkbox rect to handle clicks.
  this->CheckRect = checkRect;
}

//-----------------------------------------------------------------------------
void pqHeaderView::mousePressEvent(QMouseEvent* evt)
{
  this->PressPosition = evt->pos();
}

//-----------------------------------------------------------------------------
void pqHeaderView::mouseReleaseEvent(QMouseEvent* evt)
{
  if ((evt->pos() - this->PressPosition).manhattanLength() < 3)
  {
    this->mouseClickEvent(evt);
  }
  this->PressPosition = QPoint();
}

//-----------------------------------------------------------------------------
void pqHeaderView::mouseClickEvent(QMouseEvent* evt)
{
  if (auto amodel = this->model())
  {
    int logicalIndex = this->logicalIndexAt(evt->pos());
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
      }
    }
  }
}
