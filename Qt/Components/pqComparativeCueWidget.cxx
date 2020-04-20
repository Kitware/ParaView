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
#include "vtkPVComparativeAnimationCue.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMPropertyHelper.h"

#include <cassert>
#include <vector>

namespace
{
class pqLock
{
  bool* Var;
  bool Prev;

public:
  pqLock(bool* var, bool val)
  {
    this->Var = var;
    this->Prev = *this->Var;
    *this->Var = val;
  }
  ~pqLock() { *this->Var = this->Prev; }
};

std::vector<double> getValues(const QString& str)
{
  std::vector<double> values;
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
  QObject::connect(&this->IdleUpdateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
  QObject::connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
  QObject::connect(this, SIGNAL(cellChanged(int, int)), this, SLOT(onCellChanged(int, int)));
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
void pqComparativeCueWidget::setCue(vtkSMProxy* _cue)
{
  if (this->Cue.GetPointer() == _cue)
  {
    return;
  }
  this->VTKConnect->Disconnect();
  this->Cue = vtkSMComparativeAnimationCueProxy::SafeDownCast(_cue);
  if (this->Cue)
  {
    this->VTKConnect->Connect(this->Cue, vtkCommand::ModifiedEvent, this, SLOT(updateGUIOnIdle()));
    this->VTKConnect->Connect(
      this->Cue, vtkCommand::PropertyModifiedEvent, this, SLOT(updateGUIOnIdle()));
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
  return (this->Cue && vtkSMPropertyHelper(this->Cue, "AnimatedElement").GetAsInt() == -1);
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
  for (int cc = 0; cc < rows; cc++)
  {
    vlabels.push_back(QString("%1").arg(cc));
  }
  this->setVerticalHeaderLabels(vlabels);

  for (int cc = 0; cc < cols; cc++)
  {
    char a = 'A';
    a += cc;
    hlabels.push_back(QString::fromLocal8Bit(&a, 1));
  }
  this->setHorizontalHeaderLabels(hlabels);

  vtkSMComparativeAnimationCueProxy* acueProxy = this->cue();
  if (!acueProxy)
  {
    return;
  }

  for (int colno = 0; colno < cols; colno++)
  {
    for (int rowno = 0; rowno < rows; rowno++)
    {
      QTableWidgetItem* tableitem = new QTableWidgetItem();

      unsigned int numvalues = 0;
      double* values = acueProxy->GetValues(colno, rowno, cols, rows, numvalues);
      if (numvalues >= 1)
      {
        QStringList val_list;
        for (unsigned int cc = 0; cc < numvalues; cc++)
        {
          val_list.push_back(QString("%1").arg(values[cc]));
        }
        tableitem->setText(val_list.join(","));
      }
      else
      {
        tableitem->setText("");
      }
      this->setItem(rowno, colno, tableitem);
    }
  }
}

//-----------------------------------------------------------------------------
void pqComparativeCueWidget::onCellChanged(int rowno, int colno)
{
  if (this->InUpdateGUI)
  {
    return;
  }
  BEGIN_UNDO_SET("Parameter Changed");
  QString text = this->item(rowno, colno)->text();
  if (this->acceptsMultipleValues())
  {
    QStringList parts = text.split(',', QString::SkipEmptyParts);
    if (parts.size() > 0)
    {
      double* newvalues = new double[parts.size()];
      double* ptr = newvalues;
      foreach (QString part, parts)
      {
        *ptr = QVariant(part).toDouble();
        ptr++;
      }
      this->cue()->UpdateValue(colno, rowno, newvalues, static_cast<unsigned int>(parts.size()));
    }
  }
  else
  {
    double item_data = QVariant(text).toDouble();
    this->cue()->UpdateValue(colno, rowno, item_data);
  }
  END_UNDO_SET();

  Q_EMIT this->valuesChanged();
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
  if (ranges.size() != 1 || (ranges[0].columnCount() <= 1 && ranges[0].rowCount() <= 1))
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
  ui.mode->setVisible(ranges[0].rowCount() > 1 && ranges[0].columnCount() > 1);

  QRegExp floatNum = QRegExp("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");
  QRegExp csvFloatNum = QRegExp(QString("%1(,%1)*").arg(floatNum.pattern()));
  ui.minValue->setValidator(new QRegExpValidator(csv ? csvFloatNum : floatNum, ui.minValue));
  ui.maxValue->setValidator(new QRegExpValidator(csv ? csvFloatNum : floatNum, ui.maxValue));

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  int parameter_change_mode = ui.mode->currentIndex();
  enum
  {
    HORZ_FIRST,
    VERT_FIRST,
    HORZ_ONLY,
    VERT_ONLY
  };

  std::vector<double> minvalues = ::getValues(ui.minValue->text());
  std::vector<double> maxvalues = ::getValues(ui.maxValue->text());

  unsigned int numvalues = static_cast<unsigned int>(qMin(minvalues.size(), maxvalues.size()));

  if (numvalues == 0)
  {
    return;
  }

  BEGIN_UNDO_SET("Update Parameter Values");

  vtkSMComparativeAnimationCueProxy* acueProxy = this->cue();

  if (range.rowCount() == 1 && range.columnCount() == this->size().width())
  {
    // user set an x-range.
    acueProxy->UpdateXRange(range.topRow(), &minvalues[0], &maxvalues[0], numvalues);
  }
  else if (range.columnCount() == 1 && range.rowCount() == this->size().height())
  {
    // user set a y-range.
    acueProxy->UpdateYRange(range.leftColumn(), &minvalues[0], &maxvalues[0], numvalues);
  }
  else if (range.columnCount() == this->size().width() && range.rowCount() == this->size().height())
  {
    // full range was covered.
    switch (parameter_change_mode)
    {
      case HORZ_FIRST:
        // user set a t-range.
        acueProxy->UpdateWholeRange(&minvalues[0], &maxvalues[0], numvalues);
        break;

      case VERT_FIRST:
        acueProxy->UpdateWholeRange(&minvalues[0], &maxvalues[0], numvalues, true);
        break;

      case HORZ_ONLY:
        acueProxy->UpdateXRange(-1, &minvalues[0], &maxvalues[0], numvalues);
        break;

      case VERT_ONLY:
        acueProxy->UpdateYRange(-1, &minvalues[0], &maxvalues[0], numvalues);
        break;

      default:
        qCritical("Invalid selection");
    }
  }
  else
  {
    // cannot formulate user chose as a range. Set individual values.
    int count = range.rowCount() * range.columnCount() - 1;
    std::vector<double> newvalues;
    newvalues.resize(minvalues.size(), 0.0);
    for (int xx = range.leftColumn(); xx <= range.rightColumn(); xx++)
    {
      for (int yy = range.topRow(); yy <= range.bottomRow(); yy++)
      {
        for (unsigned int cc = 0; cc < numvalues; cc++)
        {
          double scale_factor = 1.0;
          switch (parameter_change_mode)
          {
            case HORZ_FIRST:
              scale_factor = (yy * range.columnCount() + xx) * 1.0 / count;
              break;

            case VERT_FIRST:
              scale_factor = (xx * range.rowCount() + yy) * 1.0 / count;
              break;

            case HORZ_ONLY:
              assert(range.columnCount() > 1);
              scale_factor = xx * 1.0 / (range.columnCount() - 1);
              break;

            case VERT_ONLY:
              assert(range.rowCount() > 1);
              scale_factor = yy * 1.0 / (range.rowCount() - 1);
              break;
            default:
              qCritical("Invalid selection");
          }
          newvalues[cc] = minvalues[cc] + scale_factor * (maxvalues[cc] - minvalues[cc]);
        }
        acueProxy->UpdateValue(xx, yy, &newvalues[0], numvalues);
      }
    }
  }

  END_UNDO_SET();
  Q_EMIT this->valuesChanged();

  this->updateGUIOnIdle();
}
