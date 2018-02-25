/*=========================================================================

   Program: ParaView
   Module:  pqTreeView.h

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
#ifndef pqTreeView_h
#define pqTreeView_h

#include "pqWidgetsModule.h"
#include <QTreeView>

/**
 * class: pqTreeView
 * brief: QTreeView subclass that add ParaView specific customizations.
 *
 * pqTreeView adds ParaView specific customizations to the QTreeView. These
 * include the following:
 *
 * \li Auto-resize: Oftentimes we want the view to as compact as possible, but
 *     if has up to a certain number of items, it should grow to fit those items
 *     so that the vertical scroll bars don't show up. This is supported using
 *     the `MaximumRowCountBeforeScrolling` property. The pqTreeView will grown
 *     in size vertically to fit the number of items indicated.
 * \li Avoid grabbing scroll focus: Wheel events are not handled by the widget
 *     unless the widget has focus. Together with change in focus policy to
 *     Qt::StrongFocus instead of the default Qt::WheelFocus, we improve the
 *     widget scroll behavior when nested in other scrollable panels.
 */
class PQWIDGETS_EXPORT pqTreeView : public QTreeView
{
  Q_OBJECT

  /**
  * Set the maximum number of rows beyond which this view should show a
  * vertical scroll bar. The pqTreeView will keep on resizing until
  * maximumRowCountBeforeScrolling row to avoid vertical scrolling.
  */
  Q_PROPERTY(int maximumRowCountBeforeScrolling READ maximumRowCountBeforeScrolling WRITE
      setMaximumRowCountBeforeScrolling)

  typedef QTreeView Superclass;

public:
  pqTreeView(QWidget* parent = 0);
  ~pqTreeView() override {}

  bool eventFilter(QObject* object, QEvent* e) override;

  /**
   * Overridden to eat wheel events unless this->hasFocus().
   */
  void wheelEvent(QWheelEvent* event) override;

  void setModel(QAbstractItemModel* model) override;
  void setRootIndex(const QModelIndex& index) override;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  void setMaximumRowCountBeforeScrolling(int val) { this->MaximumRowCountBeforeScrolling = val; }
  int maximumRowCountBeforeScrolling() const { return this->MaximumRowCountBeforeScrolling; }

private slots:
  void invalidateLayout();

private:
  int ScrollPadding;
  int MaximumRowCountBeforeScrolling;
};

#endif
