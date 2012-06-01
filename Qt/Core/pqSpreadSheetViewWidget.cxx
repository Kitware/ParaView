/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewWidget.cxx

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
#include "pqSpreadSheetViewWidget.h"

#include "pqSpreadSheetViewModel.h"
#include "pqNonEditableStyledItemDelegate.h"

#include <QHeaderView>
#include <QItemDelegate>
#include <QPointer>
#include <QTableView>
#include <QTextLayout>
#include <QTextOption>
#include <QPainter>
#include <QPen>
#include <QApplication>
#include <QVBoxLayout>

namespace
{
  inline bool pqIsColumnInternal(const QString& str)
    {
    return (str == "__vtkIsSelected__");
    }
}

//-----------------------------------------------------------------------------
class pqSpreadSheetViewWidget::pqDelegate : public pqNonEditableStyledItemDelegate
{
  typedef pqNonEditableStyledItemDelegate Superclass;
public:
  pqDelegate(QObject* _parent=0):Superclass(_parent)
  {
  }
  void beginPaint()
    {
    this->Top = QModelIndex();
    this->Bottom = QModelIndex();
    }
  void endPaint()
    {
    }

  virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, 
    const QModelIndex& index) const 
    {
    const_cast<pqDelegate*>(this)->Top = (this->Top.isValid() && this->Top < index)?
      this->Top : index;
    const_cast<pqDelegate*>(this)->Bottom = (this->Bottom.isValid() && index < this->Bottom)?
      this->Bottom : index;
    QString text = index.data().toString();
    if (text.isEmpty())
      return;

    // Make sure the text color is appropriate when selection
    QPalette::ColorGroup cg =
        (option.state & QStyle::State_Enabled) ?
          QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
      {
      cg = QPalette::Inactive;
      }
    if (option.state & QStyle::State_Selected)
      {
      painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
      }
    else
      {
      painter->setPen(option.palette.color(cg, QPalette::Text));
      }

    const int textMargin =
        QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    QRect textRect = option.rect.adjusted(textMargin, 0, -textMargin, 0);
    this->TextOption.setWrapMode(QTextOption::ManualWrap);
    this->TextOption.setTextDirection(option.direction);
    this->TextOption.setAlignment(
      QStyle::visualAlignment(option.direction, option.displayAlignment));
    
    int split = text.split("\t").length();
    this->TextOption.setTabStop(option.rect.width()/split);
    this->TextLayout.setTextOption(this->TextOption);
    this->TextLayout.setFont(option.font);
    this->TextLayout.setText(text);

    QSizeF textLayoutSize = this->doTextLayout(textRect.width());

    if (textRect.width() < textLayoutSize.width()
      || textRect.height() < textLayoutSize.height()) {
      QString elided;
      int start = 0;
      int end = text.indexOf(QChar::LineSeparator, start);
      if (end == -1) {
        elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
      } else while (end != -1) {
        elided += option.fontMetrics.elidedText(text.mid(start, end - start),
          option.textElideMode, textRect.width());
        start = end + 1;
        end = text.indexOf(QChar::LineSeparator, start);
      }
      this->TextLayout.setText(elided);
      textLayoutSize = this->doTextLayout(textRect.width());
    }

    const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
      layoutSize, textRect);
    this->TextLayout.draw(painter, layoutRect.topLeft(),
      QVector<QTextLayout::FormatRange>(), layoutRect);
    }

  QSizeF doTextLayout(int lineWidth) const
    {
    QFontMetrics fontMetrics(this->TextLayout.font());
    int leading = fontMetrics.leading();
    qreal height = 0;
    qreal widthUsed = 0;
    this->TextLayout.beginLayout();
    while (true) {
      QTextLine line = this->TextLayout.createLine();
      if (!line.isValid())
        break;
      line.setLineWidth(lineWidth);
      height += leading;
      line.setPosition(QPointF(0, height));
      height += line.height();
      widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    this->TextLayout.endLayout();
    return QSizeF(widthUsed, height);
    }

  QModelIndex Top;
  QModelIndex Bottom;
  mutable QTextLayout TextLayout;
  mutable QTextOption TextOption;
};


//-----------------------------------------------------------------------------
pqSpreadSheetViewWidget::pqSpreadSheetViewWidget(QWidget* parentObject)
  : Superclass(parentObject)
{
  // setup some defaults.
  this->setAlternatingRowColors(true);
  this->setCornerButtonEnabled(false);
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->horizontalHeader()->setMovable(true);
  this->SingleColumnMode = false;

  //setup the delegate.
  this->setItemDelegate(new pqDelegate(this));

  QObject::connect(
    this->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)),
    this, SLOT(onSectionDoubleClicked(int)), Qt::QueuedConnection);

  QObject::connect(
      this->horizontalHeader(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
      this, SLOT(onSortIndicatorChanged(int,Qt::SortOrder)));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewWidget::~pqSpreadSheetViewWidget()
{
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewWidget::setModel(QAbstractItemModel* model)
{
  // if model is non-null, then it must be a pqSpreadSheetViewModel.
  Q_ASSERT(model==NULL || qobject_cast<pqSpreadSheetViewModel*>(model) != NULL);
  this->Superclass::setModel(model);
  if (model)
    {
    QObject::connect(
      model, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
      this, SLOT(onHeaderDataChanged()));
    QObject::connect(
      model, SIGNAL(modelReset()),
      this, SLOT(onHeaderDataChanged()));
    QObject::connect(
      model, SIGNAL(modelReset()),
      this, SLOT(sortColumns()));
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewWidget::onHeaderDataChanged()
{
  pqSpreadSheetViewModel* shModel =
      qobject_cast<pqSpreadSheetViewModel*>(this->model());
  for (int cc=0; cc < shModel->columnCount(); cc++)
    {
    QString headerTitle =
      this->model()->headerData(cc, Qt::Horizontal).toString();
    this->setColumnHidden(cc, !shModel->isVisible(cc) || pqIsColumnInternal(headerTitle));
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
  pqSpreadSheetViewModel* smodel = 
    qobject_cast<pqSpreadSheetViewModel*>(this->model());
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
/// Called when user double clicks on a column header.
void pqSpreadSheetViewWidget::onSectionDoubleClicked(int logicalindex)
{
  int numcols = this->model()->columnCount();
  if (logicalindex < 0 || logicalindex >= numcols)
    {
    return;
    }

  QHeaderView* header = this->horizontalHeader();
  this->SingleColumnMode = !this->SingleColumnMode;
  for (int cc=0; cc < numcols;cc++)
    {
    QString headerTitle =
      this->model()->headerData(cc, Qt::Horizontal).toString();
    if (pqIsColumnInternal(headerTitle))
      {
      this->setColumnHidden(cc, true);
      }
    else
      {
      this->setColumnHidden(cc, (this->SingleColumnMode && cc!=logicalindex));
      }
    if (this->SingleColumnMode && cc == logicalindex)
      {
      header->setResizeMode(cc, QHeaderView::Stretch);
      }
    else if (!this->SingleColumnMode)
      {
      header->setResizeMode(cc, QHeaderView::Interactive);
      }
    }

  if (!this->SingleColumnMode)
    {
    this->resizeColumnsToContents();
    }
}
//-----------------------------------------------------------------------------
/// Called when user clicks on a column header for sorting purpose.
void pqSpreadSheetViewWidget::onSortIndicatorChanged(int section, Qt::SortOrder order)
{
  // Qt side
  pqSpreadSheetViewModel* internModel =
      qobject_cast<pqSpreadSheetViewModel*>(this->model());
  if(internModel->isSortable(section))
    {
    internModel->sortSection(section, order);
    this->horizontalHeader()->setSortIndicatorShown(true);
    }
  else
    {
    this->horizontalHeader()->setSortIndicatorShown(false);
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewWidget::sortColumns()
{
  int targetVisualIndex = 0;
  const char* order[6] = {"Point ID", "Cell ID", "Block Number", "ObjectID", "Points", "Structured Coordinates"};

  for(int k=0 ; k < 6; k++)
    {
    const char* columnSearched = order[k];
    for(int i=0; i < this->model()->columnCount(); i++)
      {
      QString name = this->model()->headerData(i, Qt::Horizontal).toString();
      if(name == columnSearched)
        {
        this->horizontalHeader()->moveSection(this->horizontalHeader()->visualIndex(i), targetVisualIndex++);
        break;
        }
      }
    }
}
