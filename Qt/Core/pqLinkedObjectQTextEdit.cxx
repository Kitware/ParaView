// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLinkedObjectQTextEdit.h"

#include <QTextEdit>

//-----------------------------------------------------------------------------
void pqLinkedObjectQTextEdit::link(pqLinkedObjectInterface* other)
{
  if (other)
  {
    this->ConnectedTo = other;
    this->Connection = QObject::connect(&this->TextEdit, &QTextEdit::textChanged,
      [this]()
      {
        if (!this->SettingText)
        {
          this->ConnectedTo->setText(this->getText());
        }
      });
  }
}

//-----------------------------------------------------------------------------
void pqLinkedObjectQTextEdit::unlink()
{
  if (this->ConnectedTo)
  {
    QObject::disconnect(this->Connection);
    this->ConnectedTo = nullptr;
  }
}

//-----------------------------------------------------------------------------
void pqLinkedObjectQTextEdit::setText(const QString& txt)
{
  this->SettingText = true;
  this->TextEdit.setHtml(txt);
  this->SettingText = false;
}

//-----------------------------------------------------------------------------
QString pqLinkedObjectQTextEdit::getText() const
{
  return this->TextEdit.toHtml();
}

//-----------------------------------------------------------------------------
QObject* pqLinkedObjectQTextEdit::getLinked() const noexcept
{
  return &this->TextEdit;
}

//-----------------------------------------------------------------------------
QString pqLinkedObjectQTextEdit::getName() const
{
  return this->TextEdit.objectName();
}
