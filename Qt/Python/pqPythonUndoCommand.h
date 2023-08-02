// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonUndoCommand_h
#define pqPythonUndoCommand_h

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
   * @brief The absolute cursor position within pqPythonUndoCommand::Text
   */
  std::int32_t cursorPosition = -1;

  /**
   * @brief Returns true is the entry is empty
   */
  bool isEmpty() const noexcept { return cursorPosition == -1; }
};

/**
 * @class pqPythonUndoCommand
 * @brief The python text editor undo/redo command
 * @details The \ref pqPythonUndoCommand models the undo/redo framework needed for QUndoStack.
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

#endif // pqPythonUndoCommand_h
