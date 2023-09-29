// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTextEdit.h"

#include "pqWidgetCompleter.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QKeyEvent>
#include <QRect>
#include <QScrollBar>

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
void pqTextEdit::setCompleter(pqWidgetCompleter* completer)
{
  this->Completer = completer;
  if (!this->Completer)
  {
    return;
  }

  this->Completer->setWidget(this);
  QObject::connect(this->Completer, QOverload<const QString&>::of(&QCompleter::activated), this,
    &pqTextEdit::insertCompletion);
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

  if (this->Completer != nullptr && this->Completer->popup()->isVisible())
  {
    {
      // The following keys are forwarded by the completer to the widget
      switch (e->key())
      {
        case Qt::Key_Tab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Backtab:
          e->ignore();
          return; // let the completer do default behavior
        default:
          break;
      }
    }
  }

  switch (e->key())
  {
    case Qt::Key_Tab: // Display completer or accept
      e->accept();
      if (!this->Completer->getCompleteEmptyPrompts() &&
        this->textUnderCursor().trimmed().isEmpty())
      {
        this->Superclass::keyPressEvent(
          e); // Input a tab and don't complete when the line is empty.
      }
      else
      {
        this->updateCompleter();
        this->selectCompletion();
      }
      break;
    default:
      e->accept();
      this->Superclass::keyPressEvent(e);
      this->updateCompleterIfVisible();
      break;
  }
}

//-----------------------------------------------------------------------------
QString pqTextEdit::textUnderCursor() const
{
  QTextCursor tc = this->textCursor();
  tc.select(QTextCursor::LineUnderCursor);
  return tc.selectedText();
}

//-----------------------------------------------------------------------------
void pqTextEdit::insertCompletion(const QString& completion)
{
  if (this->Completer->widget() != this)
  {
    return;
  }

  QTextCursor tc = this->textCursor();
  tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);

  if (tc.selectedText() == ".")
  {
    tc.insertText(QString(".") + completion);
  }
  else
  {
    tc = this->textCursor();
    tc.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    tc.insertText(completion);
    this->setTextCursor(tc);
  }
};

//-----------------------------------------------------------------------------
void pqTextEdit::updateCompleter()
{
  if (!this->Completer)
  {
    return;
  }

  QString completionPrefix = this->textUnderCursor();

  // Call the completer to update the completion model
  this->Completer->updateCompletionModel(completionPrefix);

  // Place and show the completer if there are available completions
  if (this->Completer->completionCount())
  {
    // Get a QRect for the cursor at the start of the
    // current word and then translate it down 8 pixels.
    QRect cr = this->cursorRect();
    cr.translate(0, 8);
    cr.setWidth(this->Completer->popup()->sizeHintForColumn(0) +
      this->Completer->popup()->verticalScrollBar()->sizeHint().width());
    this->Completer->complete(cr);
  }
  else
  {
    this->Completer->popup()->hide();
  }
};

//-----------------------------------------------------------------------------
void pqTextEdit::updateCompleterIfVisible()
{
  if (this->Completer && this->Completer->popup()->isVisible())
  {
    this->updateCompleter();
  }
};

//-----------------------------------------------------------------------------
void pqTextEdit::selectCompletion()
{
  if (this->Completer && this->Completer->completionCount() == 1)
  {
    this->insertCompletion(this->Completer->currentCompletion());
    this->Completer->popup()->hide();
  }
};

//-----------------------------------------------------------------------------
void pqTextEdit::focusOutEvent(QFocusEvent* e)
{
  this->Superclass::focusOutEvent(e);
  Q_EMIT this->editingFinished();
}

//-----------------------------------------------------------------------------
void pqTextEdit::focusInEvent(QFocusEvent* e)
{
  if (this->Completer)
  {
    this->Completer->setWidget(this);
  }
  this->Superclass::focusInEvent(e);
};
