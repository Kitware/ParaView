/*=========================================================================

   Program: ParaView
   Module:    pqComparativeCueWidget.cxx

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
#include "pqComparativeCueWidget.h"
#include "ui_pqComparativeParameterRangeDialog.h"

#include <QRegExpValidator>

#include "pqUndoStack.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMPropertyHelper.h"

#include <vtkstd/vector>

namespace
{
  class pqLock
    {
    bool *Var;
    bool Prev;
  public:
    pqLock(bool *var, bool val)
      {
      this->Var = var;
      this->Prev = *this->Var;
      *this->Var = val;
      }
    ~pqLock()
      {
      *this->Var = this->Prev;
      }
    };

  vtkstd::vector<double> getValues(const QString& str)
    {
    vtkstd::vector<double> values;
    QStringList parts = str.split(',', QString::SkipEmptyParts);
    foreach (QString part, parts)
      {
      values.push_back(QVariant(part).toDouble());
      }
    return values;
    }
};

//-----------------------------------------------------------------------------
pqComparativeCueWidget::pqComparativeCueWidget(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->Size = QSize(2, 2);
  this->IdleUpdateTimer.setInterval(0);
  this->IdleUpdateTimer.setSingleShot(true);
  QObject::connect(&this->IdleUpdateTimer, SIGNAL(timeout()),
    this, SLOT(updateGUI()));
  QObject::connect(this, SIGNAL(itemSelectionChanged()),
    this, SLOT(onSelectionChanged()));
  QObject::connect(this, SIGNAL(cellChanged(int, int)),
    this, SLOT(onCellChanged(int, int)));
  this->SelectionChanged = false;
  this->InUpdateGUI = false;
}

//-----------------------------------------------------------------------------
pqComparativeCueWidget::~pqComparativeCueWidget()
{
  this->VTKConnect->Disconnect();
  this->VTKConnect->Delete();
  this->VTKConnect = 0;
}

//-----------------------------------------------------------------------------
void pqComparativeCueWidget::setCue(vtkSMProxy* cue)
{
  if (this->Cue.GetPointer() == cue)
    {
    return;
    }
  this->VTKConnect->Disconnect();
  this->Cue = vtkSMComparativeAnimationCueProxy::SafeDownCast(cue);
  if (this->Cue)
    {
    this->VTKConnect->Connect(this->Cue, vtkCommand::ModifiedEvent,
      this, SLOT(updateGUIOnIdle()));
    this->VTKConnect->Connect(this->Cue, vtkCommand::PropertyModifiedEvent,
      this, SLOT(updateGUIOnIdle()));
    }
  this->updateGUI();
  this->setEnabled(this->Cue != NULL);
}

//-----------------------------------------------------------------------------
vtkSMComparativeAnimationCueProxy* pqComparativeCueWidget::cue() const
{
  return this->Cue;
}

//-----------------------------------------------------------------------------
bool pqComparativeCueWidget::acceptsMultipleValues() const
{
  return (this->Cue && this->Cue->GetAnimatedElement() == -1);
}

//-----------------------------------------------------------------------------
void pqComparativeCueWidget::updateGUI()
{
  pqLock lock(&this->InUpdateGUI, true);

  this->clear();
  int rows = this->size().height();
  int cols = this->size().width();

  this->setRowCount(rows);
  this->setColumnCount(cols);

  // set header labels.
  QStringList vlabels, hlabels;
  for (int cc=0; cc < rows; cc++)
    {
    vlabels.push_back(QString("%1").arg(cc));
    }
  this->setVerticalHeaderLabels(vlabels);

  for (int cc=0; cc < cols; cc++)
    {
    char a = 'A';
    a+=cc;
    hlabels.push_back(QString::fromAscii(&a, 1));
    }
  this->setHorizontalHeaderLabels(hlabels);

  vtkSMComparativeAnimationCueProxy* acue = this->cue();
  if (!acue)
    {
    return;
    }

  for (int col=0; col < cols; col++)
    {
    for (int row=0; row < rows; row++)
      {
      QTableWidgetItem* item = new QTableWidgetItem();

      unsigned int numvalues = 0;
      double* values = acue->GetValues(col, row, cols, rows, numvalues);
      if (numvalues >= 1)
        {
        QStringList val_list;
        for (unsigned int cc=0; cc < numvalues; cc++)
          {
          val_list.push_back(QString("%1").arg(values[cc]));
          }
        item->setText(val_list.join(","));
        }
      else
        {
        item->setText("");
        }
      this->setItem(row, col, item);
      }
    }
}

//-----------------------------------------------------------------------------
void pqComparativeCueWidget::onCellChanged(int row, int col)
{
  if (this->InUpdateGUI)
    {
    return;
    }
  BEGIN_UNDO_SET("Parameter Changed");
  QString text = this->item(row, col)->text();
  if (this->acceptsMultipleValues())
    {
    QStringList parts = text.split(',', QString::SkipEmptyParts);
    if (parts.size() > 0)
      {
      double *newvalues = new double[parts.size()];
      double *ptr = newvalues;
      foreach (QString part, parts)
        {
        *ptr = QVariant(part).toDouble();
        ptr++;
        }
      this->cue()->UpdateValue(col, row, newvalues,
        static_cast<unsigned int>(parts.size()));
      }
    }
  else
    {
    double data = QVariant(text).toDouble();
    this->cue()->UpdateValue(col, row, data);
    }
  END_UNDO_SET();

  emit this->valuesChanged();
}

//-----------------------------------------------------------------------------
void pqComparativeCueWidget::mouseReleaseEvent(QMouseEvent* evt)
{
  this->Superclass::mouseReleaseEvent(evt);
  if (this->SelectionChanged)
    {
    this->editRange();
    this->SelectionChanged = false;
    }
}
//-----------------------------------------------------------------------------
void pqComparativeCueWidget::editRange()
{
  QList<QTableWidgetSelectionRange> ranges = this->selectedRanges();
  if (ranges.size() != 1 ||
    (ranges[0].columnCount() <= 1 && 
     ranges[0].rowCount() <= 1))
    {
    // no selection or single item selection. Nothing to do.
    return;
    }
  QTableWidgetSelectionRange range = ranges[0];

  QDialog dialog;
  Ui::pqComparativeParameterRangeDialog ui;
  ui.setupUi(&dialog);
  bool csv = this->acceptsMultipleValues();
  ui.multivalueHint->setVisible(csv);

  QRegExp floatNum = QRegExp("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");
  QRegExp csvFloatNum = QRegExp(QString("%1(,%1)*").arg(floatNum.pattern()));
  ui.minValue->setValidator(
    new QRegExpValidator(csv? csvFloatNum: floatNum , ui.minValue));
  ui.maxValue->setValidator(
    new QRegExpValidator(csv? csvFloatNum: floatNum , ui.maxValue));

  if (dialog.exec() != QDialog::Accepted)
    {
    return;
    }


  vtkstd::vector<double> minvalues = ::getValues(ui.minValue->text());
  vtkstd::vector<double> maxvalues = ::getValues(ui.maxValue->text());

  unsigned int numvalues = static_cast<unsigned int>(qMin(minvalues.size(),
      maxvalues.size()));

  if (numvalues == 0)
    {
    return;
    }

  BEGIN_UNDO_SET("Update Parameter Values");

  if (range.rowCount() == 1 &&
    range.columnCount() == this->size().width())
    {
    // user set an x-range.
    this->cue()->UpdateXRange(range.topRow(), &minvalues[0], &maxvalues[0],
      numvalues);
    }
  else if (range.columnCount() == 1 &&
    range.rowCount() == this->size().height())
    {
    // user set a y-range.
    this->cue()->UpdateYRange(range.leftColumn(), 
      &minvalues[0], &maxvalues[0], numvalues);
    }
  else if (range.columnCount() == this->size().width() &&
    range.rowCount() == this->size().height())
    {
    // user set a t-range.
    this->cue()->UpdateWholeRange(&minvalues[0], &maxvalues[0], numvalues);
    }
  else
    {
    // cannot formulate user chose as a range. Set individual values.
    int count = range.rowCount() * range.columnCount() -1;

    for (int x=range.leftColumn(); x <= range.rightColumn(); x++)
      {
      for (int y=range.topRow(); y <= range.bottomRow(); y++)
        {
        for (unsigned int cc=0; cc < numvalues; cc++)
          {
          minvalues[cc] = minvalues[cc] + (y * range.columnCount() + x) * 
            (maxvalues[cc] - minvalues[cc]) / count;
          }
        this->cue()->UpdateValue(x, y, &minvalues[0], numvalues);
        }
      }
    }

  END_UNDO_SET();
  emit this->valuesChanged();

  this->updateGUIOnIdle();
}
