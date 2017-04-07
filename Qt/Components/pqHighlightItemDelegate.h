/*=========================================================================

   Program: ParaView
   Module:    pqTreeView.h

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
#ifndef _pqHighlightRowItemDelegate_h
#define _pqHighlightRowItemDelegate_h

#include "pqComponentsModule.h"
#include <QStyledItemDelegate>

/**
* pqHighlightItemDelegate is a delegate used to highlight item views
* It is currently used to highlight matching items found using the
* pqItemViewSearchWidget. It works by repainting the item with a
* colored background.
*/

class PQCOMPONENTS_EXPORT pqHighlightItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  /**
  * Construct the pqHighlightItemDelegate
  * The variable color is used to specify the highlight color,
  * defaults to QColor(Qt::white)
  */
  pqHighlightItemDelegate(QColor color = QColor(Qt::white), QObject* parentObject = 0)
    : QStyledItemDelegate(parentObject)
    , HighlightColor(color)
  {
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
  QColor HighlightColor;
};

#endif
