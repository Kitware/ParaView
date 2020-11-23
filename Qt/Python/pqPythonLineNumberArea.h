/*=========================================================================

   Program: ParaView
   Module:    pqPythonLineNumberArea.h

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

#ifndef pqPythonLineNumberArea_h
#define pqPythonLineNumberArea_h

#include "pqPythonModule.h"

#include <QWidget>

class QTextEdit;

/**
 * @class pqPythonLineNumberArea
 * @brief QWidget that displays line number for a QTextEdit.
 * @details This widget is to be associated with a QTextEdit
 * widget (stored as \ref text) this class. It displays the number
 * of lines inside the \ref text and highlight the line where the
 * cursor is.
 *
 * This must be used inside a QHLayout and placed either left
 * or right of the refered \ref text. The paintEvent is in charge
 * of painting this widget.
 */
class PQPYTHON_EXPORT pqPythonLineNumberArea : public QWidget
{
  Q_OBJECT

public:
  /* @brief Constructs a pqPythonLineNumberArea given a \ref text
   * @input[parent] the parent widget for the Qt ownership
   * @input[text] the text to display the line from
   */
  explicit pqPythonLineNumberArea(QWidget* parent, const QTextEdit& text)
    : QWidget(parent)
    , TextEdit(text)
  {
  }

  /**
   * @brief Return the size hint based on the number of lines present in the \ref text
   */
  QSize sizeHint() const override;

protected:
  /** @brief Paint the widget
   * @details This method paints the widget area by going through the
   * visible block inside \ref text. Are displayed the line number from
   * the \ref text in one color and the line containing the cursor
   * in another color.
   * @input[event] the pain event
   */
  void paintEvent(QPaintEvent* event) override;

private:
  /**
   * @brief The text to display the number of line on
   */
  const QTextEdit& TextEdit;
};

#endif // pqPythonLineNumberArea_h
