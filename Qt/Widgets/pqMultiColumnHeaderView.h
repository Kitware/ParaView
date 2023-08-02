// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMultiColumnHeaderView_h
#define pqMultiColumnHeaderView_h

#include "pqWidgetsModule.h" // for exports
#include <QHeaderView>

/**
 * @class pqMultiColumnHeaderView
 * @brief QHeaderView that supports showing multiple sections as one.
 *
 * pqMultiColumnHeaderView extents QHeaderView to support showing multiple
 * adjacent sections as a single section. This is useful for showing vector
 * quantities, for example. Instead of each component taking up header space
 * and making it confusing to understand that the various sections are part of
 * the same vector, pqMultiColumnHeaderView can show all those sections under a
 * single banner. It still supports resizing individual sections thus does not
 * inhibit usability.
 *
 * pqMultiColumnHeaderView simply combines adjacent sections with same
 * (non-empty) `QString` value for Qt::`DisplayRole`. This is done by
 * overriding `QHeaderView::paintSection` and custom painting such
 * sections spanning multiple sections.
 */
class PQWIDGETS_EXPORT pqMultiColumnHeaderView : public QHeaderView
{
  Q_OBJECT;
  typedef QHeaderView Superclass;

public:
  pqMultiColumnHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);
  ~pqMultiColumnHeaderView() override;

protected:
  void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

private:
  QPair<int, int> sectionSpan(int visualIndex) const;
  QString sectionDisplayText(int logicalIndex) const;

  Q_DISABLE_COPY(pqMultiColumnHeaderView);
};

#endif
