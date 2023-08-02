// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpreadSheetViewWidget.h"

#include "pqNonEditableStyledItemDelegate.h"
#include "pqSpreadSheetViewModel.h"

#include "pqMultiColumnHeaderView.h"

#include <QApplication>
#include <QHeaderView>
#include <QItemDelegate>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QTableView>
#include <QTextLayout>
#include <QTextOption>
#include <QVBoxLayout>

#include <cassert>

//-----------------------------------------------------------------------------
/*
 * This delegate helps us keep track of rows that are being painted,
 * that way we can control which blocks of data to request from the server side
 */
class pqSpreadSheetViewWidget::pqDelegate : public pqNonEditableStyledItemDelegate
{
  typedef pqNonEditableStyledItemDelegate Superclass;

public:
  pqDelegate(QObject* _parent = nullptr)
    : Superclass(_parent)
  {
  }
  void beginPaint()
  {
    this->Top = QModelIndex();
    this->Bottom = QModelIndex();
  }
  void endPaint() {}

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    this->Top = (this->Top.isValid() && this->Top < index) ? this->Top : index;
    this->Bottom = (this->Bottom.isValid() && index < this->Bottom) ? this->Bottom : index;
    this->Superclass::paint(painter, option, index);
  }

  mutable QModelIndex Top;
  mutable QModelIndex Bottom;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewWidget::pqSpreadSheetViewWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , SingleColumnMode(false)
  , OldColumnCount(0)
{
  // setup some defaults.
  this->setAlternatingRowColors(true);
  this->setCornerButtonEnabled(false);
  this->setSelectionBehavior(QAbstractItemView::SelectRows);

  auto hheader = new pqMultiColumnHeaderView(Qt::Horizontal, this);
  hheader->setObjectName("Header");
  hheader->setSectionsClickable(true);
  hheader->setSectionsMovable(false);
  hheader->setHighlightSections(false);
  // limit to using 100 columns when resizing. This addresses performance issues with
  // large data. Note visible columns only (0) is not adequate since the widget may
  // not be visible at all when being resized and we e
  hheader->setResizeContentsPrecision(100);
  this->setHorizontalHeader(hheader);

  // setup the delegate.
  this->setItemDelegate(new pqDelegate(this));

  QObject::connect(this->horizontalHeader(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this,
    SLOT(onSortIndicatorChanged(int, Qt::SortOrder)));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewWidget::~pqSpreadSheetViewWidget() = default;

//-----------------------------------------------------------------------------
void pqSpreadSheetViewWidget::setModel(QAbstractItemModel* modelToUse)
{
  // if model is non-nullptr, then it must be a pqSpreadSheetViewModel.
  assert(modelToUse == nullptr || qobject_cast<pqSpreadSheetViewModel*>(modelToUse) != nullptr);
  this->Superclass::setModel(modelToUse);
  if (modelToUse)
  {
    QObject::connect(modelToUse, SIGNAL(headerDataChanged(Qt::Orientation, int, int)), this,
      SLOT(onHeaderDataChanged()));
    QObject::connect(modelToUse, SIGNAL(modelReset()), this, SLOT(onHeaderDataChanged()));
  }

  // ensure headers are properly displayed for the new data
  this->onHeaderDataChanged();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewWidget::onHeaderDataChanged()
{
  if (auto amodel = this->model())
  {
    const int colcount = amodel->columnCount();
    for (int cc = 0; cc < colcount; cc++)
    {
      bool visible =
        amodel->headerData(cc, Qt::Horizontal, pqSpreadSheetViewModel::SectionVisible).toBool();
      this->setColumnHidden(cc, !visible);
    }

    if (this->OldColumnCount != colcount)
    {
      // don't resize column unless the column count really changed.
      // this overcomes #18430.
      this->resizeColumnsToContents();
      this->OldColumnCount = colcount;
    }
  }
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel* pqSpreadSheetViewWidget::spreadSheetViewModel() const
{
  return qobject_cast<pqSpreadSheetViewModel*>(this->Superclass::model());
}

//-----------------------------------------------------------------------------
// As one scrolls through the table view, a QAbstractItemView requests the data
// for all elements scrolled through, not only the ones eventually visible. We
// do this trick with pqDelegate to make the model request the
// data only for the region eventually visible to the user.
void pqSpreadSheetViewWidget::paintEvent(QPaintEvent* pevent)
{
  pqDelegate* del = dynamic_cast<pqDelegate*>(this->itemDelegate());
  pqSpreadSheetViewModel* smodel = qobject_cast<pqSpreadSheetViewModel*>(this->model());
  if (del && smodel)
  {
    del->beginPaint();
  }
  this->Superclass::paintEvent(pevent);
  if (del && smodel)
  {
    del->endPaint();
    smodel->setActiveRegion(del->Top.row(), del->Bottom.row());
  }
}

//-----------------------------------------------------------------------------
/// Called when user clicks on a column header for sorting purpose.
void pqSpreadSheetViewWidget::onSortIndicatorChanged(int section, Qt::SortOrder order)
{
  // Qt side
  pqSpreadSheetViewModel* internModel = qobject_cast<pqSpreadSheetViewModel*>(this->model());
  if (internModel->isSortable(section))
  {
    internModel->sortSection(section, order);
    this->horizontalHeader()->setSortIndicatorShown(true);
  }
  else
  {
    this->horizontalHeader()->setSortIndicatorShown(false);
  }
}
