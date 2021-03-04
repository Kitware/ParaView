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

#include <QHBoxLayout>

//-----------------------------------------------------------------------------
pqPythonTextArea::pqPythonTextArea(QWidget* parent)
  : QWidget(parent)
  , TextEdit(this)
  , LineNumberArea(this, *this->TextEdit)
  , SyntaxHighlighter(this, *this->TextEdit)
  , FileIO(this, *this->TextEdit)
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

  this->connect(this->TextEdit, &QTextEdit::cursorPositionChanged,
    [this]() { this->LineNumberArea->update(); });

  this->connect(this->TextEdit->document(), &QTextDocument::blockCountChanged,
    [this](int) { this->LineNumberArea->updateGeometry(); });

  this->connect(this->TextEdit->verticalScrollBar(), &QScrollBar::valueChanged,
    [this](int) { this->LineNumberArea->update(); });

  this->connect(this->TextEdit, &QTextEdit::textChanged, [this]() {
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

    this->FileIO->contentChanged();
  });

  this->connect(
    this->FileIO, &pqPythonFileIO::fileSavedAsMacro, this, &pqPythonTextArea::fileSavedAsMacro);
  this->connect(this->FileIO, &pqPythonFileIO::fileSaved, this, &pqPythonTextArea::fileSaved);
  this->connect(this->FileIO, &pqPythonFileIO::fileOpened, this, &pqPythonTextArea::fileOpened);
  this->connect(this->FileIO, &pqPythonFileIO::bufferErased, this, &pqPythonTextArea::bufferErased);
  this->connect(
    this->FileIO, &pqPythonFileIO::contentChanged, this, &pqPythonTextArea::contentChanged);

  this->connect(this->FileIO, &pqPythonFileIO::bufferErased, [this]() { this->UndoStack.clear(); });
}

//-----------------------------------------------------------------------------
#define TEST_SEQUENCE(EVENT, MODIFIER, KEY) (EVENT->modifiers() & MODIFIER) && EVENT->key() == KEY
bool pqPythonTextArea::eventFilter(QObject* obj, QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->matches(QKeySequence::Undo))
    {
      this->UndoStack.undo();
      this->LineNumberArea->update();
      return true;
    }
    else if (keyEvent->matches(QKeySequence::Redo))
    {
      this->UndoStack.redo();
      this->LineNumberArea->update();
      return true;
    }
  }

  return QObject::eventFilter(obj, event);
}
#undef TEST_SEQUENCE

//-----------------------------------------------------------------------------
bool pqPythonTextArea::saveOnClose()
{
  return this->FileIO->saveOnClose();
}

//-----------------------------------------------------------------------------
bool pqPythonTextArea::isEmpty() const
{
  return this->TextEdit->toPlainText().isEmpty();
}

//-----------------------------------------------------------------------------
bool pqPythonTextArea::isDirty() const
{
  return this->FileIO->isDirty();
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::setText(const QString& text)
{
  this->TextEdit->setPlainText(text);
  this->FileIO->setModified(true);
}

//-----------------------------------------------------------------------------
bool pqPythonTextArea::openFile(const QString& filename)
{
  return this->FileIO->openFile(filename);
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::setDefaultSaveDirectory(const QString& dir)
{
  this->FileIO->setDefaultSaveDirectory(dir);
}

//-----------------------------------------------------------------------------
const QString& pqPythonTextArea::getFilename() const
{
  return this->FileIO->getFilename();
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::connectActions(pqPythonEditorActions& actions)
{
  pqPythonEditorActions::connect(actions, this->TextEdit);
  pqPythonEditorActions::connect(actions, this->FileIO);
  pqPythonEditorActions::connect(actions, &this->UndoStack);
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::disconnectActions(pqPythonEditorActions& actions)
{
  pqPythonEditorActions::disconnect(actions, this->TextEdit);
  pqPythonEditorActions::disconnect(actions, this->FileIO);
  pqPythonEditorActions::disconnect(actions, &this->UndoStack);
}
