/*=========================================================================

   Program: ParaView
   Module:    pqPythonScriptEditor.cxx

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
#include <vtkPython.h> // must always be included first

#include "pqPythonSyntaxHighlighter.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QScrollBar>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>

#include <vtkPythonCompatibility.h>
#include <vtkPythonInterpreter.h>
#include <vtkSmartPyObject.h>

class pqPythonSyntaxHighlighter::pqInternal
{
public:
  QPointer<QTextEdit> TextEdit;
  vtkSmartPyObject PygmentsModule;
  vtkSmartPyObject HighlightFunction;
  vtkSmartPyObject PythonLexer;
  vtkSmartPyObject HtmlFormatter;
  bool IsSyntaxHighlighting;
  bool ReplaceTabs;
};

pqPythonSyntaxHighlighter::pqPythonSyntaxHighlighter(QTextEdit* textEdit, QObject* p)
  : Superclass(p)
  , Internals(new pqPythonSyntaxHighlighter::pqInternal())
{
  this->Internals->TextEdit = textEdit;
  this->Internals->TextEdit->installEventFilter(this);
  vtkPythonInterpreter::Initialize();

  {
    vtkPythonScopeGilEnsurer gilEnsurer;

    // PyErr_Fetch() -- PyErr_Restore() helps us catch import related exceptions
    // thus avoiding printing any messages to the terminal if the `pygments`
    // import fails. `pygments` is totally optional for ParaView.
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    this->Internals->PygmentsModule.TakeReference(PyImport_ImportModule("pygments"));
    PyErr_Restore(type, value, traceback);

    if (this->Internals->PygmentsModule && this->Internals->TextEdit != NULL)
    {
      this->Internals->HighlightFunction.TakeReference(
        PyObject_GetAttrString(this->Internals->PygmentsModule, "highlight"));
      vtkSmartPyObject lexersModule(PyImport_ImportModule("pygments.lexers"));
      vtkSmartPyObject formattersModule(PyImport_ImportModule("pygments.formatters.redtabhtml"));
      vtkSmartPyObject htmlFormatterClass;
      // If we have the custom formatter written for ParaView, great.
      // otherwise just default to the HtmlFormatter in pygments
      if (formattersModule)
      {
        htmlFormatterClass.TakeReference(
          PyObject_GetAttrString(formattersModule, "RedTabHtmlFormatter"));
      }
      else
      {
        formattersModule.TakeReference(PyImport_ImportModule("pygments.formatters"));
        htmlFormatterClass.TakeReference(PyObject_GetAttrString(formattersModule, "HtmlFormatter"));
      }

#ifdef VTK_PY3K
      vtkSmartPyObject pythonLexerClass(PyObject_GetAttrString(lexersModule, "Python3Lexer"));
#else
      vtkSmartPyObject pythonLexerClass(PyObject_GetAttrString(lexersModule, "PythonLexer"));
#endif
      vtkSmartPyObject emptyTuple(Py_BuildValue("()"));
      this->Internals->PythonLexer.TakeReference(PyObject_Call(pythonLexerClass, emptyTuple, NULL));
      this->Internals->HtmlFormatter.TakeReference(
        PyObject_Call(htmlFormatterClass, emptyTuple, NULL));
      PyObject_SetAttrString(this->Internals->HtmlFormatter, "noclasses", Py_True);
      PyObject_SetAttrString(this->Internals->HtmlFormatter, "nobackground", Py_True);
      this->connect(
        this->Internals->TextEdit.data(), SIGNAL(textChanged()), this, SLOT(rehighlightSyntax()));
    }
  }

  this->Internals->IsSyntaxHighlighting = false;
  // Replace tabs with 4 spaces
  this->Internals->ReplaceTabs = true;
  // Use a fixed-width font for text edits displaying code
  QFont font("Monospace");
  // cause Qt to select an fixed-width font if Monospace isn't found
  font.setStyleHint(QFont::TypeWriter);
  this->Internals->TextEdit->setFont(font);
  // Set tab width equal to 4 spaces
  QFontMetrics metrics = this->Internals->TextEdit->fontMetrics();
  this->Internals->TextEdit->setTabStopDistance(metrics.horizontalAdvance("    "));
  this->rehighlightSyntax();
}

pqPythonSyntaxHighlighter::~pqPythonSyntaxHighlighter()
{
}

bool pqPythonSyntaxHighlighter::isReplacingTabs() const
{
  return this->Internals->ReplaceTabs;
}

void pqPythonSyntaxHighlighter::setReplaceTabs(bool replaceTabs)
{
  this->Internals->ReplaceTabs = replaceTabs;
}

bool pqPythonSyntaxHighlighter::eventFilter(QObject*, QEvent* ev)
{
  if (!this->Internals->ReplaceTabs)
  {
    return false;
  }
  if (ev->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
    if (keyEvent->key() == Qt::Key_Tab)
    {
      if (!this->Internals->TextEdit.isNull())
      {
        this->Internals->TextEdit->textCursor().insertText("    ");
      }
      return true;
    }
  }
  return false;
}

void pqPythonSyntaxHighlighter::rehighlightSyntax()
{
  if (this->Internals->IsSyntaxHighlighting || !this->Internals->PygmentsModule ||
    this->Internals->TextEdit.isNull())
  {
    return;
  }
  this->Internals->IsSyntaxHighlighting = true;
  QString text = this->Internals->TextEdit->toPlainText();
  int cursorPosition = this->Internals->TextEdit->textCursor().position();
  int vScrollBarValue = this->Internals->TextEdit->verticalScrollBar()->value();
  int hScrollBarValue = this->Internals->TextEdit->horizontalScrollBar()->value();
  int leadingWhiteSpaceLength = 0;
  int trailingWhiteSpaceLength = 0;
  while (leadingWhiteSpaceLength < text.length() && text.at(leadingWhiteSpaceLength).isSpace())
  {
    ++leadingWhiteSpaceLength;
  }
  if (leadingWhiteSpaceLength < text.length())
  {
    while (trailingWhiteSpaceLength < text.length() &&
      text.at(text.length() - 1 - trailingWhiteSpaceLength).isSpace())
    {
      ++trailingWhiteSpaceLength;
    }
    QString leadingWhitespace = text.left(leadingWhiteSpaceLength);
    QString trailingWhitespace = text.right(trailingWhiteSpaceLength);

    QByteArray bytes = text.trimmed().toUtf8();

    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject unicode(PyUnicode_DecodeUTF8(bytes.data(), bytes.size(), NULL));
    vtkSmartPyObject args(Py_BuildValue("OOO", unicode.GetPointer(),
      this->Internals->PythonLexer.GetPointer(), this->Internals->HtmlFormatter.GetPointer()));
    vtkSmartPyObject resultingText(PyObject_Call(this->Internals->HighlightFunction, args, NULL));

#if PY_MAJOR_VERSION == 2
    vtkSmartPyObject resultingTextBytes(PyUnicode_AsUTF8String(resultingText));
    char* resultingTextAsCString = PyString_AsString(resultingTextBytes);
#elif PY_VERSION_HEX >= 0x03070000
    char* resultingTextAsCString = const_cast<char*>(PyUnicode_AsUTF8(resultingText));
#else
    const char* resultingTextAsCString = PyUnicode_AsUTF8(resultingText);
#endif

    QString pygmentsOutput = QString::fromUtf8(resultingTextAsCString);

    // the first span tag always should follow the pre tag like this; <pre ...><span
    int startOfPre = pygmentsOutput.indexOf(">", pygmentsOutput.indexOf("<pre")) + 1;
    int endOfPre = pygmentsOutput.lastIndexOf("</pre>");
    QString startOfHtml = pygmentsOutput.left(startOfPre);
    QString formatted = pygmentsOutput.mid(startOfPre, endOfPre - startOfPre).trimmed();
    QString endOfHtml = pygmentsOutput.right(pygmentsOutput.length() - endOfPre).trimmed();
    // The <pre ...> tag will ignore a newline immediately following the >,
    // so insert one to be ignored.  Similarly, the </pre> will ignore a
    // newline immediately preceding it, so put one in.  These quirks allow
    // inserting newlines at the beginning and end of the text and took a
    // long time to figure out.
    QString result =
      QString("%1\n%2%3%4\n%5")
        .arg(startOfHtml, leadingWhitespace, formatted, trailingWhitespace, endOfHtml);
    this->Internals->TextEdit->setHtml(result);
    QTextCursor cursor = this->Internals->TextEdit->textCursor();
    cursor.setPosition(cursorPosition);
    this->Internals->TextEdit->setTextCursor(cursor);
    this->Internals->TextEdit->verticalScrollBar()->setValue(vScrollBarValue);
    this->Internals->TextEdit->horizontalScrollBar()->setValue(hScrollBarValue);
  }
  this->Internals->IsSyntaxHighlighting = false;
}
