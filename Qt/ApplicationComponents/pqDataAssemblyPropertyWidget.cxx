/*=========================================================================

   Program: ParaView
   Module:  pqDataAssemblyPropertyWidget.cxx

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
#include "pqDataAssemblyPropertyWidget.h"
#include "ui_pqDataAssemblyPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqPropertyLinks.h"
#include "pqTreeViewExpandState.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkSMDataAssemblyDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

#include <QIdentityProxyModel>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QVBoxLayout>

#include <algorithm>
#include <set>

namespace
{

/**
 * Specialization to ensure the header checkstate/label etc. reflects the root-node.
 */
class pqDAPModel : public QIdentityProxyModel
{
public:
  pqDAPModel(QObject* prnt)
    : QIdentityProxyModel(prnt)
  {
    QObject::connect(this, &QAbstractItemModel::dataChanged,
      [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&) {
        if (topLeft.row() == 0 && !topLeft.parent().isValid())
        {
          Q_EMIT this->headerDataChanged(Qt::Horizontal, topLeft.column(), topLeft.column());
        }
      });
  }

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (orientation == Qt::Horizontal)
    {
      switch (role)
      {
        case Qt::TextAlignmentRole:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        default:
          return this->data(this->index(0, section), role);
      }
    }
    return this->QIdentityProxyModel::headerData(section, orientation, role);
  }

  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::EditRole) override
  {
    if (section == 0 && orientation == Qt::Horizontal)
    {
      return this->setData(this->index(0, section), value, role);
    }
    return this->QIdentityProxyModel::setHeaderData(section, orientation, value, role);
  }

private:
  Q_DISABLE_COPY(pqDAPModel);
};

/**
 * A quick way to support use of pqDataAssemblyPropertyWidget for single
 * properties or property groups. When single property, it's treated as a group
 * with just 1 property for function chosenPaths.
 */
vtkSmartPointer<vtkSMPropertyGroup> createGroup(vtkSMProperty* property)
{
  vtkNew<vtkSMPropertyGroup> group;
  group->AddProperty("chosenPaths", property);
  return group;
}
}

class pqDataAssemblyPropertyWidget::pqInternals
{
public:
  Ui::DataAssemblyPropertyWidget Ui;
  QPointer<pqDataAssemblyTreeModel> AssemblyTreeModel;
  QPointer<QStringListModel> StringListModel;
  QStringList ChosenPaths;
  bool BlockUpdates = false;
};

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::pqDataAssemblyPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : pqDataAssemblyPropertyWidget(smproxy, ::createGroup(smproperty), parentObject)
{
}

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::pqDataAssemblyPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqDataAssemblyPropertyWidget::pqInternals())
{
  this->setShowLabel(false);

  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.AssemblyTreeModel = new pqDataAssemblyTreeModel(this);

  // Setup proxy model to add check-state support in the header.
  auto dapmodel = new pqDAPModel(this);
  dapmodel->setSourceModel(internals.AssemblyTreeModel);

  // Add a sort-filter model to enable sorting and filtering of item in the
  // tree.
  auto sortmodel = new QSortFilterProxyModel(this);
  sortmodel->setSourceModel(dapmodel);
  sortmodel->setRecursiveFilteringEnabled(true);
  internals.Ui.tree->setModel(sortmodel);

  internals.StringListModel = new QStringListModel(this);
  internals.Ui.table->setModel(internals.StringListModel);

  // change pqTreeView header.
  internals.Ui.tree->setupCustomHeader(/*use_pqHeaderView=*/true);
  new pqTreeViewSelectionHelper(internals.Ui.tree);

  // hookup add button
  QObject::connect(internals.Ui.add, &QAbstractButton::clicked, [this](bool) {
    auto& iinternals = (*this->Internals);
    auto currentIndex = iinternals.Ui.table->currentIndex();
    const int pos =
      (currentIndex.isValid() ? (currentIndex.row() + 1) : iinternals.StringListModel->rowCount());
    if (iinternals.StringListModel->insertRows(pos, 1))
    {
      currentIndex = iinternals.StringListModel->index(pos, 0);
      iinternals.Ui.table->setCurrentIndex(currentIndex);
      iinternals.Ui.table->edit(currentIndex);
    }
  });

  // enabled-state for remove button.
  QObject::connect(internals.Ui.table->selectionModel(), &QItemSelectionModel::selectionChanged,
    [this](const QItemSelection&, const QItemSelection&) {
      auto& iinternals = (*this->Internals);
      iinternals.Ui.remove->setEnabled(iinternals.Ui.table->selectionModel()->hasSelection());
    });

  // hookup remove button.
  QObject::connect(internals.Ui.remove, &QAbstractButton::clicked, [this](bool) {
    auto& iinternals = (*this->Internals);
    auto rowIndexes = iinternals.Ui.table->selectionModel()->selectedRows();
    std::set<int> rows;
    std::transform(rowIndexes.begin(), rowIndexes.end(), std::inserter(rows, rows.end()),
      [](const QModelIndex& idx) { return idx.row(); });
    for (auto iter = rows.rbegin(); iter != rows.rend(); ++iter)
    {
      iinternals.StringListModel->removeRow(*iter);
    }
  });

  // hookup removeAll button.
  QObject::connect(internals.Ui.removeAll, &QAbstractButton::clicked, [this](bool) {
    auto& iinternals = (*this->Internals);
    iinternals.StringListModel->setStringList(QStringList());
  });

  if (auto smproperty = smgroup->GetProperty("chosenPaths"))
  {
    internals.AssemblyTreeModel->setUserCheckable(true);

    // monitor AssemblyTreeModel data changes.
    QObject::connect(internals.AssemblyTreeModel.data(), &pqDataAssemblyTreeModel::modelDataChanged,
      this, &pqDataAssemblyPropertyWidget::assemblyTreeModified);

    // monitor StringListModel data changes.
    QObject::connect(internals.StringListModel.data(), &QAbstractItemModel::dataChanged, this,
      &pqDataAssemblyPropertyWidget::stringListModified);
    QObject::connect(internals.StringListModel.data(), &QAbstractItemModel::rowsInserted, this,
      &pqDataAssemblyPropertyWidget::stringListModified);
    QObject::connect(internals.StringListModel.data(), &QAbstractItemModel::rowsRemoved, this,
      &pqDataAssemblyPropertyWidget::stringListModified);

    // observe the property's domain to update the hierarchy when it changes.
    auto domain = smproperty->FindDomain<vtkSMDataAssemblyDomain>();
    if (!domain)
    {
      qWarning("Missing vtkSMDataAssemblyDomain domain.");
    }
    else
    {
      pqCoreUtilities::connect(
        domain, vtkCommand::DomainModifiedEvent, this, SLOT(updateDataAssembly(vtkObject*)));
      this->updateDataAssembly(domain);
    }
    this->addPropertyLink(this, "chosenPaths", SIGNAL(chosenPathsChanged()), smproperty);
  }
  else
  {
    internals.AssemblyTreeModel->setUserCheckable(false);
  }
}

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::~pqDataAssemblyPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::assemblyTreeModified(int role)
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }

  if (role == Qt::CheckStateRole)
  {
    QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
    internals.ChosenPaths = internals.AssemblyTreeModel->checkedNodes();
    internals.StringListModel->setStringList(internals.ChosenPaths);
    Q_EMIT this->chosenPathsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::stringListModified()
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }
  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.ChosenPaths = internals.StringListModel->stringList();
  internals.AssemblyTreeModel->setCheckedNodes(internals.ChosenPaths);
  Q_EMIT this->chosenPathsChanged();
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::updateDataAssembly(vtkObject* sender)
{
  if (auto domain = vtkSMDataAssemblyDomain::SafeDownCast(sender))
  {
    auto& internals = (*this->Internals);
    pqTreeViewExpandState helper;
    helper.save(internals.Ui.tree);
    internals.AssemblyTreeModel->setDataAssembly(domain->GetDataAssembly());
    internals.AssemblyTreeModel->setCheckedNodes(internals.ChosenPaths);
    internals.Ui.tree->setRootIndex(internals.Ui.tree->model()->index(0, 0));
    internals.Ui.tree->expandToDepth(2);
    helper.restore(internals.Ui.tree);
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setChosenPaths(const QStringList& paths)
{
  auto& internals = (*this->Internals);
  internals.ChosenPaths = paths;

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setCheckedNodes(paths);
  internals.StringListModel->setStringList(paths);
  Q_EMIT this->chosenPathsChanged();
}

//-----------------------------------------------------------------------------
const QStringList& pqDataAssemblyPropertyWidget::chosenPaths() const
{
  auto& internals = (*this->Internals);
  return internals.ChosenPaths;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setChosenPaths(const QList<QVariant>& paths)
{
  QStringList string_paths;
  std::transform(paths.begin(), paths.end(), std::back_inserter(string_paths),
    [](const QVariant& value) { return value.toString(); });
  this->setChosenPaths(string_paths);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::chosenPathsAsVariantList() const
{
  QList<QVariant> variant_paths;
  const auto& string_paths = this->chosenPaths();
  std::transform(string_paths.begin(), string_paths.end(), std::back_inserter(variant_paths),
    [](const QString& value) { return QVariant(value); });
  return variant_paths;
}
