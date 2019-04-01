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

#include <cassert>
#include <cmath>

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QVector>

#include "pqCollapsedGroup.h"
#include "pqSMAdaptor.h"
#include "pqSampleScalarAddRangeDialog.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMIntRangeDomain.h"
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
  QVector<QVariant> Labels;
  bool AllowIntegralValuesOnly;

  int computeOffset(const QModelIndex& idx) const
  {
    return idx.row() * this->NumberOfColumns + idx.column();
  }
  QModelIndex computeIndex(int offset) const
  {
    return this->index(offset / this->NumberOfColumns, offset % this->NumberOfColumns);
  }

public:
  pqTableModel(int num_columns, bool integers_only = false, QObject* parentObject = NULL)
    : Superclass(parentObject)
    , NumberOfColumns(num_columns)
    , AllowIntegralValuesOnly(integers_only)
  {
    assert(num_columns > 0);
  }

  ~pqTableModel() override {}

  void setLabels(std::vector<const char*>& labels)
  {
    this->Labels.resize(static_cast<int>(labels.size()));
    for (int i = 0; i < static_cast<int>(labels.size()); i++)
    {
      this->Labels[i] = QVariant(labels[i]);
    }
  }

  void setAllowIntegerValuesOnly(bool allow) { this->AllowIntegralValuesOnly = allow; }

  // QAbstractTableModel API -------------------------------------------------
  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    return this->Superclass::flags(idx) | Qt::ItemIsEditable;
  }

  int rowCount(const QModelIndex& prnt = QModelIndex()) const override
  {
    Q_UNUSED(prnt);
    return this->Values.size() / this->NumberOfColumns;
  }
  int columnCount(const QModelIndex& prnt = QModelIndex()) const override
  {
    Q_UNUSED(prnt);
    return this->NumberOfColumns;
  }

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (section < this->Labels.size() && orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      return this->Labels[section];
    }

    return this->Superclass::headerData(section, orientation, role);
  }

  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override
  {
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::EditRole)
    {
      if (idx.isValid() && idx.row() < this->rowCount() && idx.column() < this->columnCount())
      {
        int offset = this->computeOffset(idx);
        return (offset < this->Values.size() ? this->Values[offset].toString() : QVariant());
      }
    }
    return QVariant();
  }

  bool setData(const QModelIndex& idx, const QVariant& aValue, int role = Qt::EditRole) override
  {
    Q_UNUSED(role);
    if (!aValue.toString().isEmpty())
    {
      int offset = this->computeOffset(idx);
      if (offset >= this->Values.size())
      {
        // we don't need to fire this->beginInsertRows
        // since this typically happens for setting a non-existent
        // column value.
        this->Values.resize(offset + 1);
      }
      if (this->Values[offset] != aValue)
      {
        if (this->AllowIntegralValuesOnly)
        {
          this->Values[offset] = aValue.toInt();
        }
        else
        {
          this->Values[offset] = aValue;
        }
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
      emit this->beginRemoveRows(QModelIndex(), new_row_count, old_row_count - 1);
      this->Values.resize(new_size);
      emit this->endRemoveRows();
    }
    else if (new_row_count > old_row_count)
    {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(), old_row_count, new_row_count - 1);
      this->Values.resize(new_size);
      for (int cc = old_size; cc < new_size; cc++)
      {
        if (this->AllowIntegralValuesOnly)
        {
          this->Values[cc] = values[cc].toInt();
        }
        else
        {
          this->Values[cc] = values[cc];
        }
      }
      emit this->endInsertRows();
    }

    assert(this->Values.size() == values.size());

    // now check which data has changed.
    for (int cc = 0; cc < this->Values.size(); cc++)
    {
      if (this->Values[cc] != values[cc])
      {
        if (this->AllowIntegralValuesOnly)
        {
          this->Values[cc] = values[cc].toInt();
        }
        else
        {
          this->Values[cc] = values[cc];
        }
        QModelIndex idx = this->computeIndex(cc);
        emit this->dataChanged(idx, idx);
      }
    }
  }
  const QVector<QVariant> value() const { return this->Values; }

  QModelIndex addRow(const QModelIndex& after = QModelIndex())
  {
    int row = after.isValid() ? after.row() : (this->rowCount() - 1);

    QVariantList copy;
    for (int cc = row * this->NumberOfColumns; cc < (row + 1) * this->NumberOfColumns; cc++)
    {
      copy.push_back((cc >= 0 && cc < this->Values.size()) ? this->Values[cc] : QVariant());
    }

    // insert after current row.
    row++;

    emit this->beginInsertRows(QModelIndex(), row, row);
    if (row * this->NumberOfColumns > this->Values.size())
    {
      this->Values.resize(row * this->NumberOfColumns - 1);
    }
    for (int cc = 0; cc < this->NumberOfColumns; cc++)
    {
      this->Values.insert(row * this->NumberOfColumns + cc, copy[cc]);
    }
    emit this->endInsertRows();
    return this->index(row, 0);
  }

  // Given a list of modelindexes, return a vector containing multiple sorted
  // vectors of rows, split by their discontinuity
  void splitSelectedIndexesToRowRanges(
    const QModelIndexList& indexList, QVector<QVector<QVariant> >& result)
  {
    if (indexList.empty())
    {
      return;
    }
    result.clear();
    QVector<int> rows;
    QModelIndexList::const_iterator iter = indexList.begin();
    for (; iter != indexList.end(); ++iter)
    {
      if ((*iter).isValid())
      {
        rows.push_back((*iter).row());
      }
    }
    qSort(rows.begin(), rows.end());
    result.resize(1);
    result[0].push_back(rows[0]);
    for (int i = 1; i < rows.size(); ++i)
    {
      if (rows[i] == rows[i - 1])
      {
        // avoid duplicate
        continue;
      }
      if (rows[i] != rows[i - 1] + 1)
      {
        result.push_back(QVector<QVariant>());
      }
      result.back().push_back(QVariant(rows[i]));
    }
  }

  // Remove the given rows. Returns item before or after the removed
  // item, if any.
  QModelIndex removeListedRows(const QModelIndexList& toRemove = QModelIndexList())
  {
    QVector<QVector<QVariant> > rowRanges;
    this->splitSelectedIndexesToRowRanges(toRemove, rowRanges);
    int numGroups = static_cast<int>(rowRanges.size());
    for (int g = numGroups - 1; g > -1; --g)
    {
      int numRows = rowRanges.at(g).size();
      int beginRow = rowRanges.at(g).at(0).toInt();
      int endRow = rowRanges.at(g).at(numRows - 1).toInt();
      emit this->beginRemoveRows(QModelIndex(), beginRow, endRow);
      for (int r = endRow; r >= beginRow; --r)
      {
        this->Values.remove(r * this->NumberOfColumns, this->NumberOfColumns);
      }
      emit this->endRemoveRows();
    }

    int firstRow = rowRanges.at(0).at(0).toInt();
    int rowsCount = this->rowCount();
    if (firstRow < rowsCount)
    {
      // since firstRow is still a valid row.
      return this->index(firstRow, toRemove.at(0).column());
    }
    else if (rowsCount > 0 && (firstRow > (rowsCount - 1)))
    {
      // just return the index for last row.
      return this->index(rowsCount - 1, toRemove.at(0).column());
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
  enum ValueMode
  {
    MODE_INT,
    MODE_DOUBLE
  };
  Ui::ScalarValueListPropertyWidget Ui;
  vtkNew<vtkEventQtSlotConnect> VTKRangeConnector;
  vtkWeakPointer<vtkSMDomain> RangeDomain;
  pqTableModel Model;
  ValueMode Mode;

  pqInternals(pqScalarValueListPropertyWidget* self, int columnCount)
    : Model(columnCount)
    , Mode(MODE_DOUBLE)
  {
    this->Ui.setupUi(self);
    this->Ui.Table->setModel(&this->Model);
#if QT_VERSION >= 0x050000
    this->Ui.Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    this->Ui.Table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    this->Ui.Table->horizontalHeader()->setStretchLastSection(true);
    this->Ui.Table->horizontalHeader()->hide();

    this->DefaultText = this->Ui.ScalarRangeLabel->text();
    this->Ui.ScalarRangeLabel->hide();

    // hide add-range button if columnCount > 1 since we don't know how to fill
    // values in that case.
    this->Ui.AddRange->setVisible(columnCount == 1);
  }
  ~pqInternals() { this->VTKRangeConnector->Disconnect(); }

  void setScalarRangeLabel(double min, double max)
  {
    this->Ui.ScalarRangeLabel->setText(this->DefaultText.arg(min).arg(max));
  }
  void setScalarRangeLabel(int min, int max)
  {
    this->Ui.ScalarRangeLabel->setText(this->DefaultText.arg(min).arg(max));
  }

  void clearScalarRangeLabel()
  {
    this->Ui.ScalarRangeLabel->setText(this->DefaultText.arg("<unknown>").arg("<unknown>"));
  }
};

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::pqScalarValueListPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* pWidget)
  : Superclass(smProxy, pWidget)
{
  this->setShowLabel(false);

  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(smProperty);
  assert(vp != NULL);

  this->Internals = new pqInternals(this, vp->GetNumberOfElementsPerCommand());
  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SIGNAL(scalarsChanged()));

  Ui::ScalarValueListPropertyWidget& ui = this->Internals->Ui;

  // Hide the AddRange button initially. If there is a range domain,
  // this will be added back.
  ui.AddRange->hide();

  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(add()));
  QObject::connect(ui.AddRange, SIGNAL(clicked()), this, SLOT(addRange()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(remove()));
  QObject::connect(ui.RemoveAll, SIGNAL(clicked()), this, SLOT(removeAll()));
  QObject::connect(ui.Table, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));

  // update `Remove` button enabled state based on selection.
  ui.Remove->setEnabled(false);
  QObject::connect(ui.Table->selectionModel(), &QItemSelectionModel::selectionChanged,
    [&ui](const QItemSelection&, const QItemSelection&) {
      ui.Remove->setEnabled(ui.Table->selectionModel()->selectedIndexes().size() > 0);
    });
}

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::~pqScalarValueListPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setScalars(const QVariantList& values)
{
  this->Internals->Model.setValue(values);
}

//-----------------------------------------------------------------------------
QVariantList pqScalarValueListPropertyWidget::scalars() const
{
  return this->Internals->Model.value().toList();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setShowLabels(bool showLabels)
{
  this->Internals->Ui.Table->horizontalHeader()->setVisible(showLabels);
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setLabels(std::vector<const char*>& labels)
{
  this->Internals->Model.setLabels(labels);
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::add()
{
  QModelIndex idx = this->Internals->Model.addRow(this->Internals->Ui.Table->currentIndex());
  this->Internals->Ui.Table->setCurrentIndex(idx);
  this->Internals->Ui.Table->edit(idx);
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::editPastLastRow()
{
  this->Internals->Model.addRow(this->Internals->Ui.Table->currentIndex());
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::remove()
{
  QModelIndexList indexes = this->Internals->Ui.Table->selectionModel()->selectedIndexes();
  if (indexes.size() == 0)
  {
    // Nothing selected. Nothing to remove
    return;
  }
  QModelIndex idx = this->Internals->Model.removeListedRows(indexes);
  this->Internals->Ui.Table->setCurrentIndex(idx);
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::removeAll()
{
  auto& internals = (*this->Internals);
  internals.Ui.Table->selectionModel()->clear();
  internals.Model.removeAll();
  emit this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::addRange()
{
  if (this->Internals->Mode == pqInternals::MODE_DOUBLE)
  {
    double range_min, range_max;
    if (!this->getRange(range_min, range_max))
    {
      range_min = 0.0;
      range_max = 10.0;
    }

    pqSampleScalarAddRangeDialog dialog(range_min, range_max, 10, false);
    if (dialog.exec() != QDialog::Accepted)
    {
      return;
    }

    QVariantList value = this->Internals->Model.value().toList();
    value += dialog.getRange();

    this->Internals->Model.setValue(value);
    emit this->scalarsChanged();
  }
  else if (this->Internals->Mode == pqInternals::MODE_INT)
  {
    int range_min, range_max;
    if (!this->getRange(range_min, range_max))
    {
      range_min = 0;
      range_max = 10;
    }

    pqSampleScalarAddRangeDialog dialog(range_min, range_max, 10, false);
    if (dialog.exec() != QDialog::Accepted)
    {
      return;
    }

    QVariantList range = dialog.getRange();
    QVariantList intRange;
    for (QVariantList::iterator i = range.begin(); i != range.end(); ++i)
    {
      double val = i->toDouble();
      int ival =
        static_cast<int>(((val - std::floor(val)) < 0.5) ? std::floor(val) : std::ceil(val));
      if (intRange.empty() || (intRange.back().toInt() != ival))
      {
        intRange.push_back(ival);
      }
    }

    QVariantList value = this->Internals->Model.value().toList();
    value += intRange;

    this->Internals->Model.setValue(value);
    emit this->scalarsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setRangeDomain(vtkSMDoubleRangeDomain* smRangeDomain)
{
  this->Internals->VTKRangeConnector->Disconnect();
  this->Internals->RangeDomain = smRangeDomain;
  this->Internals->Mode = pqInternals::MODE_DOUBLE;
  this->Internals->Model.setAllowIntegerValuesOnly(false);
  if (smRangeDomain && (smRangeDomain->GetMinimumExists(0) || smRangeDomain->GetMinimumExists(0)))
  {
    this->Internals->VTKRangeConnector->Connect(
      smRangeDomain, vtkCommand::DomainModifiedEvent, this, SLOT(smRangeModified()));
    this->Internals->Ui.ScalarRangeLabel->show();
    this->Internals->Ui.AddRange->show();
    this->smRangeModified();
  }
  else
  {
    this->Internals->Ui.ScalarRangeLabel->hide();
    this->Internals->Ui.AddRange->hide();
  }
}
//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::setRangeDomain(vtkSMIntRangeDomain* smRangeDomain)
{
  this->Internals->VTKRangeConnector->Disconnect();
  this->Internals->RangeDomain = smRangeDomain;
  this->Internals->Mode = pqInternals::MODE_INT;
  this->Internals->Model.setAllowIntegerValuesOnly(true);
  if (smRangeDomain && (smRangeDomain->GetMinimumExists(0) || smRangeDomain->GetMinimumExists(0)))
  {
    this->Internals->VTKRangeConnector->Connect(
      smRangeDomain, vtkCommand::DomainModifiedEvent, this, SLOT(smRangeModified()));
    this->Internals->Ui.ScalarRangeLabel->show();
    this->Internals->Ui.AddRange->show();
    this->smRangeModified();
  }
  else
  {
    this->Internals->Ui.ScalarRangeLabel->hide();
    this->Internals->Ui.AddRange->hide();
  }
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::smRangeModified()
{
  if (this->Internals->Mode == pqInternals::MODE_DOUBLE)
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
  else if (this->Internals->Mode == pqInternals::MODE_INT)
  {
    int range_min, range_max;
    if (this->getRange(range_min, range_max))
    {
      this->Internals->setScalarRangeLabel(range_min, range_max);
    }
    else
    {
      this->Internals->clearScalarRangeLabel();
    }
  }
}

//-----------------------------------------------------------------------------
bool pqScalarValueListPropertyWidget::getRange(double& range_min, double& range_max)
{
  assert(this->Internals->Mode == pqInternals::MODE_DOUBLE);
  // Return the range of values in the input (if available)
  if (this->Internals->RangeDomain)
  {
    int min_exists = 0, max_exists = 0;
    vtkSMDoubleRangeDomain* doubleRange =
      vtkSMDoubleRangeDomain::SafeDownCast(this->Internals->RangeDomain);
    assert(doubleRange != NULL);
    range_min = doubleRange->GetMinimum(0, min_exists);
    range_max = doubleRange->GetMaximum(0, max_exists);
    return (min_exists && max_exists);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqScalarValueListPropertyWidget::getRange(int& range_min, int& range_max)
{
  assert(this->Internals->Mode == pqInternals::MODE_INT);
  // Return the range of values in the input (if available)
  if (this->Internals->RangeDomain)
  {
    int min_exists = 0, max_exists = 0;
    vtkSMIntRangeDomain* doubleRange =
      vtkSMIntRangeDomain::SafeDownCast(this->Internals->RangeDomain);
    assert(doubleRange != NULL);
    range_min = doubleRange->GetMinimum(0, min_exists);
    range_max = doubleRange->GetMaximum(0, max_exists);
    return (min_exists && max_exists);
  }
  return false;
}
