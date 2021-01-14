/*=========================================================================

   Program: ParaView
   Module:    pqPythonSyntaxHighlighter.h

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
#ifndef _pqPythonSyntaxHighlighter_h
#define _pqPythonSyntaxHighlighter_h

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
  ~pqPythonSyntaxHighlighter() override {}

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
   * Connects the highlighter to \ref text.
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
