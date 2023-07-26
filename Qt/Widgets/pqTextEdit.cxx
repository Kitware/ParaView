// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
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
pqTextEdit::~pqTextEdit() = default;

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
