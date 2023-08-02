// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonLineNumberArea.h"

#include "pqPythonScriptEditor.h"
#include "pqPythonUtils.h"

#include <cmath>

#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

namespace details
{
//-----------------------------------------------------------------------------
inline std::uint32_t getNumberOfDigits(std::uint32_t i)
{
  return i > 0 ? static_cast<std::int32_t>(std::log10(static_cast<float>(i)) + 1) : 1;
}
}

//-----------------------------------------------------------------------------
QSize pqPythonLineNumberArea::sizeHint() const
{
  const std::uint32_t numberOfDigits =
    details::getNumberOfDigits(std::max(1, this->TextEdit.document()->blockCount()));

  const std::int32_t space = 2 * this->TextEdit.fontMetrics().horizontalAdvance(' ') +
    numberOfDigits * this->TextEdit.fontMetrics().horizontalAdvance(QLatin1Char('9'));

  return QSize{ space, this->TextEdit.height() };
}

//-----------------------------------------------------------------------------
void pqPythonLineNumberArea::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);

  const QPalette& palette = this->palette();

  painter.fillRect(event->rect(), palette.window());
  painter.setFont(this->TextEdit.font());

  const QSize size = this->sizeHint();

  std::int32_t firstBlockId = std::max(0, details::getFirstVisibleBlockId(this->TextEdit) - 1);
  QTextBlock block = this->TextEdit.document()->findBlockByNumber(firstBlockId);

  while (block.isValid() && block.isVisible())
  {
    const QTextCursor blockCursor(block);
    const QRect blockCursorRect = this->TextEdit.cursorRect(blockCursor);

    painter.setPen((this->TextEdit.textCursor().blockNumber() == firstBlockId)
        ? palette.text().color()
        : palette.placeholderText().color());
    const QString number = QString::number(firstBlockId + 1);
    painter.drawText(-5, blockCursorRect.y() + 2, size.width(),
      this->TextEdit.fontMetrics().height(), Qt::AlignRight, number);

    block = block.next();
    firstBlockId++;
  }
}
