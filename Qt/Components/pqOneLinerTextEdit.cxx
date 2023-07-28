// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOneLinerTextEdit.h"

// Server Manager Includes.

// Qt Includes.
#include <QKeyEvent>
#include <QMimeData>

//-----------------------------------------------------------------------------
pqOneLinerTextEdit::pqOneLinerTextEdit(QWidget* _parent)
  : Superclass(_parent)
{
  this->setLineWrapMode(WidgetWidth);
  this->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  this->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  this->connect(this, &QTextEdit::textChanged, this, &pqOneLinerTextEdit::adjustToText);
}

//-----------------------------------------------------------------------------
void pqOneLinerTextEdit::adjustToText()
{
  auto docSize = this->document()->size();
  this->setFixedHeight(
    docSize.height() + this->contentsMargins().top() + this->contentsMargins().bottom());
}

//-----------------------------------------------------------------------------
void pqOneLinerTextEdit::insertFromMimeData(const QMimeData* source)
{
  QString text = source->text().simplified();
  this->insertPlainText(text);
}

//-----------------------------------------------------------------------------
void pqOneLinerTextEdit::resizeEvent(QResizeEvent* event)
{
  this->Superclass::resizeEvent(event);
  this->adjustToText();
}

//-----------------------------------------------------------------------------
void pqOneLinerTextEdit::keyPressEvent(QKeyEvent* event)
{
  if (event)
  {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
      return;
    }
  }

  this->Superclass::keyPressEvent(event);
}
