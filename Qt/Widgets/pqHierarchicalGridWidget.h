// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHierarchicalGridWidget_h
#define pqHierarchicalGridWidget_h

#include "pqWidgetsModule.h" // for export macros
#include <QScopedPointer>    // for ivar
#include <QWidget>

/**
 * @class pqHierarchicalGridWidget
 * @brief Widget that supports resizing of a pqHierarchicalGridLayout
 *
 * pqHierarchicalGridWidget is intended to be used together with
 * pqHierarchicalGridLayout. It enables interactive resizing of individual
 * widgets placed in the pqHierarchicalGridLayout.
 *
 * When not using pqHierarchicalGridLayout, pqHierarchicalGridWidget will simply
 * act as any other QWidget.
 *
 */
class PQWIDGETS_EXPORT pqHierarchicalGridWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqHierarchicalGridWidget(QWidget* parent = nullptr);
  ~pqHierarchicalGridWidget() override;

  ///@{
  /**
   * Enable/disable interactive resizing of the layout.
   * Default is enabled.
   */
  void setUserResizability(bool);
  bool userResizability() const;
  ///@}

  /**
   * handle cursor changes on mouse move
   */
  bool eventFilter(QObject* caller, QEvent* evt) override;

Q_SIGNALS:
  void splitterMoved(int location, double splitFraction);

protected:
  void mouseMoveEvent(QMouseEvent* evt) override;
  void mousePressEvent(QMouseEvent* evt) override;
  void mouseReleaseEvent(QMouseEvent* evt) override;

private:
  Q_DISABLE_COPY(pqHierarchicalGridWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
  friend class pqInternals;
  void setSplitFraction(int location, double fraction);

  friend class pqHierarchicalGridLayout;

  /**
   * Provides information about splitters, their direction, their location and
   * their range.
   */
  struct SplitterInfo
  {
    Qt::Orientation Direction;
    QRect Bounds;
    int Location;
    int Position;
  };

  void setSplitters(const QVector<SplitterInfo>& splitters);
};

#endif
