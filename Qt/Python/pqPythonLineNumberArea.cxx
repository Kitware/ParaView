/*=========================================================================

   Program: ParaView
   Module:    pqPythonLineNumberArea.cxx

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
