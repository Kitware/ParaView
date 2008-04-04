/*=========================================================================

   Program: ParaView
   Module:    pqColorTableDelegate.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqColorTableDelegate.cxx
/// \date 8/11/2006

#include "pqColorTableDelegate.h"

#include <QApplication>
#include <QModelIndex>
#include <QPainter>
#include <QSize>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QStyleOptionViewItem>
#include <QVariant>


pqColorTableDelegate::pqColorTableDelegate(QObject *parentObject)
  : QAbstractItemDelegate(parentObject)
{
  this->ColorSize = 16;
}

QSize pqColorTableDelegate::sizeHint(const QStyleOptionViewItem &/*option*/,
    const QModelIndex &index) const
{
  QVariant value = index.data(Qt::SizeHintRole);
  if(value.isValid())
    {
    return qvariant_cast<QSize>(value);
    }

  // No decoration is shown, so the prefered size is the color size.
  return QSize(this->ColorSize, this->ColorSize);
}

void pqColorTableDelegate::paint(QPainter *painter,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if(!index.isValid())
    {
    return;
    }

  // Set the color group.
  QStyleOptionViewItem opt = option;
  opt.palette.setCurrentColorGroup(option.state & QStyle::State_Enabled ?
      QPalette::Active : QPalette::Disabled);

  // If the index is selected, paint the highlight in the background.
  // Paint the color's border as well.
  QRect border = option.rect;
  border.setTop(border.top() + 1);
  border.setBottom(border.bottom() - 2);
  border.setLeft(border.left() + 1);
  border.setRight(border.right() - 2);
  if(option.state & QStyle::State_Selected)
    {
    painter->fillRect(option.rect, opt.palette.brush(QPalette::Highlight));
    painter->setPen(opt.palette.color(QPalette::HighlightedText));
    painter->drawRect(border);
    }
  else
    {
    painter->setPen(opt.palette.color(QPalette::Text));
    painter->drawRect(border);
    }

  // Paint the color.
  QColor color = qvariant_cast<QColor>(index.data(Qt::DisplayRole));
  if(!color.isValid())
    {
    color = Qt::white;
    }

  border.setTop(border.top() + 1);
  border.setLeft(border.left() + 1);
  painter->fillRect(border, color);

  // Draw the focus rect if needed.
  if (option.state & QStyle::State_HasFocus)
    {
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(opt);
    o.rect = option.rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    o.backgroundColor = opt.palette.color(opt.state & QStyle::State_Selected ?
        QPalette::Highlight : QPalette::Background);
    QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
    }
}


