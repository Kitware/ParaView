// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLinkedObjectPythonTextArea.h"
#include "pqPythonTextArea.h"

//-----------------------------------------------------------------------------
pqLinkedObjectPythonTextArea::pqLinkedObjectPythonTextArea(pqPythonTextArea& textArea) noexcept
  : pqLinkedObjectQTextEdit(*textArea.getTextEdit())
  , TextArea(textArea)
{
}

//-----------------------------------------------------------------------------
void pqLinkedObjectPythonTextArea::link(pqLinkedObjectInterface* other)
{
  if (other)
  {
    const QUndoStack& undoStack = this->TextArea.getUndoStack();
    this->ConnectedTo = other;
    this->Connection = QObject::connect(&undoStack, &QUndoStack::indexChanged, [this]() {
      if (!this->SettingText)
      {
        this->ConnectedTo->setText(this->getText());
      }
    });
  }
}
