/*=========================================================================

   Program: ParaView
   Module:    pqTextEdit.cxx

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
#include "pqTextEdit.h"

// Server Manager Includes.

// Qt Includes.
#include <QDebug>
#include <QKeyEvent>

// ParaView Includes.

//-----------------------------------------------------------------------------
class pqTextEditPrivate
{
  Q_DECLARE_PUBLIC(pqTextEdit);

protected:
  pqTextEdit* const q_ptr;

public:
  pqTextEditPrivate(pqTextEdit& object);

  void init();

  bool TextChangedAndEditingFinishedPending;
  QString OldText;
  QList<int> StopFocusKeys;
  int StopFocusModifiers;

private:
  pqTextEditPrivate& operator=(const pqTextEditPrivate&);
};

// --------------------------------------------------------------------------
pqTextEditPrivate::pqTextEditPrivate(pqTextEdit& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void pqTextEditPrivate::init()
{
  Q_Q(pqTextEdit);

  this->TextChangedAndEditingFinishedPending = false;

  q->connect(q, SIGNAL(editingFinished()), q, SLOT(onEditingFinished()));
  q->connect(q, SIGNAL(textChanged()), q, SLOT(onTextEdited()));

  this->StopFocusKeys << Qt::Key_Enter << Qt::Key_Return;
  this->StopFocusModifiers = Qt::ControlModifier | Qt::AltModifier;
}

//-----------------------------------------------------------------------------
pqTextEdit::pqTextEdit(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new pqTextEditPrivate(*this))
{
  Q_D(pqTextEdit);
  d->init();
}

//-----------------------------------------------------------------------------
pqTextEdit::pqTextEdit(const QString& _contents, QWidget* _parent)
  : Superclass(_contents, _parent)
  , d_ptr(new pqTextEditPrivate(*this))
{
  Q_D(pqTextEdit);
  d->init();
}

//-----------------------------------------------------------------------------
pqTextEdit::~pqTextEdit()
{
}

//-----------------------------------------------------------------------------
void pqTextEdit::onTextEdited()
{
  Q_D(pqTextEdit);
  d->TextChangedAndEditingFinishedPending = true;
}

//-----------------------------------------------------------------------------
void pqTextEdit::onEditingFinished()
{
  Q_D(pqTextEdit);
  if (d->TextChangedAndEditingFinishedPending)
  {
    Q_EMIT this->textChangedAndEditingFinished();
    d->TextChangedAndEditingFinishedPending = false;
  }
}

//-----------------------------------------------------------------------------
void pqTextEdit::keyPressEvent(QKeyEvent* e)
{
  Q_D(pqTextEdit);

  if (e)
  {
    d->TextChangedAndEditingFinishedPending = true;

    if (d->StopFocusKeys.contains(e->key()) && e->modifiers() & d->StopFocusModifiers)
    {
      this->clearFocus();
    }
  }

  this->Superclass::keyPressEvent(e);
}

//-----------------------------------------------------------------------------
void pqTextEdit::focusOutEvent(QFocusEvent* e)
{
  this->Superclass::focusOutEvent(e);
  Q_EMIT this->editingFinished();
}
