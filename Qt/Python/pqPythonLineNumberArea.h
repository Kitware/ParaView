// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonLineNumberArea_h
#define pqPythonLineNumberArea_h

#include "pqPythonModule.h"

#include <QWidget>

class QTextEdit;

/**
 * @class pqPythonLineNumberArea
 * @brief QWidget that displays line number for a QTextEdit.
 * @details This widget is to be associated with a QTextEdit
 * widget (stored as text) this class. It displays the number
 * of lines inside the text and highlight the line where the
 * cursor is.
 *
 * This must be used inside a QHLayout and placed either left
 * or right of the refered text. The paintEvent is in charge
 * of painting this widget.
 */
class PQPYTHON_EXPORT pqPythonLineNumberArea : public QWidget
{
  Q_OBJECT

public:
  /* @brief Constructs a pqPythonLineNumberArea given a text
   * @param parent the parent widget for the Qt ownership
   * @param text the text to display the line from
   */
  explicit pqPythonLineNumberArea(QWidget* parent, const QTextEdit& text)
    : QWidget(parent)
    , TextEdit(text)
  {
  }

  /**
   * @brief Return the size hint based on the number of lines present in the text
   */
  QSize sizeHint() const override;

protected:
  /** @brief Paint the widget
   * @details This method paints the widget area by going through the
   * visible block inside text. Are displayed the line number from
   * the text in one color and the line containing the cursor
   * in another color.
   * @param event the pain event
   */
  void paintEvent(QPaintEvent* event) override;

private:
  /**
   * @brief The text to display the number of line on
   */
  const QTextEdit& TextEdit;
};

#endif // pqPythonLineNumberArea_h
