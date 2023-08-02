// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonSyntaxHighlighter_h
#define pqPythonSyntaxHighlighter_h

#undef slots
#include "vtkPython.h"
#define slots Q_SLOTS

#include "pqPythonModule.h"

#include <QObject>

#include "vtkPythonCompatibility.h"
#include "vtkPythonInterpreter.h"
#include "vtkSmartPyObject.h"

class QTextEdit;
class QUndoStack;

struct pqPythonTextHistoryEntry;

/**
 * This class is a helper object to attach to a QTextEdit to add Python
 * syntax highlighting to the text that is displayed. The pygments python
 * module is used to generate syntax highlighting. Since mixing tabs and
 * spaces is an error for python's indentation, tabs are highlighted in red.
 *
 * The QTextEdit is set up so that it uses a fixed-width font and tabs are
 * the width of 4 spaces by the constructor.
 *
 * This will also optionally
 * capture presses of the tab key that would go to the QTextEdit and
 * insert 4 spaces instead of the tab. This option is enabled by default.
 */
class PQPYTHON_EXPORT pqPythonSyntaxHighlighter : public QObject
{
  Q_OBJECT
public:
  typedef QObject Superclass;
  /**
   * Creates a pqPythonSyntaxHighlighter on the given QTextEdit.
   * The highlighter is not directly connected to the QTextEdit object (see \ref ConnectHighligter).
   * NOTE: the optional tab key capture is enabled by the constructor
   */
  explicit pqPythonSyntaxHighlighter(QObject* p, QTextEdit& textEdit);
  ~pqPythonSyntaxHighlighter() override = default;

  /**
   * Returns true if the tab key is being intercepted to insert spaces in
   * the text edit
   */
  bool isReplacingTabs() const;
  /**
   * Used to enable/disable tab key capture
   * Passing true will cause the tab key to insert 4 spaces in the QTextEdit
   */
  void setReplaceTabs(bool replaceTabs);

  /**
   * Returns the highlighted input text
   */
  QString Highlight(const QString& src) const;

  /**
   * Connects the highlighter to text.
   * This is not automatically called by the constructor
   * as we might want to defer the connection to another widget.
   */
  void ConnectHighligter() const;

protected:
  /**
   * This event filter is applied to the TextEdit to translate presses of the
   * Tab key into 4 spaces being inserted
   */
  bool eventFilter(QObject*, QEvent*) override;

private:
  QTextEdit& TextEdit;

  vtkSmartPyObject PygmentsModule;

  vtkSmartPyObject HighlightFunction;

  vtkSmartPyObject PythonLexer;

  vtkSmartPyObject HtmlFormatter;

  /**
   * If true, replaces the tabs with 4 spaces
   * whenever there is a new input inside the \ref TextEdit
   */
  bool ReplaceTabs = true;

  Q_DISABLE_COPY(pqPythonSyntaxHighlighter);
};

#endif
