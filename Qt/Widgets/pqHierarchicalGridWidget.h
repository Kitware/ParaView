/*=========================================================================

   Program: ParaView
   Module:  pqHierarchicalGridWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
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
  pqHierarchicalGridWidget(QWidget* parent = 0);
  ~pqHierarchicalGridWidget() override;

  //@{
  /**
   * Enable/disable interactive resizing of the layout.
   * Default is enabled.
   */
  void setUserResizability(bool);
  bool userResizability() const;
  //@}

  /**
   * handle cursor changes on mouse move
   */
  bool eventFilter(QObject* caller, QEvent* evt) override;

signals:
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
