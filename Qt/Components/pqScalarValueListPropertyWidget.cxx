/*=========================================================================

   Program: ParaView
   Module: pqScalarValueListPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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
#include "pqScalarValueListPropertyWidget.h"
#include "ui_pqScalarValueListPropertyWidget.h"

#include <cmath>

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QVector>

#include "pqSMAdaptor.h"
#include "pqCollapsedGroup.h"
#include "pqSampleScalarAddRangeDialog.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMVectorProperty.h"
#include "vtkWeakPointer.h"

namespace
{
class pqTableModel : public QAbstractTableModel
{
  typedef QAbstractTableModel Superclass;
  int NumberOfColumns;
  QVector<QVariant> Values;

  int computeOffset(const QModelIndex& idx) const
    {
    return idx.row() * this->NumberOfColumns + idx.column();
    }
  QModelIndex computeIndex(int offset) const
    {
    return this->index(
      offset / this->NumberOfColumns,
      offset % this->NumberOfColumns);
    }
public:
  pqTableModel(int num_columns, QObject* parentObject=NULL)
    : Superclass(parentObject),
    NumberOfColumns(num_columns)
    {
    Q_ASSERT(num_columns > 0);
    }

  virtual ~pqTableModel()
    {
    }

  // QAbstractTableModel API -------------------------------------------------
  virtual Qt::ItemFlags flags(const QModelIndex &idx) const
    { return this->Superclass::flags(idx) | Qt::ItemIsEditable; }

  virtual int rowCount(const QModelIndex &prnt=QModelIndex()) const
    { Q_UNUSED(prnt); return this->Values.size() / this->NumberOfColumns; }
  virtual int columnCount(const QModelIndex &prnt=QModelIndex()) const
    { Q_UNUSED(prnt); return this->NumberOfColumns; }

  virtual QVariant data(const QModelIndex& idx, int role=Qt::DisplayRole) const
    {
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
      {
      if (idx.isValid() &&
        idx.row() < this->rowCount() &&
        idx.column() < this->columnCount())
        {
        int offset = this->computeOffset(idx);
        return (offset < this->Values.size()?
          this->Values[offset] : QVariant());
        }
      }
    return QVariant();
    }

  virtual bool setData(const QModelIndex &idx, const QVariant &value,
    int role=Qt::EditRole)
    {
    Q_UNUSED(role);
    if (!value.toString().isEmpty())
      {
      int offset = this->computeOffset(idx);
      if (offset >= this->Values.size())
        {
        // we don't need to fire this->beginInsertRows
        // since this typically happens for setting a non-existent
        // column value.
        this->Values.resize(offset+1);
        }
      if (this->Values[offset] != value)
        {
        this->Values[offset] = value;
        emit this->dataChanged(idx, idx);
        }
      }
    return false;
    }

  // API to change table ------------------------------------------------------
  void setValue(const QVariantList& values)
    {
    int old_size = this->Values.size();
    int new_size = values.size();

    int old_row_count = old_size / this->NumberOfColumns;
    int new_row_count = new_size / this->NumberOfColumns;

    if (old_row_count > new_row_count)
      {
      // rows are removed.
      emit this->beginRemoveRows(QModelIndex(),
        new_row_count, old_row_count-1);
      this->Values.resize(new_size);
      emit this->endRemoveRows();
      }
    else if (new_row_count > old_row_count)
      {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(),
        old_row_count, new_row_count-1);
      this->Values.resize(new_size);
      for (int cc=old_size; cc < new_size; cc++)
        {
        this->Values[cc] = values[cc];
        }
      emit this->endInsertRows();
      }

    Q_ASSERT(this->Values.size() == values.size());

    // now check which data has changed.
    for (int cc=0; cc < this->Values.size(); cc++)
      {
      if (this->Values[cc] != values[cc])
        {
        this->Values[cc] = values[cc];
        QModelIndex idx = this->computeIndex(cc);
        emit this->dataChanged(idx, idx);
        }
      }
    }
  const QVector<QVariant> value() const
    {return this->Values;}

  QModelIndex addRow(const QModelIndex& after=QModelIndex())
    {
    int row = after.isValid()? after.row() : (this->rowCount() -1);

    QVariantList copy;
    for (int cc=row*this->NumberOfColumns; cc < (row+1)*this->NumberOfColumns; cc++)
      {
      copy.push_back(
       (cc >=0 && cc < this->Values.size())? this->Values[cc] : QVariant());
      }

    // insert after current row.
    row++;

    emit this->beginInsertRows(QModelIndex(), row, row);
    if (row*this->NumberOfColumns > this->Values.size())
      {
      this->Values.resize(row * this->NumberOfColumns-1);
      }
    for (int cc=0; cc < this->NumberOfColumns; cc++)
      {
      this->Values.insert(row*this->NumberOfColumns + cc, copy[cc]);
      }
    emit this->endInsertRows();
    return this->index(row, 0);
    }

  QModelIndex removeRow(const QModelIndex& toRemove=QModelIndex())
    {
    if (!toRemove.isValid()) { return QModelIndex(); }

    int row = toRemove.row();
    emit this->beginRemoveRows(QModelIndex(), row, row);
    this->Values.remove(row*this->NumberOfColumns, this->NumberOfColumns);
    emit this->endRemoveRows();

    if (row < this->rowCount())
      {
      // since toRemove is still a valid row.
      return toRemove;
      }
    row -= 1;
    if (row >=0 && row < this->rowCount())
      {
      return this->index(row, toRemove.column());
      }
    return QModelIndex();
    }

  void removeAll()
    {
    emit this->beginResetModel();
    this->Values.clear();
    emit this->endResetModel();
    }
};

}

class pqScalarValueListPropertyWidget::pqInternals
{
  QString DefaultText;
public:
  Ui::ScalarValueListPropertyWidget Ui;
  vtkNew<vtkEventQtSlotConnect> VTKRangeConnector;
  vtkWeakPointer<vtkSMDoubleRangeDomain> RangeDomain;
  pqTableModel Model;

  pqInternals(pqScalarValueListPropertyWidget* self, int columnCount)
    : Model(columnCount)
    {
    this->Ui.setupUi(self);
    this->Ui.Table->setModel(&this->Model);
    this->Ui.Table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    this->Ui.Table->horizontalHeader()->setStretchLastSection(true);
    this->Ui.Table->horizontalHeader()->hide();

    this->DefaultText = this->Ui.ScalarRangeLabel->text();
    this->Ui.ScalarRangeLabel->hide();

    // hide add-range button if columnCount > 1 since we don't know how to fill
    // values in that case.
    this->Ui.AddRange->setVisible(columnCount==1);
    }
  ~pqInternals()
    {
    this->VTKRangeConnector->Disconnect();
    }

  void setScalarRangeLabel(double min, double max)
    {
    this->Ui.ScalarRangeLabel->setText(
      this->DefaultText.arg(min).arg(max));
    }

  void clearScalarRangeLabel()
    {
    this->Ui.ScalarRangeLabel->setText(
      this->DefaultText.arg("<unknown>").arg("<unknown>"));
    }
};

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::pqScalarValueListPropertyWidget(
  vtkSMProperty *smProperty, vtkSMProxy *smProxy, QWidget *pWidget)
  : Superclass(smProxy, pWidget)
{
  this->setShowLabel(false);

  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(smProperty);
  Q_ASSERT(vp != NULL);

  this->Internals = new pqInternals(this, vp->GetNumberOfElementsPerCommand());
  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    this, SIGNAL(scalarsChanged()));

  Ui::ScalarValueListPropertyWidget &ui = this->Internals->Ui;
  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(add()));
  QObject::connect(ui.AddRange, SIGNAL(clicked()), this, SLOT(addRange()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(remove()));
  QObject::connect(ui.RemoveAll, SIGNAL(clicked()), this, SLOT(removeAll()));
  QObject::connect(ui.Table, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
}

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::~pqScalarValueListPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setScalars(const QVariantList &values)
{
  this->Internals->Model.setValue(values);
}

//-----------------------------------------------------------------------------
QVariantList pqScalarValueListPropertyWidget::scalars() const
{
  return this->Internals->Model.value().toList();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::add()
{
  QModelIndex idx = this->Internals->Model.addRow(
    this->Internals->Ui.Table->currentIndex());
  this->Internals->Ui.Table->setCurrentIndex(idx);
  this->Internals->Ui.Table->edit(idx);
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::editPastLastRow()
{
  QModelIndex idx = this->Internals->Model.addRow(
    this->Internals->Ui.Table->currentIndex());
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::remove()
{
 QModelIndex idx = this->Internals->Model.removeRow(
    this->Internals->Ui.Table->currentIndex());
  this->Internals->Ui.Table->setCurrentIndex(idx);
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::removeAll()
{
  this->Internals->Model.removeAll();
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::addRange()
{
  double range_min, range_max;
  if(!this->getRange(range_min, range_max))
    {
    range_min=0.0;
    range_max=10.0;
    }

  pqSampleScalarAddRangeDialog dialog(range_min, range_max, 10, false);
  if (dialog.exec() != QDialog::Accepted)
    {
    return;
    }

  const double from = dialog.from();
  const double to = dialog.to();
  const int steps = dialog.steps();
  const bool logarithmic = dialog.logarithmic();

  if (steps < 2 || from == to)
    {
    return;
    }

  QVariantList value = this->Internals->Model.value().toList();

  if (logarithmic)
    {
    const double sign = from < 0 ? -1.0 : 1.0;
    const double log_from = std::log10(std::abs(from ? from : 1.0e-6 * (from - to)));
    const double log_to = std::log10(std::abs(to ? to : 1.0e-6 * (to - from)));

    for (int i = 0; i != steps; i++)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      value.push_back(
        sign * pow(10.0, (1.0 - mix) * log_from + (mix) * log_to));
      }
    }
  else
    {
    for (int i = 0; i != steps; i++)
      {
      const double mix = static_cast<double>(i) / static_cast<double>(steps - 1);
      value.push_back((1.0 - mix) * from + (mix) * to);
      }
    }
  this->Internals->Model.setValue(value);
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setRangeDomain(
  vtkSMDoubleRangeDomain* smRangeDomain)
{
  this->Internals->VTKRangeConnector->Disconnect();
  this->Internals->RangeDomain = smRangeDomain;
  if (smRangeDomain)
    {
    this->Internals->VTKRangeConnector->Connect(
      smRangeDomain, vtkCommand::DomainModifiedEvent,
      this, SLOT(smRangeModified()));
    this->Internals->Ui.ScalarRangeLabel->show();
    this->smRangeModified();
    }
  else
    {
    this->Internals->Ui.ScalarRangeLabel->hide();
    }
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::smRangeModified()
{
  double range_min, range_max;
  if (this->getRange(range_min, range_max))
    {
    this->Internals->setScalarRangeLabel(range_min, range_max);
    }
  else
    {
    this->Internals->clearScalarRangeLabel();
    }
}

//-----------------------------------------------------------------------------
bool pqScalarValueListPropertyWidget::getRange(double& range_min, double& range_max)
{
  // Return the range of values in the input (if available)
  if (this->Internals->RangeDomain)
    {
    int min_exists = 0, max_exists=0;
    range_min = this->Internals->RangeDomain->GetMinimum(0, min_exists);
    range_max = this->Internals->RangeDomain->GetMaximum(0, max_exists);
    return (min_exists && max_exists);
    }
  return false;
}
