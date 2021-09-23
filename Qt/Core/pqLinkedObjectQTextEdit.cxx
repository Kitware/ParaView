/*=========================================================================

   Program: ParaView
   Module:    pqLinkedObjectQTextEdit.cxx

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

#include "pqLinkedObjectQTextEdit.h"

#include <QTextEdit>

//-----------------------------------------------------------------------------
void pqLinkedObjectQTextEdit::link(pqLinkedObjectInterface* other)
{
  if (other)
  {
    this->ConnectedTo = other;
    this->Connection = QObject::connect(&this->TextEdit, &QTextEdit::textChanged, [this]() {
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
