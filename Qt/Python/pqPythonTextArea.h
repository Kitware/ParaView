/*=========================================================================

   Program: ParaView
   Module:    pqPythonTextArea.h

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

#ifndef pqPythonTextArea_h
#define pqPythonTextArea_h

#include "pqPythonModule.h"
#include "pqPythonUndoCommand.h"

#include <QKeyEvent>
#include <QUndoStack>
#include <QWidget>

class QTextEdit;

class pqPythonLineNumberArea;
class pqPythonSyntaxHighlighter;
class pqPythonUndoStack;
class pqPythonFileIO;

/**
 * @class pqPythonTextArea
 * @brief A python text editor widget
 * @details Displays an editable text area with syntax
 * python highlighting and line numbering.
 */
class PQPYTHON_EXPORT pqPythonTextArea : public QWidget
{
  Q_OBJECT

public:
  /**
   * @brief Construct a pqPythonTextArea
   * @input[parent] the parent widget for the Qt ownership
   */
  explicit pqPythonTextArea(QWidget* parent);

  /**
   * @brief Returns the underlying \ref TextEdit
   */
  QTextEdit* GetTextEdit() { return this->TextEdit; }

  /**
   * @brief Returns a unique action that triggers an
   * undo command on the text.
   */
  QAction* GetUndoAction();

  /**
   * @brief Returns a unique action that triggers an
   * redo command on the text.
   */
  QAction* GetRedoAction();

  /**
   * @brief Returns the new file unique action
   */
  QAction* GetNewFileAction();

  /**
   * @brief Returns the open file unique action
   */
  QAction* GetOpenFileAction();

  /**
   * @brief Returns the save file unique action
   */
  QAction* GetSaveFileAction();

  /**
   * @brief Returns the save as file action.
   * Compared to the SaveFile action, this one
   * always open a dialog asking where to save the
   * copy of the current opened buffer.
   */
  QAction* GetSaveFileAsAction();

  /**
   * @brief Same as \ref GetSaveFileAsAction
   * but saves a copy of the buffer as a ParaView
   * Python macro.
   */
  QAction* GetSaveFileAsMacroAction();

  /**
   * @brief Saves and closes the current opened
   * buffer when a Qt close event occurs.
   *
   * Return false if the buffer has not been saved.
   */
  bool SaveOnClose();

  bool OpenFile(const QString& filename);

  void SetDefaultSaveDirectory(const QString& dir);

signals:
  /**
   * @brief Triggered when the current text
   * buffer is erased
   */
  void BufferErased();

  /**
   * @brief Triggers after a file has been
   * successfuly opened in the current buffer.
   */
  void FileOpened(const QString&);

  /**
   * @brief Triggers after a successful
   * saves of the current buffer.
   */
  void FileSaved(const QString&);

  /**
   * @brief Triggers after a successful
   * copy of the current buffer to a new
   * file.
   */
  void FileSavedAsMacro(const QString&);

protected:
  /**
   * @brief Filter the KeyEvent CTRL+Z
   * @details We need to filter the QTextEdit KeyEvent
   * otherwise our undo/redo actions are not triggered.
   */
  bool eventFilter(QObject* obj, QEvent* event) override;

private:
  /**
   * @brief The editable text area
   */
  QTextEdit* TextEdit = nullptr;

  /**
   * @brief The line number area widget
   */
  pqPythonLineNumberArea* LineNumberArea = nullptr;

  /**
   * @brief The syntax highlighter used to color the \ref TextEdit
   */
  pqPythonSyntaxHighlighter* SyntaxHighlighter = nullptr;

  /**
   * @brief The IO module used to save the \ref TextEdit
   */
  pqPythonFileIO* FileIO = nullptr;

  /**
   * @brief The text QUndoStack
   */
  QUndoStack UndoStack;

  /**
   * @brief The last history entry in the undo stack
   * @details Unfortunatly, QTextEdit doesn't keep
   * a text hystory, which we need for the \ref UndoStack.
   */
  pqPythonTextHistoryEntry lastEntry;

  QAction* UndoAction = nullptr;

  Qt::KeyboardModifier UndoMofidier = Qt::ControlModifier;
  Qt::Key UndoKey = Qt::Key_Z;

  QAction* RedoAction = nullptr;

  Qt::KeyboardModifier RedoMofidier = Qt::ControlModifier;
  Qt::Key RedoKey = Qt::Key_Y;
};

#endif // pqPythonTextArea_h
