/*=========================================================================

   Program: ParaView
   Module:    pqFlatTreeView.h

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

/**
* \file pqFlatTreeView.h
* \date 3/27/2006
*/

#ifndef _pqFlatTreeView_h
#define _pqFlatTreeView_h

#include "pqWidgetsModule.h"
#include <QAbstractScrollArea>
#include <QModelIndex>          // Needed for return type
#include <QStyleOptionViewItem> // Needed for return type

class pqFlatTreeViewItem;
class pqFlatTreeViewItemRows;
class pqFlatTreeViewInternal;

class QAbstractItemModel;
class QColor;
class QFontMetrics;
class QHeaderView;
class QItemSelection;
class QItemSelectionModel;
class QPoint;
class QRect;

/**
* \class pqFlatTreeView
* \brief
*   The pqFlatTreeView class is used to display a flattened tree
*   view of a hierarchical model.
*
* The tree view is flattened by taking long indented chains of
* single items and lining them up vertically. If an item is the
* only descendent of its parent item, it is drawn directly below
* its parent. A vertical branch is drawn between the items to
* indicate relationship. If an item has more than one descendent,
* those items are indented from the parent. Normal tree view
* branches are drawn between the parent and child items to show
* the relationship.
*/
class PQWIDGETS_EXPORT pqFlatTreeView : public QAbstractScrollArea
{
  Q_OBJECT

public:
  enum SelectionBehavior
  {
    SelectItems,
    SelectRows,
    SelectColumns
  };

  enum SelectionMode
  {
    NoSelection,
    SingleSelection,
    ExtendedSelection
  };

public:
  /**
  * \brief
  *   Creates a flat tree view.
  * \param parent The parent widget for this instance.
  */
  pqFlatTreeView(QWidget* parent = 0);
  ~pqFlatTreeView() override;

  /**
  * \brief
  *   Used to monitor the header view.
  *
  * When the header view is shown or hidden, the layout needs to be
  * updated and repainted.
  *
  * \param object The object which will receive the event.
  * \param e The event to be sent.
  */
  bool eventFilter(QObject* object, QEvent* e) override;

  /**
  * \name Model Setup Methods
  */
  //@{
  QAbstractItemModel* getModel() const { return this->Model; }
  void setModel(QAbstractItemModel* model);

  QModelIndex getRootIndex() const;
  void setRootIndex(const QModelIndex& index);
  //@}

  /**
  * \name Selection Setup Methods
  */
  //@{
  QItemSelectionModel* getSelectionModel() const { return this->Selection; }
  void setSelectionModel(QItemSelectionModel* selectionModel);

  SelectionBehavior getSelectionBehavior() const { return this->Behavior; }
  void setSelectionBehavior(SelectionBehavior behavior);

  SelectionMode getSelectionMode() const { return this->Mode; }
  void setSelectionMode(SelectionMode mode);
  //@}

  /**
  * \name Column Management Methods
  */
  //@{
  QHeaderView* getHeader() const { return this->HeaderView; }
  void setHeader(QHeaderView* headerView);

  /**
  * \brief
  *   Gets whether or not the column size is managed by the view.
  *
  * Column size management is on by default and used when the
  * view header is hidden. When size management is on, the columns
  * will be resized to fit the model data in the column.
  *
  * \return
  *   True if the column size is managed by the view.
  */
  bool isColumnSizeManaged() const { return this->ManageSizes; }

  /**
  * \brief
  *   Sets whether or not the column size is managed by the view.
  * \param managed True if the column size should be managed.
  * \sa
  *   pqFlatTreeView::isColumnSizeManaged()
  */
  void setColumnSizeManaged(bool managed);
  //@}

  /**
  * \name Drawing Options
  */
  //@{
  int getIconSize() const;
  void setIconSize(int iconSize);
  //@}

  /**
  * \name Index Location Methods
  */
  //@{
  bool isIndexHidden(const QModelIndex& index) const;
  void getVisibleRect(const QModelIndex& index, QRect& area) const;
  QModelIndex getIndexVisibleAt(const QPoint& point) const;
  QModelIndex getIndexCellAt(const QPoint& point) const;
  void getSelectionIn(const QRect& rect, QItemSelection& items) const;
  //@}

  /**
  * \name Index Navigation Methods
  */
  //@{
  bool isIndexExpanded(const QModelIndex& index) const;
  QModelIndex getNextVisibleIndex(
    const QModelIndex& index, const QModelIndex& root = QModelIndex()) const;
  QModelIndex getRelativeIndex(const QString& id, const QModelIndex& root = QModelIndex()) const;
  void getRelativeIndexId(
    const QModelIndex& index, QString& id, const QModelIndex& root = QModelIndex()) const;
  //@}

  /**
  * \name Editing Methods
  */
  //@{
  bool startEditing(const QModelIndex& index);
  void finishEditing();
  void cancelEditing();
  //@}

Q_SIGNALS:
  void activated(const QModelIndex& index);
  void clicked(const QModelIndex& index);

public Q_SLOTS:
  void reset();
  void selectAll();
  void setCurrentIndex(const QModelIndex& index);
  void expandAll();
  void expand(const QModelIndex& index);
  void collapse(const QModelIndex& index);
  void scrollTo(const QModelIndex& index);

protected Q_SLOTS:
  /**
  * \name Model Change Handlers
  */
  //@{
  void insertRows(const QModelIndex& parent, int start, int end);
  void startRowRemoval(const QModelIndex& parent, int start, int end);
  void finishRowRemoval(const QModelIndex& parent, int start, int end);
  void insertColumns(const QModelIndex& parent, int start, int end);
  void startColumnRemoval(const QModelIndex& parent, int start, int end);
  void finishColumnRemoval(const QModelIndex& parent, int start, int end);
  void updateData(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  //@}

protected:
  /**
  * \name Keyboard Event Handlers
  */
  //@{
  void keyPressEvent(QKeyEvent* e) override;
  void keyboardSearch(const QString& search);
  //@}

  /**
  * \name Mouse Event Handlers
  */
  //@{
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void mouseDoubleClickEvent(QMouseEvent* e) override;
  //@}

  /**
  * \name Event Handlers
  */
  //@{
  bool event(QEvent* e) override;
  //@}

  int horizontalOffset() const;
  int verticalOffset() const;

  void resizeEvent(QResizeEvent* e) override;
  bool viewportEvent(QEvent* e) override;
  void paintEvent(QPaintEvent* e) override;
  QStyleOptionViewItem getViewOptions() const;

private Q_SLOTS:
  /**
  * \name Header Signal Handlers
  */
  //@{
  void handleSectionResized(int index, int oldSize, int newSize);
  void handleSectionMoved(int index, int oldVisual, int newVisual);
  //@}

  /**
  * \name Selection Signal Handlers
  */
  //@{
  void changeCurrent(const QModelIndex& current, const QModelIndex& previous);
  void changeCurrentRow(const QModelIndex& current, const QModelIndex& previous);
  void changeCurrentColumn(const QModelIndex& current, const QModelIndex& previous);
  void changeSelection(const QItemSelection& selected, const QItemSelection& deselected);
  //@}

private:
  void resetRoot();
  void resetPreferredSizes();

  /**
  * \name Layout Methods
  */
  //@{
  void layoutEditor();
  void layoutItems();
  void layoutItem(pqFlatTreeViewItem* item, int& point, const QFontMetrics& fm);
  int getDataWidth(const QModelIndex& index, const QFontMetrics& fm) const;
  int getWidthSum(pqFlatTreeViewItem* item, int column) const;
  bool updateContentsWidth();
  void updateScrollBars();
  void addChildItems(pqFlatTreeViewItem* item, int parentChildCount);
  //@}

  /**
  * \name Tree Navigation Methods
  */
  //@{
  bool getIndexRowList(const QModelIndex& index, pqFlatTreeViewItemRows& rowList) const;
  pqFlatTreeViewItem* getItem(const pqFlatTreeViewItemRows& rowList) const;
  pqFlatTreeViewItem* getItem(const QModelIndex& index) const;
  pqFlatTreeViewItem* getItemAt(int contentsY) const;
  pqFlatTreeViewItem* getNextItem(pqFlatTreeViewItem* item) const;
  pqFlatTreeViewItem* getNextVisibleItem(pqFlatTreeViewItem* item) const;
  pqFlatTreeViewItem* getPreviousVisibleItem(pqFlatTreeViewItem* item) const;
  pqFlatTreeViewItem* getLastVisibleItem() const;
  //@}

  void expandItem(pqFlatTreeViewItem* item);

  void getSelectionIn(
    const QModelIndex& topLeft, const QModelIndex& bottomRight, QItemSelection& items) const;

  void drawBranches(QPainter& painter, pqFlatTreeViewItem* item, int halfIndent,
    const QColor& branchColor, const QColor& expandColor, const QStyleOptionViewItem& options);
  bool drawDecoration(QPainter& painter, int px, int py, const QModelIndex& index,
    const QStyleOptionViewItem& options, int itemHeight);
  void drawData(QPainter& painter, int px, int py, const QModelIndex& index,
    const QStyleOptionViewItem& options, int itemHeight, int itemWidth, int columnWidth,
    bool selected);
  void drawFocus(
    QPainter& painter, const QRect& cell, const QStyleOptionViewItem& options, bool selected);

private:
  QAbstractItemModel* Model;
  QItemSelectionModel* Selection;
  SelectionBehavior Behavior;
  SelectionMode Mode;
  QHeaderView* HeaderView;
  pqFlatTreeViewItem* Root;
  pqFlatTreeViewInternal* Internal;
  int IconSize;
  int IndentWidth;
  int ContentsWidth;
  int ContentsHeight;
  int TextMargin;
  int DoubleTextMargin;
  bool FontChanged;
  bool ManageSizes;
  bool InUpdateWidth;
  bool HeaderOwned;
  bool SelectionOwned;

  static int PipeLength;
};

#endif
