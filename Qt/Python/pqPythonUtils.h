/*=========================================================================

   Program: ParaView
   Module:    pqPythonUtils.h

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

#ifndef pqPythonUtils_h
#define pqPythonUtils_h

#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextEdit>

namespace details
{
/**
 * @brief Returns the id of the first visible block inside a QTextEdit
 * @details Taken from
 * <a
 * href="https://stackoverflow.com/questions/2443358/how-to-add-lines-numbers-to-qtextedit/24596246#24596246">Stackoverflow</a>
 */
inline std::int32_t getFirstVisibleBlockId(const QTextEdit& text)
{
  QTextCursor curs = QTextCursor(text.document());
  curs.movePosition(QTextCursor::Start);
  for (std::int32_t i = 0; i < text.document()->blockCount(); ++i)
  {
    QTextBlock block = curs.block();

    QRect r1 = text.viewport()->geometry();
    QRect r2 = text.document()
                 ->documentLayout()
                 ->blockBoundingRect(block)
                 .translated(text.viewport()->geometry().x(),
                   text.viewport()->geometry().y() - (text.verticalScrollBar()->sliderPosition()))
                 .toRect();

    if (r1.contains(r2, true))
    {
      return i;
    }

    curs.movePosition(QTextCursor::NextBlock);
  }

  return 0;
};

inline QString stripFilename(const QString& filepath)
{
  return QFileInfo(filepath).fileName();
}
}

/**
 * @brief Stack array using an enum as indexer. The enum
 * should contain as its last element END. Otherwise, you should
 * set the third template parameter N to the size of your enum.
 */
template <typename E, class T, std::size_t N = static_cast<size_t>(E::END)>
struct EnumArray : public std::array<T, N>
{
  T& operator[](E e) { return std::array<T, N>::operator[](static_cast<size_t>(e)); }
  const T& operator[](E e) const { return std::array<T, N>::operator[](static_cast<size_t>(e)); }
};

#endif // pqPythonUtils_h
