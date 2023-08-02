// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLineEdit.h"

// Server Manager Includes.

// Qt Includes.
#include <QFocusEvent>
#include <QTimer>

// ParaView Includes.

//-----------------------------------------------------------------------------
pqLineEdit::pqLineEdit(QWidget* _parent)
  : Superclass(_parent)
  , EditingFinishedPending(false)
  , ResetCursorPositionOnEditingFinished(true)
{
  this->connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  this->connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(onTextEdited()));
}

//-----------------------------------------------------------------------------
pqLineEdit::pqLineEdit(const QString& _contents, QWidget* _parent)
  : Superclass(_contents, _parent)
  , EditingFinishedPending(false)
  , ResetCursorPositionOnEditingFinished(true)
{
  this->connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  this->connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(onTextEdited()));
}

//-----------------------------------------------------------------------------
pqLineEdit::~pqLineEdit() = default;

//-----------------------------------------------------------------------------
void pqLineEdit::onTextEdited()
{
  this->EditingFinishedPending = true;
}

//-----------------------------------------------------------------------------
void pqLineEdit::onEditingFinished()
{
  if (this->EditingFinishedPending)
  {
    Q_EMIT this->textChangedAndEditingFinished();
    this->EditingFinishedPending = false;
  }
  if (this->ResetCursorPositionOnEditingFinished)
  {
    this->setCursorPosition(0);
  }
}

//-----------------------------------------------------------------------------
void pqLineEdit::setTextAndResetCursor(const QString& val)
{
  if (this->text() != val)
  {
    this->Superclass::setText(val);
    this->setCursorPosition(0);
  }
}

//-----------------------------------------------------------------------------
void pqLineEdit::triggerTextChangedAndEditingFinished()
{
  // Since we do not update this->EditingFinishedPending when the text is
  // changed programmatically, textChangedAndEditingFinished() wasn't getting
  // fired after setText() was called during test playback. To overcome that
  // issue, the playback manually calls this method.
  this->onTextEdited();
  this->onEditingFinished();
}

//-----------------------------------------------------------------------------
void pqLineEdit::focusInEvent(QFocusEvent* evt)
{
  // First let the base class process the event
  this->Superclass::focusInEvent(evt);

  if (evt)
  {
    switch (evt->reason())
    {
      case Qt::MouseFocusReason:
      case Qt::TabFocusReason:
      case Qt::BacktabFocusReason:
      case Qt::ShortcutFocusReason:
        // Then select the text by a single shot timer, so that everything will
        // be processed before (calling selectAll() directly won't work)
        QTimer::singleShot(0, this, &QLineEdit::selectAll);
        break;
      default:
        break;
    }
  }
}
