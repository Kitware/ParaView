/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqTableView_h
#define pqTableView_h

#include "pqWidgetsModule.h" // for export macro
#include <QTableView>

/**
* pqTableView extends QTableView (in the spirit of pqTableView) to resize the
* view vertically to fit contents. It servers to purposes:
* \li Avoids putting a scroll bar until the table reaches a certain length.
* \li Avoids taking too much space when table has fewer rows.
*/
class PQWIDGETS_EXPORT pqTableView : public QTableView
{
  Q_OBJECT
  typedef QTableView Superclass;

  /**
  * Maximum number of rows beyond which this view should show a
  * vertical scroll bar. The pqTableView will keep on resizing until
  * maximumRowCountBeforeScrolling row to avoid vertical scrolling.
  * Set this to 0, and this will behave exactly as QTableView.
  * Default is 0.
  */
  Q_PROPERTY(int maximumRowCountBeforeScrolling READ maximumRowCountBeforeScrolling WRITE
      setMaximumRowCountBeforeScrolling);

  /**
  * The number of rows to use as the minimum to determine the size of the
  * widget when there are fewer or no rows.
  */
  Q_PROPERTY(int minimumRowCount READ minimumRowCount WRITE setMinimumRowCount);

  /**
  * The number of rows to always pad the widget with. This is used, so long as
  * the total number of rows doesn't exceed the
  * maximumRowCountBeforeScrolling.
  * Note this gets added on top of the padding added, if any, due to
  * minimumRowCount.
  */
  Q_PROPERTY(int padding READ padding WRITE setPadding);

public:
  pqTableView(QWidget* parent = 0);
  virtual ~pqTableView();

  /**
  * Set the maximum number of rows beyond which this view should show a
  * vertical scroll bar. The pqTableView will keep on resizing until
  * maximumRowCountBeforeScrolling row to avoid vertical scrolling.
  * Set this to 0, and this will behave exactly as QTableView.
  * Default is 0.
  */
  void setMaximumRowCountBeforeScrolling(int val) { this->MaximumRowCountBeforeScrolling = val; }
  int maximumRowCountBeforeScrolling() const { return this->MaximumRowCountBeforeScrolling; }

  /**
  * Set the number of rows to use as the minimum to determine the size of the
  * widget when there are fewer or no rows.
  */
  void setMinimumRowCount(int val) { this->MinimumRowCount = val; }
  int minimumRowCount() const { return this->MinimumRowCount; }

  /**
  * Set the number of rows to always pad the widget with. This is used
  * when the actual number of rows is less than or equal to
  * maximumRowCountBeforeScrolling for a non-zero
  * maximumRowCountBeforeScrolling.
  */
  void setPadding(int val) { this->Padding = val; }
  int padding() const { return this->Padding; }

  /**
  * Overridden to ensure the view updates its size as rows are
  * added/removed.
  */
  virtual void setModel(QAbstractItemModel* model);
  virtual void setRootIndex(const QModelIndex& index);

  /**
  * Overridden to report size as per the state of this pqTableView.
  */
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

  /**
  * Overridden to handle events from QScrollBar.
  */
  virtual bool eventFilter(QObject* watched, QEvent* evt);

private slots:
  void invalidateLayout();

private:
  Q_DISABLE_COPY(pqTableView)

  int MaximumRowCountBeforeScrolling;
  int MinimumRowCount;
  int Padding;
  int ScrollPadding;
};

#endif
