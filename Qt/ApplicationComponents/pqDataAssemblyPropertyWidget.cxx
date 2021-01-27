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

#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqTreeViewExpandState.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkSMCompositeTreeDomain.h"
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
 * with just 1 property for function Selectors.
 */
vtkSmartPointer<vtkSMPropertyGroup> createGroup(vtkSMProperty* property)
{
  vtkNew<vtkSMPropertyGroup> group;
  if (property->FindDomain<vtkSMDataAssemblyDomain>())
  {
    group->AddProperty("Selectors", property);
  }
  else if (property->FindDomain<vtkSMCompositeTreeDomain>())
  {
    group->AddProperty("CompositeIndices", property);
  }
  return group;
}
}

class pqDataAssemblyPropertyWidget::pqInternals
{
public:
  Ui::DataAssemblyPropertyWidget Ui;
  QPointer<pqDataAssemblyTreeModel> AssemblyTreeModel;
  QPointer<QStringListModel> StringListModel;
  QStringList Selectors;
  std::vector<unsigned int> CompositeIndices;
  bool BlockUpdates = false;

  bool InCompositeIndicesMode = false;
  bool LeafNodesOnly = false;

  vtkDataAssembly* assembly() const { return this->AssemblyTreeModel->dataAssembly(); }
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
  internals.Ui.hierarchy->setModel(sortmodel);

  internals.StringListModel = new QStringListModel(this);
  internals.Ui.table->setModel(internals.StringListModel);

  // change pqTreeView header.
  internals.Ui.hierarchy->setupCustomHeader(/*use_pqHeaderView=*/true);
  new pqTreeViewSelectionHelper(internals.Ui.hierarchy);

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

  if (auto smproperty = smgroup->GetProperty("Selectors"))
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
    this->addPropertyLink(this, "selectors", SIGNAL(selectorsChanged()), smproperty);
  }
  else if (smproperty = smgroup->GetProperty("CompositeIndices"))
  {
    internals.InCompositeIndicesMode = true;

    internals.AssemblyTreeModel->setUserCheckable(true);

    // hide the tab bar since we don't show the list of strings for in this mode.
    internals.Ui.tabWidget->tabBar()->hide();

    // monitor AssemblyTreeModel data changes.
    QObject::connect(internals.AssemblyTreeModel.data(), &pqDataAssemblyTreeModel::modelDataChanged,
      this, &pqDataAssemblyPropertyWidget::assemblyTreeModified);

    // observe the property's domain to update the hierarchy when it changes.
    auto domain = smproperty->FindDomain<vtkSMCompositeTreeDomain>();
    if (!domain)
    {
      qWarning("Missing vtkSMCompositeTreeDomain domain.");
    }
    else
    {
      // indicates if we should only returns composite indices for leaf nodes.
      internals.LeafNodesOnly == (domain->GetMode() == vtkSMCompositeTreeDomain::LEAVES);

      pqCoreUtilities::connect(
        domain, vtkCommand::DomainModifiedEvent, this, SLOT(updateDataAssembly(vtkObject*)));
      this->updateDataAssembly(domain);
    }
    this->addPropertyLink(this, "compositeIndices", SIGNAL(compositeIndicesChanged()), smproperty);
  }
  else
  {
    internals.AssemblyTreeModel->setUserCheckable(false);
  }

  if (auto smproperty = smgroup->GetProperty("ActiveAssembly"))
  {
    auto adaptor = new pqSignalAdaptorComboBox(internals.Ui.assemblyCombo);
    new pqComboBoxDomain(internals.Ui.assemblyCombo, smproperty);
    this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), smproperty);
  }
  else
  {
    internals.Ui.assemblyCombo->hide();
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

  if (role != Qt::CheckStateRole)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.Selectors = internals.AssemblyTreeModel->checkedNodes();
  internals.StringListModel->setStringList(internals.Selectors);
  Q_EMIT this->selectorsChanged();

  if (internals.InCompositeIndicesMode)
  {
    // compute composite indices.
    if (auto assembly = internals.assembly())
    {
      std::vector<std::string> selectors(internals.Selectors.size());
      std::transform(internals.Selectors.begin(), internals.Selectors.end(), selectors.begin(),
        [](const QString& str) { return str.toStdString(); });
      internals.CompositeIndices = vtkDataAssemblyUtilities::GenerateCompositeIndicesFromSelectors(
        assembly, selectors, internals.LeafNodesOnly);
    }
    else
    {
      internals.CompositeIndices.clear();
    }
    Q_EMIT this->compositeIndicesChanged();
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
  internals.Selectors = internals.StringListModel->stringList();
  internals.AssemblyTreeModel->setCheckedNodes(internals.Selectors);
  Q_EMIT this->selectorsChanged();
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::updateDataAssembly(vtkObject* sender)
{
  vtkDataAssembly* assembly = nullptr;
  const char* name = "(?)";
  if (auto domain = vtkSMDataAssemblyDomain::SafeDownCast(sender))
  {
    assembly = domain->GetDataAssembly();
    name = domain->GetDataAssemblyName();
  }
  else if (auto cdomain = vtkSMCompositeTreeDomain::SafeDownCast(sender))
  {
    assembly = cdomain->GetHierarchy();
    name = "Hierarchy";
  }
  auto& internals = (*this->Internals);
  pqTreeViewExpandState helper;
  helper.save(internals.Ui.hierarchy);
  internals.AssemblyTreeModel->setDataAssembly(assembly);
  internals.AssemblyTreeModel->setCheckedNodes(internals.Selectors);
  internals.Ui.tabWidget->setTabText(0, name);
  internals.Ui.hierarchy->setRootIndex(internals.Ui.hierarchy->model()->index(0, 0));
  internals.Ui.hierarchy->expandToDepth(2);
  helper.restore(internals.Ui.hierarchy);
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectors(const QStringList& paths)
{
  auto& internals = (*this->Internals);
  internals.Selectors = paths;

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setCheckedNodes(paths);
  internals.StringListModel->setStringList(paths);
  Q_EMIT this->selectorsChanged();
}

//-----------------------------------------------------------------------------
const QStringList& pqDataAssemblyPropertyWidget::selectors() const
{
  auto& internals = (*this->Internals);
  return internals.Selectors;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectors(const QList<QVariant>& paths)
{
  QStringList string_paths;
  std::transform(paths.begin(), paths.end(), std::back_inserter(string_paths),
    [](const QVariant& value) { return value.toString(); });
  this->setSelectors(string_paths);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::selectorsAsVariantList() const
{
  QList<QVariant> variant_paths;
  const auto& string_paths = this->selectors();
  std::transform(string_paths.begin(), string_paths.end(), std::back_inserter(variant_paths),
    [](const QString& value) { return QVariant(value); });
  return variant_paths;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setCompositeIndices(const QList<QVariant>& values)
{
  std::vector<unsigned int> indices(values.size());
  std::transform(values.begin(), values.end(), indices.begin(),
    [](const QVariant& var) { return var.value<unsigned int>(); });

  auto& internals = (*this->Internals);
  internals.CompositeIndices = indices;
  // TODO:
  Q_EMIT this->compositeIndicesChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::compositeIndicesAsVariantList() const
{
  const auto& internals = (*this->Internals);
  QList<QVariant> result;
  for (const auto& item : internals.CompositeIndices)
  {
    result.push_back(item);
  }
  return result;
}
