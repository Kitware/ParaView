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

#include <QTextEdit>

class pqPythonLineNumberArea;
class pqPythonSyntaxHighlighter;

/// @class pqPythonTextArea
/// @brief A python text editor widget
/// @details Displays an editable text area with syntax
/// python highlighting and line numbering.
class PQPYTHON_EXPORT pqPythonTextArea : public QWidget
{
  Q_OBJECT

public:
  /// @brief Construct a pqPythonTextArea
  /// @input[parent] the parent widget for the Qt ownership
  explicit pqPythonTextArea(QWidget* parent);

  /// @brief Returns the underlying \ref TextEdit
  QTextEdit* GetTextEdit() { return this->TextEdit; }

private:
  /// @brief The editable text area
  QTextEdit* TextEdit;

  /// @brief The line number area widget
  pqPythonLineNumberArea* LineNumberArea;

  /// @brief The syntax highlighter used to color the \ref TextEdit
  pqPythonSyntaxHighlighter* SyntaxHighlighter;
};

#endif // pqPythonTextArea_h
