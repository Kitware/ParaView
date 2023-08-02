// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
