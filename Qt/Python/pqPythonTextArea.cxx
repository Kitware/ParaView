/*=========================================================================

   Program: ParaView
   Module:    pqPythonTextArea.cxx

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

#include "pqPythonTextArea.h"

#include "pqPythonFileIO.h"
#include "pqPythonLineNumberArea.h"
#include "pqPythonSyntaxHighlighter.h"
#include "pqPythonUtils.h"

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QScrollBar>
#include <QTextBlock>

//-----------------------------------------------------------------------------
pqPythonTextArea::pqPythonTextArea(QWidget* parent)
  : QWidget(parent)
  , TextEdit(new QTextEdit(this))
  , LineNumberArea(new pqPythonLineNumberArea(this, *this->TextEdit))
  , SyntaxHighlighter(new pqPythonSyntaxHighlighter(this, *this->TextEdit))
  , FileIO(new pqPythonFileIO(this, *this->TextEdit))
{
  this->TextEdit->setUndoRedoEnabled(false);
  this->TextEdit->setAcceptDrops(false);
  this->TextEdit->setAcceptRichText(false);
  this->TextEdit->setContextMenuPolicy(Qt::NoContextMenu);

  // Install the Undo/Redo event filter
  this->TextEdit->installEventFilter(this);

  // Prepare the HBoxLayout for the text widget and the line area widget
  QHBoxLayout* layout = new QHBoxLayout();
  layout->addWidget(this->LineNumberArea);
  layout->addWidget(this->TextEdit);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  this->setLayout(layout);

  connect(this->TextEdit, &QTextEdit::cursorPositionChanged,
    [this]() { this->LineNumberArea->update(); });

  connect(this->TextEdit->document(), &QTextDocument::blockCountChanged,
    [this](int) { this->LineNumberArea->updateGeometry(); });

  connect(this->TextEdit->verticalScrollBar(), &QScrollBar::valueChanged,
    [this](int) { this->LineNumberArea->update(); });

  connect(this->TextEdit, &QTextEdit::textChanged, [this]() {
    const QString text = this->TextEdit->toPlainText();
    const int cursorPosition = this->TextEdit->textCursor().position();

    // Push a new command to the stack (which calls redo, thus applying the command)
    this->UndoStack.push(new pqPythonUndoCommand(
      *this->TextEdit, this->SyntaxHighlighter, this->lastEntry, { text, cursorPosition }));

    // Save the last entry
    this->lastEntry = { text, cursorPosition };

    const int vScrollBarValue = this->TextEdit->verticalScrollBar()->value();
    this->TextEdit->verticalScrollBar()->setValue(vScrollBarValue);

    const int hScrollBarValue = this->TextEdit->horizontalScrollBar()->value();
    this->TextEdit->horizontalScrollBar()->setValue(hScrollBarValue);
  });

  connect(this->FileIO, SIGNAL(FileSavedAsMacro(const QString&)), this,
    SIGNAL(FileSavedAsMacro(const QString&)));
  connect(this->FileIO, SIGNAL(FileSaved(const QString&)), this, SIGNAL(FileSaved(const QString&)));
  connect(
    this->FileIO, SIGNAL(FileOpened(const QString&)), this, SIGNAL(FileOpened(const QString&)));
  connect(this->FileIO, SIGNAL(BufferErased()), this, SIGNAL(BufferErased()));

  connect(this->FileIO, &pqPythonFileIO::BufferErased, [this]() { this->UndoStack.clear(); });
}

//-----------------------------------------------------------------------------
#define TEST_SEQUENCE(EVENT, MODIFIER, KEY) (EVENT->modifiers() & MODIFIER) && EVENT->key() == KEY
bool pqPythonTextArea::eventFilter(QObject* obj, QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (TEST_SEQUENCE(keyEvent, this->UndoMofidier, this->UndoKey))
    {
      this->UndoAction->trigger();
      return true;
    }
    else if (TEST_SEQUENCE(keyEvent, this->RedoMofidier, this->RedoKey))
    {
      this->RedoAction->trigger();
      return true;
    }
  }

  return QObject::eventFilter(obj, event);
}
#undef TEST_SEQUENCE

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetUndoAction()
{
  if (UndoAction == nullptr)
  {
    this->UndoAction = this->UndoStack.createUndoAction(this, tr("&Undo"));

    // Update the line number area if undo triggers
    connect(this->UndoAction, &QAction::triggered, [this]() { this->LineNumberArea->update(); });
  }

  return UndoAction;
}

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetRedoAction()
{
  if (RedoAction == nullptr)
  {
    this->RedoAction = this->UndoStack.createRedoAction(this, tr("&Redo"));

    // Update the line number area if redo triggers
    connect(this->RedoAction, &QAction::triggered, [this]() { this->LineNumberArea->update(); });
  }

  return RedoAction;
}

//-----------------------------------------------------------------------------
bool pqPythonTextArea::SaveOnClose()
{
  if (this->FileIO->SaveOnClose())
  {
    this->TextEdit->clear();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPythonTextArea::OpenFile(const QString& filename)
{
  return this->FileIO->OpenFile(filename);
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::SetDefaultSaveDirectory(const QString& dir)
{
  this->FileIO->SetDefaultSaveDirectory(dir);
}

using Action = pqPythonFileIO::IOAction;

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetNewFileAction()
{
  return this->FileIO->GetAction(Action::NewFile);
}

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetOpenFileAction()
{
  return this->FileIO->GetAction(Action::OpenFile);
}

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetSaveFileAction()
{
  return this->FileIO->GetAction(Action::SaveFile);
}

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetSaveFileAsAction()
{
  return this->FileIO->GetAction(Action::SaveFileAs);
}

//-----------------------------------------------------------------------------
QAction* pqPythonTextArea::GetSaveFileAsMacroAction()
{
  return this->FileIO->GetAction(Action::SaveFileAsMacro);
}
