// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonTextArea.h"
#include "pqPythonFileIO.h"
#include "pqPythonLineNumberArea.h"
#include "pqPythonSyntaxHighlighter.h"

#include <QHBoxLayout>
#include <QTextEdit>

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

  this->connect(this->TextEdit.data(), &QTextEdit::cursorPositionChanged,
    [this]() { this->LineNumberArea->update(); });

  this->connect(this->TextEdit->document(), &QTextDocument::blockCountChanged,
    [this](int) { this->LineNumberArea->updateGeometry(); });

  this->connect(this->TextEdit->verticalScrollBar(), &QScrollBar::valueChanged,
    [this](int) { this->LineNumberArea->update(); });

  this->connect(this->TextEdit.data(), &QTextEdit::textChanged, [this]() {
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
    this->FileIO.data(), &pqPythonFileIO::fileSaved, this, &pqPythonTextArea::fileSaved);
  this->connect(
    this->FileIO.data(), &pqPythonFileIO::fileOpened, this, &pqPythonTextArea::fileOpened);
  this->connect(
    this->FileIO.data(), &pqPythonFileIO::bufferErased, this, &pqPythonTextArea::bufferErased);
  this->connect(
    this->FileIO.data(), &pqPythonFileIO::contentChanged, this, &pqPythonTextArea::contentChanged);

  this->connect(
    this->FileIO.data(), &pqPythonFileIO::bufferErased, [this]() { this->UndoStack.clear(); });
}

//-----------------------------------------------------------------------------
#define TEST_SEQUENCE(EVENT, MODIFIER, KEY)                                                        \
  ((EVENT)->modifiers() & (MODIFIER)) && (EVENT)->key() == (KEY)
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
bool pqPythonTextArea::openFile(const QString& filename, vtkTypeUInt32 location)
{
  return this->FileIO->openFile(filename, location);
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
  pqPythonEditorActions::connect(actions, this->TextEdit.data());
  pqPythonEditorActions::connect(actions, this->FileIO.data());
  pqPythonEditorActions::connect(actions, &this->UndoStack);
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::disconnectActions(pqPythonEditorActions& actions)
{
  pqPythonEditorActions::disconnect(actions, this->TextEdit.data());
  pqPythonEditorActions::disconnect(actions, this->FileIO.data());
  pqPythonEditorActions::disconnect(actions, &this->UndoStack);
}

//-----------------------------------------------------------------------------
void pqPythonTextArea::unlink()
{
  this->TextLinker = pqTextLinker();
}
