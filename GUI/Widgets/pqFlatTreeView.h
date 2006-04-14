/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqFlatTreeView.h
/// \date 3/27/2006

#ifndef _pqFlatTreeView_h
#define _pqFlatTreeView_h


#include "QtWidgetsExport.h"
#include <QAbstractItemView>

class pqFlatTreeViewItem;
class pqFlatTreeViewItemRows;
class pqFlatTreeViewInternal;

class QColor;
class QFontMetrics;
class QHeaderView;
class QItemSelection;
class QPoint;
class QStyleOptionViewItem;


/// \class pqFlatTreeView
/// \brief
///   The pqFlatTreeView class is used to display a flattened tree
///   view of a hierarchical model.
///
/// The tree view is flattened by taking long indented chains of
/// single items and lining them up vertically. If an item is the
/// only descendent of its parent item, it is drawn directly below
/// its parent. A vertical branch is drawn between the items to
/// indicate relationship. If an item has more than one descendent,
/// those items are indented from the parent. Normal tree view
/// branches are drawn between the parent and child items to show
/// the relationship.
class QTWIDGETS_EXPORT pqFlatTreeView : public QAbstractItemView
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a flat tree view.
  /// \param parent The parent widget for this instance.
  pqFlatTreeView(QWidget *parent=0);
  virtual ~pqFlatTreeView();

  virtual QModelIndex indexAt(const QPoint &point) const;
  virtual QRect visualRect(const QModelIndex &index) const;
  virtual void scrollTo(const QModelIndex &index,
      QAbstractItemView::ScrollHint hint=QAbstractItemView::EnsureVisible);
  virtual void keyboardSearch(const QString &search);

  virtual void setModel(QAbstractItemModel *model);
  virtual void setSelectionModel(QItemSelectionModel *itemSelectionModel);
  virtual void reset();
  virtual void setRootIndex(const QModelIndex &index);
  virtual bool eventFilter(QObject *object, QEvent *e);

  /// \name Column Management Methods
  //@{
  QHeaderView *header() const {return this->HeaderView;}
  void setHeader(QHeaderView *headerView);

  /// \brief
  ///   Gets whether or not the column size is managed by the view.
  ///
  /// Column size management is on by default and used when the
  /// view header is hidden. When size management is on, the columns
  /// will be resized to fit the model data in the column.
  ///
  /// \return
  ///   True if the column size is managed by the view.
  bool isColumnSizeManaged() const {return this->ManageSizes;}

  /// \brief
  ///   Sets whether or not the column size is managed by the view.
  /// \param managed True if the column size should be managed.
  /// \sa
  ///   pqFlatTreeView::isColumnSizeManaged()
  void setColumnSizeManaged(bool managed);
  //@}

public slots:
  void expand(const QModelIndex &index);
  void collapse(const QModelIndex &index);

protected slots:
  void rowsRemoved(const QModelIndex &parentIndex, int start, int end);

protected:
  virtual void rowsInserted(const QModelIndex &parentIndex,
      int start, int end);
  virtual void rowsAboutToBeRemoved(const QModelIndex &parentIndex,
      int start, int end);
  virtual void dataChanged(const QModelIndex &topLeft,
      const QModelIndex &bottomRight);

  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction,
      Qt::KeyboardModifiers modifiers);

  virtual bool isIndexHidden(const QModelIndex &index) const;
  virtual void setSelection(const QRect &rect,
      QItemSelectionModel::SelectionFlags command);
  virtual QRegion visualRegionForSelection(
      const QItemSelection &selection) const;

  virtual int horizontalOffset() const;
  virtual int verticalOffset() const;
  virtual void resizeEvent(QResizeEvent *e);
  virtual bool viewportEvent(QEvent *e);
  virtual void paintEvent(QPaintEvent *e);

private slots:
  void handleSectionResized(int index, int oldSize, int newSize);
  void handleSectionMoved(int index, int oldVisual, int newVisual);

  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);
  void changeCurrentRow(const QModelIndex &current,
      const QModelIndex &previous);
  void changeCurrentColumn(const QModelIndex &current,
      const QModelIndex &previous);
  void changeSelection(const QItemSelection &selected,
      const QItemSelection &deselected);

private:
  void resetInternal(int numberOfColumns);
  void layoutItems();
  void layoutItem(pqFlatTreeViewItem *item, int &point,
      const QFontMetrics &fm);
  bool updateContentsWidth();
  void updateScrollBars();
  void addChildItems(pqFlatTreeViewItem *item, int parentChildCount);
  bool getIndexRowList(const QModelIndex &index,
      pqFlatTreeViewItemRows &rowList) const;
  pqFlatTreeViewItem *getItem(const pqFlatTreeViewItemRows &rowList) const;
  pqFlatTreeViewItem *getItem(const QModelIndex &index) const;
  pqFlatTreeViewItem *getNextVisibleItem(pqFlatTreeViewItem *item) const;
  pqFlatTreeViewItem *getPreviousVisibleItem(pqFlatTreeViewItem *item) const;
  pqFlatTreeViewItem *getLastVisibleItem() const;

  void drawBranches(QPainter &painter, pqFlatTreeViewItem *item,
      int halfIndent, const QColor &branchColor, const QColor &expandColor,
      QStyleOptionViewItem &options);

private:
  QHeaderView *HeaderView;
  pqFlatTreeViewItem *Root;
  pqFlatTreeViewInternal *Internal;
  int ItemHeight;
  int IndentWidth;
  int ContentsWidth;
  int ContentsHeight;
  bool FontChanged;
  bool ManageSizes;
  bool InUpdateWidth;

  static int TextMargin;
  static int DoubleTextMargin;
  static int PipeLength;
};

#endif
