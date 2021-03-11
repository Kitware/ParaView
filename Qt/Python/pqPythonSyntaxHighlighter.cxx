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

namespace globals
{
constexpr const char* kFourSpaces = "    ";
}

//-----------------------------------------------------------------------------
pqPythonSyntaxHighlighter::pqPythonSyntaxHighlighter(QObject* p, QTextEdit& textEdit)
  : Superclass(p)
  , TextEdit(textEdit)
{
  this->TextEdit.installEventFilter(this);
  vtkPythonInterpreter::Initialize();

  {
    vtkPythonScopeGilEnsurer gilEnsurer;

    // PyErr_Fetch() -- PyErr_Restore() helps us catch import related exceptions
    // thus avoiding printing any messages to the terminal if the `pygments`
    // import fails. `pygments` is totally optional for ParaView.
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    this->PygmentsModule.TakeReference(PyImport_ImportModule("pygments"));
    PyErr_Restore(type, value, traceback);

    if (this->PygmentsModule)
    {
      this->HighlightFunction.TakeReference(
        PyObject_GetAttrString(this->PygmentsModule, "highlight"));
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
      this->PythonLexer.TakeReference(PyObject_Call(pythonLexerClass, emptyTuple, nullptr));
      this->HtmlFormatter.TakeReference(PyObject_Call(htmlFormatterClass, emptyTuple, nullptr));
      PyObject_SetAttrString(this->HtmlFormatter, "noclasses", Py_True);
      PyObject_SetAttrString(this->HtmlFormatter, "nobackground", Py_True);
    }
    else
    {
      static bool warnOnce = true;
      if (warnOnce)
      {
        qInfo("Python package 'pygments' is missing. Please install the package for syntax "
              "highlighting.");
        warnOnce = false;
      }
    }
  }

  // Use a fixed-width font for text edits displaying code
  QFont font("Monospace");
  // cause Qt to select an fixed-width font if Monospace isn't found
  font.setStyleHint(QFont::TypeWriter);
  this->TextEdit.setFont(font);
  // Set tab width equal to 4 spaces
  QFontMetrics metrics = this->TextEdit.fontMetrics();
  this->TextEdit.setTabStopDistance(metrics.horizontalAdvance(globals::kFourSpaces));
}

//-----------------------------------------------------------------------------
bool pqPythonSyntaxHighlighter::isReplacingTabs() const
{
  return this->ReplaceTabs;
}

//-----------------------------------------------------------------------------
void pqPythonSyntaxHighlighter::setReplaceTabs(bool replaceTabs)
{
  this->ReplaceTabs = replaceTabs;
}

//-----------------------------------------------------------------------------
bool pqPythonSyntaxHighlighter::eventFilter(QObject* obj, QEvent* ev)
{
  if (!this->ReplaceTabs)
  {
    return false;
  }
  if (ev->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
    if (keyEvent->modifiers() == Qt::NoModifier && keyEvent->key() == Qt::Key_Tab)
    {
      this->TextEdit.textCursor().insertText(globals::kFourSpaces);
      return true;
    }
  }

  return QObject::eventFilter(obj, ev);
}

//-----------------------------------------------------------------------------
QString pqPythonSyntaxHighlighter::Highlight(const QString& text) const
{
  int leadingWhiteSpaceLength = 0;
  int trailingWhiteSpaceLength = 0;
  while (leadingWhiteSpaceLength < text.length() && text.at(leadingWhiteSpaceLength).isSpace())
  {
    ++leadingWhiteSpaceLength;
  }

  QString result;
  if (this->PygmentsModule && leadingWhiteSpaceLength < text.length())
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
    vtkSmartPyObject unicode(PyUnicode_DecodeUTF8(bytes.data(), bytes.size(), nullptr));
    vtkSmartPyObject args(Py_BuildValue("OOO", unicode.GetPointer(), this->PythonLexer.GetPointer(),
      this->HtmlFormatter.GetPointer()));
    vtkSmartPyObject resultingText(PyObject_Call(this->HighlightFunction, args, nullptr));

#if PY_VERSION_HEX >= 0x03070000
    char* resultingTextAsCString = const_cast<char*>(PyUnicode_AsUTF8(resultingText));
#else
    const char* resultingTextAsCString = PyUnicode_AsUTF8(resultingText);
#endif

    const QString pygmentsOutput = QString::fromUtf8(resultingTextAsCString);

    // the first span tag always should follow the pre tag like this; <pre ...><span
    const int startOfPre = pygmentsOutput.indexOf(">", pygmentsOutput.indexOf("<pre")) + 1;
    const int endOfPre = pygmentsOutput.lastIndexOf("</pre>");
    const QString startOfHtml = pygmentsOutput.left(startOfPre);
    const QString formatted = pygmentsOutput.mid(startOfPre, endOfPre - startOfPre).trimmed();
    const QString endOfHtml = pygmentsOutput.right(pygmentsOutput.length() - endOfPre).trimmed();

    // The <pre ...> tag will ignore a newline immediately following the >,
    // so insert one to be ignored.  Similarly, the </pre> will ignore a
    // newline immediately preceding it, so put one in.  These quirks allow
    // inserting newlines at the beginning and end of the text and took a
    // long time to figure out.
    result = QString("%1\n%2%3%4\n%5")
               .arg(startOfHtml, leadingWhitespace, formatted, trailingWhitespace, endOfHtml);
  }

  return result;
}

//-----------------------------------------------------------------------------
void pqPythonSyntaxHighlighter::ConnectHighligter() const
{
  connect(&this->TextEdit, &QTextEdit::textChanged, [this]() {
    const QString text = this->TextEdit.toPlainText();
    const int cursorPosition = this->TextEdit.textCursor().position();

    const QString highlightedText = this->Highlight(text);
    if (!highlightedText.isEmpty())
    {
      const bool oldState = this->TextEdit.blockSignals(true);

      this->TextEdit.setHtml(this->Highlight(text));
      QTextCursor cursor = this->TextEdit.textCursor();
      cursor.setPosition(cursorPosition);
      this->TextEdit.setTextCursor(cursor);

      const int vScrollBarValue = this->TextEdit.verticalScrollBar()->value();
      this->TextEdit.verticalScrollBar()->setValue(vScrollBarValue);

      const int hScrollBarValue = this->TextEdit.horizontalScrollBar()->value();
      this->TextEdit.horizontalScrollBar()->setValue(hScrollBarValue);

      this->TextEdit.blockSignals(oldState);
    }
  });
}
