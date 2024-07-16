// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqScalarValueListPropertyWidget.h"
#include "ui_pqScalarValueListPropertyWidget.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QVector>

#include "pqCollapsedGroup.h"
#include "pqSMAdaptor.h"
#include "pqSeriesGeneratorDialog.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMTimeStepsDomain.h"
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
  pqTableModel(int num_columns, bool integers_only = false, QObject* parentObject = nullptr)
    : Superclass(parentObject)
    , NumberOfColumns(num_columns)
    , AllowIntegralValuesOnly(integers_only)
  {
    assert(num_columns > 0);
  }

  ~pqTableModel() override = default;

  void setLabels(const std::vector<std::string>& labels)
  {
    this->Labels.resize(static_cast<int>(labels.size()));
    for (int i = 0; i < static_cast<int>(labels.size()); i++)
    {
      this->Labels[i] = QVariant(labels[i].c_str());
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
      Q_EMIT this->dataChanged(idx, idx);
      return true;
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
      Q_EMIT this->beginRemoveRows(QModelIndex(), new_row_count, old_row_count - 1);
      this->Values.resize(new_size);
      Q_EMIT this->endRemoveRows();
    }
    else if (new_row_count > old_row_count)
    {
      // rows are added.
      Q_EMIT this->beginInsertRows(QModelIndex(), old_row_count, new_row_count - 1);
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
      Q_EMIT this->endInsertRows();
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
        Q_EMIT this->dataChanged(idx, idx);
      }
    }
  }
  QVector<QVariant> value() const { return this->Values; }

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

    Q_EMIT this->beginInsertRows(QModelIndex(), row, row);
    if (row * this->NumberOfColumns > this->Values.size())
    {
      this->Values.resize(row * this->NumberOfColumns - 1);
    }
    for (int cc = 0; cc < this->NumberOfColumns; cc++)
    {
      this->Values.insert(row * this->NumberOfColumns + cc, copy[cc]);
    }
    Q_EMIT this->endInsertRows();
    return this->index(row, 0);
  }

  // Given a list of modelindexes, return a vector containing multiple sorted
  // vectors of rows, split by their discontinuity
  void splitSelectedIndexesToRowRanges(
    const QModelIndexList& indexList, QVector<QVector<QVariant>>& result)
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
    std::sort(rows.begin(), rows.end());
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
    QVector<QVector<QVariant>> rowRanges;
    this->splitSelectedIndexesToRowRanges(toRemove, rowRanges);
    int numGroups = static_cast<int>(rowRanges.size());
    for (int g = numGroups - 1; g > -1; --g)
    {
      int numRows = rowRanges.at(g).size();
      int beginRow = rowRanges.at(g).at(0).toInt();
      int endRow = rowRanges.at(g).at(numRows - 1).toInt();
      Q_EMIT this->beginRemoveRows(QModelIndex(), beginRow, endRow);
      for (int r = endRow; r >= beginRow; --r)
      {
        this->Values.remove(r * this->NumberOfColumns, this->NumberOfColumns);
      }
      Q_EMIT this->endRemoveRows();
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
    Q_EMIT this->beginResetModel();
    this->Values.clear();
    Q_EMIT this->endResetModel();
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
    MODE_DOUBLE,
    MODE_TIMESTEPS
  };
  Ui::ScalarValueListPropertyWidget Ui;
  vtkNew<vtkEventQtSlotConnect> VTKRangeConnector;
  vtkWeakPointer<vtkSMDomain> RangeDomain;
  pqTableModel Model;
  ValueMode Mode;
  QPointer<pqSeriesGeneratorDialog> GeneratorDialog;

  pqInternals(pqScalarValueListPropertyWidget* self, int columnCount)
    : Model(columnCount)
    , Mode(MODE_DOUBLE)
  {
    this->Ui.setupUi(self);
    this->Ui.Table->setModel(&this->Model);
    this->Ui.Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
    this->Ui.ScalarRangeLabel->setText(this->DefaultText.arg(QString("<%1>").arg(tr("unknown")))
                                         .arg(QString("<%1>").arg(tr("unknown"))));
  }
};

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::pqScalarValueListPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* pWidget)
  : Superclass(smProxy, pWidget)
{
  this->setShowLabel(false);
  this->setProperty(smProperty);

  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(smProperty);
  assert(vp != nullptr);

  this->Internals = new pqInternals(this, vp->GetNumberOfElementsPerCommand());
  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SIGNAL(scalarsChanged()));

  Ui::ScalarValueListPropertyWidget& ui = this->Internals->Ui;

  // Hide the AddRange button initially. If there is a range domain,
  // this will be added back.
  ui.AddRange->hide();

  auto* hints = smProperty->GetHints();
  const bool showRestoreButton = hints && hints->FindNestedElementByName("AllowRestoreDefaults");
  ui.RestoreDefaults->setVisible(showRestoreButton);

  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(add()));
  QObject::connect(ui.AddRange, SIGNAL(clicked()), this, SLOT(addRange()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(remove()));
  QObject::connect(ui.RemoveAll, SIGNAL(clicked()), this, SLOT(removeAll()));
  QObject::connect(ui.Table, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
  QObject::connect(ui.RestoreDefaults, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

  // update `Remove` button enabled state based on selection.
  ui.Remove->setEnabled(false);
  QObject::connect(ui.Table->selectionModel(), &QItemSelectionModel::selectionChanged,
    [&ui](const QItemSelection&, const QItemSelection&) {
      ui.Remove->setEnabled(ui.Table->selectionModel()->selectedIndexes().empty() == false);
    });

  if (smProperty->GetInformationOnly())
  {
    ui.Add->hide();
    ui.AddRange->hide();
    ui.Remove->hide();
    ui.RemoveAll->hide();
  }
}

//-----------------------------------------------------------------------------
pqScalarValueListPropertyWidget::~pqScalarValueListPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
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
void pqScalarValueListPropertyWidget::setLabels(const std::vector<std::string>& labels)
{
  this->Internals->Model.setLabels(labels);
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::add()
{
  QModelIndex idx = this->Internals->Model.addRow(this->Internals->Ui.Table->currentIndex());
  this->Internals->Ui.Table->setCurrentIndex(idx);
  this->Internals->Ui.Table->edit(idx);
  Q_EMIT this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::editPastLastRow()
{
  this->Internals->Model.addRow(this->Internals->Ui.Table->currentIndex());
  Q_EMIT this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::remove()
{
  QModelIndexList indexes = this->Internals->Ui.Table->selectionModel()->selectedIndexes();
  if (indexes.empty())
  {
    // Nothing selected. Nothing to remove
    return;
  }
  QModelIndex idx = this->Internals->Model.removeListedRows(indexes);
  this->Internals->Ui.Table->setCurrentIndex(idx);
  Q_EMIT this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::removeAll()
{
  auto& internals = (*this->Internals);
  internals.Ui.Table->selectionModel()->clear();
  internals.Model.removeAll();
  Q_EMIT this->scalarsChanged();
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::addRange()
{
  switch (this->Internals->Mode)
  {
    case pqInternals::MODE_DOUBLE:
    case pqInternals::MODE_TIMESTEPS:
    {
      double rangeMin, rangeMax;
      if (!this->getRange(rangeMin, rangeMax))
      {
        rangeMin = 0.0;
        rangeMax = 9.0;
      }

      if (!this->Internals->GeneratorDialog)
      {
        this->Internals->GeneratorDialog = new pqSeriesGeneratorDialog(rangeMin, rangeMax, this);
      }
      else
      {
        this->Internals->GeneratorDialog->setDataRange(rangeMin, rangeMax);
      }

      if (this->Internals->GeneratorDialog->exec() != QDialog::Accepted)
      {
        return;
      }

      QVariantList value = this->Internals->Model.value().toList();
      for (const auto& newvalue : this->Internals->GeneratorDialog->series())
      {
        value.push_back(QVariant(newvalue));
      }
      this->Internals->Model.setValue(value);
      Q_EMIT this->scalarsChanged();
    }
    break;
    case pqInternals::MODE_INT:
    {
      int rangeMin, rangeMax;
      if (!this->getRange(rangeMin, rangeMax))
      {
        rangeMin = 0;
        rangeMax = 10;
      }

      if (!this->Internals->GeneratorDialog)
      {
        this->Internals->GeneratorDialog = new pqSeriesGeneratorDialog(rangeMin, rangeMax, this);
      }
      else
      {
        this->Internals->GeneratorDialog->setDataRange(rangeMin, rangeMax);
      }

      if (this->Internals->GeneratorDialog->exec() != QDialog::Accepted)
      {
        return;
      }

      QVariantList intRange;
      for (const auto& newvalue : this->Internals->GeneratorDialog->series())
      {
        const int ival = static_cast<int>(std::floor(newvalue + 0.5));
        if (intRange.empty() || (intRange.back().toInt() != ival))
        {
          intRange.push_back(ival);
        }
      }

      QVariantList value = this->Internals->Model.value().toList();
      value += intRange;
      this->Internals->Model.setValue(value);
      Q_EMIT this->scalarsChanged();
    }
    break;
    default:
      break;
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
void pqScalarValueListPropertyWidget::setRangeDomain(vtkSMTimeStepsDomain* tsDomain)
{
  this->Internals->VTKRangeConnector->Disconnect();
  this->Internals->RangeDomain = tsDomain;
  this->Internals->Mode = pqInternals::MODE_TIMESTEPS;
  this->Internals->Model.setAllowIntegerValuesOnly(false);
  if (tsDomain)
  {
    this->Internals->VTKRangeConnector->Connect(
      tsDomain, vtkCommand::DomainModifiedEvent, this, SLOT(smRangeModified()));
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
  switch (this->Internals->Mode)
  {
    case pqInternals::MODE_DOUBLE:
    case pqInternals::MODE_TIMESTEPS:
    {
      double rangeMin, rangeMax;
      if (this->getRange(rangeMin, rangeMax))
      {
        this->Internals->setScalarRangeLabel(rangeMin, rangeMax);
      }
      else
      {
        this->Internals->clearScalarRangeLabel();
      }
    }
    break;
    case pqInternals::MODE_INT:
    {
      int rangeMin, rangeMax;
      if (this->getRange(rangeMin, rangeMax))
      {
        this->Internals->setScalarRangeLabel(rangeMin, rangeMax);
      }
      else
      {
        this->Internals->clearScalarRangeLabel();
      }
    }
    break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
bool pqScalarValueListPropertyWidget::getRange(double& rangeMin, double& rangeMax)
{
  // Return the range of values in the input (if available)
  if (this->Internals->RangeDomain)
  {
    if (this->Internals->Mode == pqInternals::MODE_DOUBLE)
    {
      int min_exists = 0, max_exists = 0;
      vtkSMDoubleRangeDomain* doubleRange =
        vtkSMDoubleRangeDomain::SafeDownCast(this->Internals->RangeDomain);
      assert(doubleRange != nullptr);
      rangeMin = doubleRange->GetMinimum(0, min_exists);
      rangeMax = doubleRange->GetMaximum(0, max_exists);
      return (min_exists && max_exists);
    }
    else // this->Internals->Mode == pqInternals::MODE_TIMESTEPS
    {
      // Timesteps are sorted in vtkSMTimeStepsDomain
      vtkSMTimeStepsDomain* tsDomain =
        vtkSMTimeStepsDomain::SafeDownCast(this->Internals->RangeDomain);
      auto values = tsDomain->GetValues();
      if (values.size() >= 2)
      {
        rangeMin = values.front();
        rangeMax = values.back();
        return true;
      }
      else
      {
        return false;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqScalarValueListPropertyWidget::getRange(int& rangeMin, int& rangeMax)
{
  assert(this->Internals->Mode == pqInternals::MODE_INT);
  // Return the range of values in the input (if available)
  if (this->Internals->RangeDomain)
  {
    int min_exists = 0, max_exists = 0;
    vtkSMIntRangeDomain* doubleRange =
      vtkSMIntRangeDomain::SafeDownCast(this->Internals->RangeDomain);
    assert(doubleRange != nullptr);
    rangeMin = doubleRange->GetMinimum(0, min_exists);
    rangeMax = doubleRange->GetMaximum(0, max_exists);
    return (min_exists && max_exists);
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqScalarValueListPropertyWidget::restoreDefaults()
{
  this->property()->ResetToXMLDefaults();
  Q_EMIT this->scalarsChanged();
}
