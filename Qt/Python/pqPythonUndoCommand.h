/*=========================================================================

   Program: ParaView
   Module:    pqPythonUndoStack.h

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
#ifndef pqPythonUndoStack_h
#define pqPythonUndoStack_h

#include "pqPythonModule.h"

#include <QTextEdit>
#include <QUndoCommand>

#include <functional>
#include <iostream>
#include <stack>
#include <vector>

class pqPythonSyntaxHighlighter;

/**
 * @struct PythonTextHistory
 * @brief Data structure that contains information to perform the undo
 * and redo operations.
 *
 * @details This structure is part of the public headers as other
 * might need to instantiate and pass it to the \ref pqPythonUndoCommand.
 */
struct pqPythonTextHistoryEntry
{
  pqPythonTextHistoryEntry() = default;

  pqPythonTextHistoryEntry(const QString& txt, const std::int32_t cursorPos)
    : content(txt)
    , cursorPosition(cursorPos)
  {
  }

  /**
   * @brief The raw text
   */
  QString content = "";

  /**
   * @brief The absolute cursor position within \ref pqPythonUndoCommand::Text
   */
  std::int32_t cursorPosition = -1;

  /**
   * @brief Returns true is the entry is empty
   */
  bool isEmpty() const { return cursorPosition == -1; }
};

/**
 * @class pqPythonUndoCommand
 * @brief The python text editor undo/redo command
 * @detail The \ref pqPythonUndoCommand models the undo/redo framework needed for QUndoStack.
 * The actual storage of the \ref PythonTextHistory is external to the command: this avoids
 * duplicating entries.
 */
class PQPYTHON_EXPORT pqPythonUndoCommand : public QUndoCommand
{
public:
  /**
   * @brief Construct an undo command
   */
  pqPythonUndoCommand(QTextEdit& text, pqPythonSyntaxHighlighter* highlighter,
    pqPythonTextHistoryEntry& lastHistoryEntry,
    const pqPythonTextHistoryEntry&& currentHistoryEntry);

  /**
   * @brief Overriden function that performs the undo
   */
  void undo() override;

  /**
   * @brief Overriden function that performs the redo
   */
  void redo() override;

  /**
   * @brief Returns the current history entry
   */
  const pqPythonTextHistoryEntry& getCurrentHistoryEntry() const
  {
    return this->CurrentHistoryEntry;
  }

private:
  /**
   * @brief Utility function that mutates the \ref text
   * using the given history value \ref h.
   */
  void swapImpl(const pqPythonTextHistoryEntry& h);

  /// @brief The text to do the undo/redo on
  QTextEdit& Text;

  pqPythonSyntaxHighlighter* Highlighter;

  /**
   * @brief The last history entry in the text
   * @details This is a direct reference to the history entry
   * contained in the text. This is being modified when performing
   * the undo function.
   *
   * If we don't modify the current history entry in the next
   * when performing an undo, the overall undo stack is not
   * what we expect it to be. Let's take the following commands:
   * [enter 1 ... enter 2 ... undo ... enter 3 ... undo]
   * Without modifying the \ref TextLastHystoryEntry we would get
   * 1 -> 12 -> 1 -> 13 -> 12, which is wrong.
   * This problem arises from the fact that the QTextEdit doesn't keep
   * its last state in memory.
   */
  pqPythonTextHistoryEntry& TextLastHystoryEntry;

  const pqPythonTextHistoryEntry LastHistoryEntry;

  const pqPythonTextHistoryEntry CurrentHistoryEntry;
};

#endif // pqPythonUndoStack_h
