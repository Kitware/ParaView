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
#include <map>

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
      this->resizeColumnsToContentsRespectingComponents();
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
void pqSpreadSheetViewWidget::resizeColumnsToContentsRespectingComponents()
{
  QHeaderView* hdrView = this->horizontalHeader();
  pqSpreadSheetViewModel* smodel = qobject_cast<pqSpreadSheetViewModel*>(this->model());

  // First we need to find out which columns are actually multi-component columns. We do
  // not have access to the underlying VTK data, so we are simply collecting the indices of
  // all columns with a specific name, so the size of this array gives us the number of
  // components for a specific name
  // At the same time find out if we have any multi-component columns at all
  std::map<QString, std::vector<int>> visHdrNameInx;
  bool hasMultiCompCols = false;
  for (int c = 0; c < smodel->columnCount(); ++c)
  {
    if (hdrView->isSectionHidden(c))
    {
      continue;
    }

    QString hdrLabel = smodel->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
    std::vector<int>& hdrInxVec =
      visHdrNameInx[smodel->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString()];
    hdrInxVec.push_back(c);

    if (1 < hdrInxVec.size())
    {
      hasMultiCompCols = true;
    }
  }

  // First calculate all column widths ignoring multi-component columns and remember
  // the results
  // If we do not have multi-component columns at all, we are already done here
  this->resizeColumnsToContents();
  if (!hasMultiCompCols)
  {
    return;
  }

  // Remember the current initial column widths
  std::vector<int> visHdrWidths0;
  for (int c = 0; c < smodel->columnCount(); ++c)
  {
    if (hdrView->isSectionHidden(c))
    {
      visHdrWidths0.push_back(-1);
    }
    else
    {
      visHdrWidths0.push_back(hdrView->sectionSize(c));
    }
  }

  // For the rest of this function we want to stop signalling from the model because we are
  // temporarily manipulate the header label output
  bool sigBlockBefore = smodel->blockSignals(true);

  // Mark those table columns for empty display headers that are part of a
  // multi-component column
  for (auto it = visHdrNameInx.begin(); it != visHdrNameInx.end(); ++it)
  {
    if (1 >= it->second.size())
    {
      continue;
    }

    for (auto ith = it->second.begin(); ith != it->second.end(); ++ith)
    {
      smodel->setHeaderData(*ith, Qt::Horizontal, 0, pqSpreadSheetViewModel::SectionHeaderTemp);
    }
  }

  // Calculate column widths with all multi-component column headers empty
  // and remember the results
  this->resizeColumnsToContents();
  std::vector<int> visHdrWidthsEmpty;
  for (int c = 0; c < smodel->columnCount(); ++c)
  {
    if (hdrView->isSectionHidden(c))
    {
      visHdrWidthsEmpty.push_back(-1);
    }
    else
    {
      visHdrWidthsEmpty.push_back(hdrView->sectionSize(c));
    }
  }

  // Since we have no way to directly set the section width in a header view, we will again
  // manipulate the header label that the model returns. In order to get a reasonable approximation,
  // we are using a string that contains as many 'a' characters as needed to achieve a specific
  // string width
  QFontMetrics fontm = hdrView->fontMetrics();
  int aWidth = fontm.size(0, "a").width();

  // For every multi-component column we check whether the column width is determined
  // by the header label or the data
  for (auto it = visHdrNameInx.begin(); it != visHdrNameInx.end(); ++it)
  {
    if (1 >= it->second.size())
    {
      continue;
    }

    for (auto ith = it->second.begin(); ith != it->second.end(); ++ith)
    {
      // We divide the "original" column width by the number of components, and then we
      // see whether this value is larger or the column width that is calculated by
      // considering data only (and give it a little bit "extra")
      int compWidth = visHdrWidths0[*ith] / static_cast<int>(it->second.size());
      if (visHdrWidthsEmpty[*ith] < compWidth)
      {
        smodel->setHeaderData(
          *ith, Qt::Horizontal, compWidth / aWidth + 1, pqSpreadSheetViewModel::SectionHeaderTemp);
      }
    }
  }

  // Now we can calculate the column widths and get a result as desired
  this->resizeColumnsToContents();

  // Remove temporary header manipulation, but do not recalculate column widths because
  // the current manipulated widths are exactly what we want to achieve with this function!
  for (auto it = visHdrNameInx.begin(); it != visHdrNameInx.end(); ++it)
  {
    if (1 >= it->second.size())
    {
      continue;
    }

    for (auto ith = visHdrNameInx[it->first].begin(); ith != visHdrNameInx[it->first].end(); ++ith)
    {
      smodel->setHeaderData(*ith, Qt::Horizontal, -1, pqSpreadSheetViewModel::SectionHeaderTemp);
    }
  }

  // Re-enable signalling
  smodel->blockSignals(sigBlockBefore);
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
