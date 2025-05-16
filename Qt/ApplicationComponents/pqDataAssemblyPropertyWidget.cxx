// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDataAssemblyPropertyWidget.h"
#include "ui_pqDataAssemblyPropertyWidget.h"

#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDataRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "pqHeaderView.h"
#include "pqHighlightableToolButton.h"
#include "pqMultiBlockPropertiesStateWidget.h"
#include "pqOutputPort.h"
#include "pqPVApplicationCore.h"
#include "pqSMAdaptor.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqTreeViewExpandState.h"
#include "pqTreeViewSelectionHelper.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDataAssemblyDomain.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentedArrayListDomain.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSelectionSource.h"
#include "vtkSmartPointer.h"

#include <QAbstractTableModel>
#include <QAction>
#include <QIdentityProxyModel>
#include <QPainter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>

#include <algorithm>
#include <set>

namespace detail
{
constexpr int BlockPropertiesStateRole = Qt::UserRole;
constexpr int BlockPropertiesResetRole = Qt::UserRole + 1;
} // end of detail

namespace
{
//=================================================================================
/**
 * A table model to show lists for chosen selectors. This helps use
 * show a compact/non-hierarchical view of selectors.
 */
class TableSelectorsModel : public QAbstractTableModel
{
  using Superclass = QAbstractTableModel;

public:
  TableSelectorsModel(QObject* prnt)
    : Superclass(prnt)
  {
  }
  ~TableSelectorsModel() override = default;

  int columnCount(const QModelIndex&) const override
  {
    return 1; // only one column.
  }

  int rowCount(const QModelIndex&) const override { return this->Values.size(); }

  Qt::ItemFlags flags(const QModelIndex& indx) const override
  {
    return this->Superclass::flags(indx) | Qt::ItemIsEditable;
  }

  void setData(const QList<QPair<QString, QVariant>>& values)
  {
    if (this->Values != values)
    {
      this->beginResetModel();
      this->Values = values;
      this->endResetModel();
    }
  }

  void setData(const QStringList& selectors)
  {
    QList<QPair<QString, QVariant>> values;
    for (const auto& value : selectors)
    {
      values.push_back(qMakePair(value, QVariant()));
    }
    this->setData(values);
  }

  QStringList selectors() const
  {
    QStringList val;
    for (auto& pair : this->Values)
    {
      val.push_back(pair.first);
    }
    return val;
  }

  const QList<QPair<QString, QVariant>>& values() const { return this->Values; }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation != Qt::Horizontal)
    {
      return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
      switch (section)
      {
        default:
        case 0:
          return "Selector";
      }
    }
    return QVariant();
  }

  bool setData(const QModelIndex& index, const QVariant& value, int role) override
  {
    if (index.row() >= 0 && index.row() < this->Values.size() &&
      (role == Qt::EditRole || role == Qt::DisplayRole))
    {
      bool changed = false;
      auto& pair = this->Values[index.row()];
      if (index.column() == 0)
      {
        if (pair.first != value.toString())
        {
          pair.first = value.toString();
          changed = true;
        }
      }

      if (changed)
      {
        Q_EMIT this->dataChanged(index, index, { role });
      }
      return changed;
    }
    return false;
  }
  QVariant data(const QModelIndex& indx, int role) const override
  {
    const auto& pair = this->Values[indx.row()];
    if (indx.column() == 0)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
          return pair.first;
      }
    }
    return QVariant();
  }

  bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
  {
    if (count < 1 || row < 0 || row > this->rowCount(parent))
    {
      return false;
    }
    this->beginInsertRows(parent, row, row + count - 1);
    for (int r = 0; r < count; ++r)
    {
      this->Values.insert(row, {});
    }
    this->endInsertRows();
    return true;
  }

  bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
  {
    if (count <= 0 || row < 0 || (row + count) > this->rowCount(parent))
    {
      return false;
    }

    this->beginRemoveRows(parent, row, row + count - 1);
    for (int r = 0; r < count; ++r)
    {
      this->Values.removeAt(row);
    }
    this->endRemoveRows();
    return true;
  }

private:
  Q_DISABLE_COPY(TableSelectorsModel);
  QList<QPair<QString, QVariant>> Values;
};

class CallbackDataVisitor : public vtkDataAssemblyVisitor
{
public:
  static CallbackDataVisitor* New();
  vtkTypeMacro(CallbackDataVisitor, vtkDataAssemblyVisitor);

  std::function<void(int)> VisitCallback;
  void Visit(int nodeid) override
  {
    if (this->VisitCallback)
    {
      this->VisitCallback(nodeid);
    }
  }

protected:
  CallbackDataVisitor() = default;
  ~CallbackDataVisitor() override = default;

private:
  CallbackDataVisitor(const CallbackDataVisitor&) = delete;
  void operator=(const CallbackDataVisitor&) = delete;
};
vtkStandardNewMacro(CallbackDataVisitor);

//=================================================================================
/**
 * Specialization to ensure the header checkstate/label etc. reflects the root-node.
 * This also handles mapping the custom detail::BlockPropertiesStateRole and
 * detail::BlockPropertiesResetRole roles to columns,
 * and rendering appropriate swatches for the same.
 */
class pqDAPModel : public QIdentityProxyModel
{
  const int PixmapSize;
  using Superclass = QIdentityProxyModel;

  QString HeaderText; // header text for column 0.

public:
  pqDAPModel(int pixmapSize, QObject* prnt)
    : Superclass(prnt)
    , PixmapSize(pixmapSize)
  {
    QObject::connect(this, &QAbstractItemModel::dataChanged,
      [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&)
      {
        if (topLeft.row() == 0 && !topLeft.parent().isValid())
        {
          Q_EMIT this->headerDataChanged(Qt::Horizontal, topLeft.column(), topLeft.column());
        }
      });
  }

  ~pqDAPModel() override { this->resetStateWidgets(); }

  void setHeaderText(const QString& txt)
  {
    if (this->HeaderText != txt)
    {
      this->HeaderText = txt;
      this->headerDataChanged(Qt::Horizontal, 0, 0);
    }
  }

  void setPropertyNames(const QList<QString>& propertyNames)
  {
    if (this->PropertyNames != propertyNames)
    {
      this->PropertyNames = propertyNames;
    }
  }

  void setRepresentation(vtkSMProxy* repr)
  {
    pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
    auto pqRepr = qobject_cast<pqDataRepresentation*>(smm->findItem<pqProxy*>(repr));
    if (this->Representation != pqRepr)
    {
      this->Representation = pqRepr;
    }
  }

  void setSourceModel(QAbstractItemModel* smodel) override
  {
    this->Superclass::setSourceModel(smodel);
    if (smodel)
    {
      // when data with custom roles on the source model change, we need to map
      // it to decorate role on appropriate column to ensure the view updates correctly.
      QObject::connect(smodel, &QAbstractItemModel::dataChanged,
        [&](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
        {
          for (int cc = 0; cc < static_cast<int>(this->ExtraColumns.size()); ++cc)
          {
            const auto& customRole = this->ExtraColumns[cc];
            if (roles.contains(customRole))
            {
              Q_EMIT this->dataChanged(topLeft.siblingAtColumn(cc + 1),
                bottomRight.siblingAtColumn(cc + 1), { Qt::DecorationRole });
            }
          }
        });
    }
  }

  void initializeStateWidgets(vtkDataAssembly* assembly)
  {
    if (!assembly || this->ExtraColumns.empty())
    {
      return;
    }
    for (auto& widget : this->StateWidgets)
    {
      widget->getResetButton()->click();
    }
    this->resetStateWidgets();
    vtkNew<CallbackDataVisitor> visitor;
    visitor->VisitCallback = [&](int nodeId) -> void
    { this->StateWidgets[nodeId] = this->createStateWidget(nodeId); };
    assembly->Visit(visitor);
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

        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          if (section == 0 && !this->HeaderText.isEmpty())
          {
            return this->HeaderText;
          }
          VTK_FALLTHROUGH;
        default:
          return this->data(this->index(0, section), role);
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::EditRole) override
  {
    if (section == 0 && orientation == Qt::Horizontal && this->columnCount() > 0)
    {
      return this->setData(this->index(0, section), value, role);
    }
    return this->Superclass::setHeaderData(section, orientation, value, role);
  }

  int columnCount(const QModelIndex& index = QModelIndex()) const override
  {
    return this->Superclass::columnCount(index) + static_cast<int>(this->ExtraColumns.size());
  }

  QVariant data(const QModelIndex& index, int role) const override
  {
    if (index.column() == 0 || !index.isValid())
    {
      return this->Superclass::data(index, role);
    }

    const int columnRole = this->ExtraColumns.at(index.column() - 1);
    if (columnRole == detail::BlockPropertiesStateRole)
    {
      pqMultiBlockPropertiesStateWidget* stateWidget = this->getStateWidget(index);
      if (!stateWidget)
      {
        return QVariant();
      }
      switch (role)
      {
        case Qt::DecorationRole:
          return stateWidget->getStatePixmap(stateWidget->getState());
        case Qt::ToolTipRole:
          return stateWidget->getStateToolTip(stateWidget->getState());
        case detail::BlockPropertiesStateRole:
          return this->Superclass::data(index.siblingAtColumn(0), detail::BlockPropertiesStateRole);
        default:
          return QVariant();
      }
    }
    else if (columnRole == detail::BlockPropertiesResetRole)
    {
      pqMultiBlockPropertiesStateWidget* stateWidget = this->getStateWidget(index);
      if (!stateWidget)
      {
        return QVariant();
      }
      switch (role)
      {
        case Qt::DecorationRole:
          return stateWidget->getResetButton()->grab();
        case Qt::ToolTipRole:
          return stateWidget->getResetButton()->toolTip();
        case detail::BlockPropertiesResetRole:
          return this->Superclass::data(index.siblingAtColumn(0), detail::BlockPropertiesResetRole);
        default:
          return QVariant();
      }
    }
    return QVariant();
  }

  /**
   * Add a new column for the specified role.
   */
  void addColumn(int role)
  {
    this->beginResetModel();
    this->ExtraColumns.push_back(role);
    this->endResetModel();
  }

  /**
   * Returns the role associated with a column.
   */
  int roleForColumn(int column) const
  {
    column -= 1;
    try
    {
      return this->ExtraColumns.at(column);
    }
    catch (std::out_of_range&)
    {
      return 0;
    }
  }

  pqMultiBlockPropertiesStateWidget* getStateWidget(const QModelIndex& index) const
  {
    const int nodeId = this->getNodeId(index);
    const auto& result = this->StateWidgets.find(nodeId);
    return result != this->StateWidgets.end() ? result.value() : nullptr;
  }

private:
  Q_DISABLE_COPY(pqDAPModel);

  int getNodeId(const QModelIndex& index) const
  {
    auto treeModel = qobject_cast<pqDataAssemblyTreeModel*>(this->sourceModel());
    return treeModel->nodeId(index);
  }

  pqMultiBlockPropertiesStateWidget* createStateWidget(const int nodeId)
  {
    auto treeModel = qobject_cast<pqDataAssemblyTreeModel*>(this->sourceModel());
    const QString selector = QString::fromStdString(treeModel->dataAssembly()->GetNodePath(nodeId));

    pqMultiBlockPropertiesStateWidget* widget = new pqMultiBlockPropertiesStateWidget(
      this->Representation->getProxy(), this->PropertyNames, this->PixmapSize, selector);
    const bool hasBlockColorArrayNames =
      std::find(this->PropertyNames.begin(), this->PropertyNames.end(), "BlockColorArrayNames") !=
      this->PropertyNames.end();
    if (hasBlockColorArrayNames)
    {
      QObject::connect(widget, &pqMultiBlockPropertiesStateWidget::startStateReset, this,
        [this, nodeId]() -> void
        {
          this->OldLuts[nodeId] =
            this->Representation->getLookupTableProxies(vtkSMColorMapEditorHelper::Blocks);
        });
    }
    QObject::connect(widget, &pqMultiBlockPropertiesStateWidget::endStateReset, this,
      [this, nodeId, hasBlockColorArrayNames]() -> void
      {
        if (hasBlockColorArrayNames)
        {
          pqDisplayColorWidget::hideScalarBarsIfNotNeeded(
            this->Representation->getViewProxy(), this->OldLuts[nodeId]);
        }
        Q_EMIT qobject_cast<pqDataAssemblyPropertyWidget*>(this->parent())->changeFinished();
      });
    QObject::connect(widget, &pqMultiBlockPropertiesStateWidget::stateChanged, this,
      [this](pqMultiBlockPropertiesStateWidget::BlockPropertyState)
      {
        auto sourceModel = qobject_cast<pqDataAssemblyTreeModel*>(this->sourceModel());
        QModelIndex sourceModelIndex = sourceModel->index(0);
        // TODO: The above line forces the entire model to be updated, which is not great,
        // but since the below line doesn't work as expected, we have to do this.
        // QModelIndex sourceModelIndex = sourceModel->index(nodeId);
        QModelIndex modelIndex = this->mapFromSource(sourceModelIndex);
        const QModelIndex stateIndex = modelIndex.siblingAtColumn(1);
        const QModelIndex resetIndex = modelIndex.siblingAtColumn(2);
        Q_EMIT this->dataChanged(stateIndex, resetIndex, { Qt::DecorationRole, Qt::ToolTipRole });
      });
    return widget;
  }

  void resetStateWidgets()
  {
    for (auto& widget : this->StateWidgets)
    {
      delete widget;
    }
    this->StateWidgets.clear();
  }

  std::vector<int> ExtraColumns;

  QList<QString> PropertyNames;
  QPointer<pqDataRepresentation> Representation;

  QMap<int, pqMultiBlockPropertiesStateWidget*> StateWidgets;
  QMap<int, std::vector<vtkSMProxy*>> OldLuts;
};

//=================================================================================
void adjustHeader(QHeaderView* header)
{
  if (header->count() > 1)
  {
    header->moveSection(0, header->count() - 1);
    header->setResizeContentsPrecision(10);
    header->resizeSections(QHeaderView::ResizeToContents);
  }
}

//=================================================================================
void hookupTableView(QTableView* view, TableSelectorsModel* model, QAbstractButton* addButton,
  QAbstractButton* removeButton, QAbstractButton* removeAllButton)
{
  // hookup add button
  QObject::connect(addButton, &QAbstractButton::clicked,
    [=](bool)
    {
      auto currentIndex = view->currentIndex();
      const int row =
        currentIndex.isValid() ? (currentIndex.row() + 1) : model->rowCount(QModelIndex());
      if (model->insertRows(row, 1))
      {
        currentIndex = model->index(row, view->horizontalHeader()->logicalIndex(0));
        view->setCurrentIndex(currentIndex);
        view->edit(currentIndex);
      }
    });

  // enabled-state for remove button.
  QObject::connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
    [=](const QItemSelection&, const QItemSelection&)
    { removeButton->setEnabled(view->selectionModel()->hasSelection()); });

  // hookup remove button.
  QObject::connect(removeButton, &QAbstractButton::clicked,
    [=](bool)
    {
      auto rowIndexes = view->selectionModel()->selectedIndexes();
      std::set<int> rows;
      std::transform(rowIndexes.begin(), rowIndexes.end(), std::inserter(rows, rows.end()),
        [](const QModelIndex& idx) { return idx.row(); });
      for (auto iter = rows.rbegin(); iter != rows.rend(); ++iter)
      {
        model->removeRow(*iter);
      }
    });

  // hookup remove all
  QObject::connect(removeAllButton, &QAbstractButton::clicked,
    [=](bool) { model->setData(QList<QPair<QString, QVariant>>{}); });
}

//=================================================================================
// Helper to create a selection for the producerPort to select blocks using the
// specified selectors.
void selectBlocks(vtkSMOutputPort* producerPort, const std::string& assemblyName,
  const std::set<std::string>& selectors)
{
  auto producer = producerPort->GetSourceProxy();

  vtkNew<vtkSelectionSource> selectionSource;
  selectionSource->SetFieldType(vtkSelectionNode::CELL);
  selectionSource->SetContentType(vtkSelectionNode::BLOCK_SELECTORS);
  selectionSource->SetArrayName(assemblyName.c_str());
  for (const auto& selector : selectors)
  {
    selectionSource->AddBlockSelector(selector.c_str());
  }
  selectionSource->Update();

  // create selection proxy
  auto selectionSourceProxy = vtk::TakeSmartPointer(
    vtkSMSourceProxy::SafeDownCast(vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      producer->GetSession(), selectionSource->GetOutput())));
  // create append selection proxy from selection source proxy
  auto appendSelections = vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(selectionSourceProxy)));
  if (appendSelections)
  {
    producer->SetSelectionInput(producerPort->GetPortIndex(), appendSelections, 0);

    auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
    auto selManager = pqPVApplicationCore::instance()->selectionManager();
    selManager->select(smmodel->findItem<pqOutputPort*>(producerPort));
  }
}

//=================================================================================
// Helper to return a list of selectors, if any from the selection source.
// It returns empty list if the selection source doesn't select blocks.
std::vector<std::string> getSelectors(vtkSMProxy* selectionSource, vtkDataAssembly* assembly)
{
  if (!selectionSource)
  {
    return {};
  }

  if (strcmp(selectionSource->GetXMLName(), "BlockSelectorsSelectionSource") == 0)
  {
    auto svp =
      vtkSMStringVectorProperty::SafeDownCast(selectionSource->GetProperty("BlockSelectors"));
    return svp->GetElements();
  }
  else if (strcmp(selectionSource->GetXMLName(), "BlockSelectionSource") == 0)
  {
    auto ids = vtkSMPropertyHelper(selectionSource, "Blocks").GetIdTypeArray();
    return vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
      std::vector<unsigned int>(ids.begin(), ids.end()), assembly);
  }
  return {};
}

//=================================================================================
void hookupActiveSelection(vtkSMProxy* repr, vtkSMProperty* selectedSelectors,
  QItemSelectionModel* selectionModel, pqDataAssemblyTreeModel* model)
{
  const bool enableActiveSelectionProperty =
    vtkPVGeneralSettings::GetInstance()->GetSelectOnClickMultiBlockInspector();
  // This function handles hooking up the selectionModel to follow active selection
  // and vice-versa.
  selectionModel->setProperty("PQ_IGNORE_SELECTION_CHANGES", false);
  QObject::connect(selectionModel, &QItemSelectionModel::selectionChanged,
    [=](const QItemSelection&, const QItemSelection&)
    {
      if (selectionModel->property("PQ_IGNORE_SELECTION_CHANGES").toBool())
      {
        return;
      }
      auto selection = selectionModel->selection();
      auto currentModel = selectionModel->model();
      while (auto proxyModel = qobject_cast<QAbstractProxyModel*>(currentModel))
      {
        selection = proxyModel->mapSelectionToSource(selection);
        currentModel = proxyModel->sourceModel();
      }

      assert(currentModel == model);
      const auto assembly = model->dataAssembly();
      const auto nodeIds = model->nodeId(selection.indexes());
      std::set<std::string> selectors;
      std::transform(nodeIds.begin(), nodeIds.end(), std::inserter(selectors, selectors.end()),
        [&](int id) { return assembly->GetNodePath(id); });

      // mark selected selectors in the widget
      if (auto selectedSelectorsProp = vtkSMStringVectorProperty::SafeDownCast(selectedSelectors))
      {
        BEGIN_UNDO_EXCLUDE();
        selectedSelectorsProp->SetElements({ selectors.begin(), selectors.end() });
        repr->UpdateProperty(selectedSelectorsProp->GetXMLName());
        END_UNDO_EXCLUDE();
      }

      if (enableActiveSelectionProperty)
      {
        // make active selection.
        auto producerPort = vtkSMPropertyHelper(repr, "Input").GetAsOutputPort();
        selectionModel->setProperty("PQ_IGNORE_SELECTION_CHANGES", true);
        const std::string assemblyName = repr && repr->GetProperty("Assembly")
          ? vtkSMPropertyHelper(repr, "Assembly").GetAsString()
          : "Hierarchy";
        selectBlocks(producerPort, assemblyName, selectors);
        selectionModel->setProperty("PQ_IGNORE_SELECTION_CHANGES", false);
      }
    });

  if (enableActiveSelectionProperty)
  {
    auto selManager = pqPVApplicationCore::instance()->selectionManager();
    auto connection = QObject::connect(selManager, &pqSelectionManager::selectionChanged,
      [=](pqOutputPort* port)
      {
        // When user creates selection in the application, reflect it in the
        // widget, if possible.
        if (selectionModel->property("PQ_IGNORE_SELECTION_CHANGES").toBool())
        {
          return;
        }
        selectionModel->setProperty("PQ_IGNORE_SELECTION_CHANGES", true);

        const auto assembly = model->dataAssembly();
        auto producerPort = vtkSMPropertyHelper(repr, "Input").GetAsOutputPort();
        if (port == nullptr || port->getOutputPortProxy() != producerPort || assembly == nullptr)
        {
          // clear selection in the widget.
          selectionModel->clearSelection();
        }
        else
        {
          auto appendSelections = port->getSelectionInput();
          const unsigned int numInputs = appendSelections
            ? vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements()
            : 0;
          for (unsigned int i = 0; i < numInputs; ++i)
          {
            // update selection.
            auto selectionSource = vtkSMSourceProxy::SafeDownCast(
              vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i));
            auto selectors = getSelectors(selectionSource, assembly);
            const auto nodes = assembly->SelectNodes(selectors);
            QList<int> iListNodes;
            std::copy(nodes.begin(), nodes.end(), std::back_inserter(iListNodes));
            auto indexes = model->index(iListNodes);
            QItemSelection selection;
            for (const auto& idx : indexes)
            {
              selection.select(idx, idx);
            }

            std::vector<QAbstractProxyModel*> proxyModels;
            auto currentModel = selectionModel->model();
            while (auto proxyModel = qobject_cast<QAbstractProxyModel*>(currentModel))
            {
              proxyModels.push_back(proxyModel);
              currentModel = proxyModel->sourceModel();
            }
            std::reverse(proxyModels.begin(), proxyModels.end());
            for (auto& proxyModel : proxyModels)
            {
              selection = proxyModel->mapSelectionFromSource(selection);
            }
            selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
          }
        }
        selectionModel->setProperty("PQ_IGNORE_SELECTION_CHANGES", false);
      });

    // ensure that the connection is destroyed with the selectionModel dies.
    QObject::connect(selectionModel, &QObject::destroyed,
      [connection, selManager]() { selManager->disconnect(connection); });
  }
}

//=================================================================================
QList<QVariant> compositeIndicesToVariantList(const std::vector<unsigned int>& cids)
{
  QList<QVariant> result;
  for (const auto& cid : cids)
  {
    result.push_back(cid);
  }
  return result;
}

//=================================================================================
/**
 * A quick way to support use of pqDataAssemblyPropertyWidget for single
 * properties or property groups. When single property, it's treated as a group
 * with just 1 property for function Selectors.
 */
vtkSmartPointer<vtkSMPropertyGroup> createGroup(vtkSMProperty* property)
{
  vtkNew<vtkSMPropertyGroup> group;
  group->AddProperty("Selectors", property);
  group->SetXMLLabel(property->GetXMLLabel());
  return group;
}
}

class pqDataAssemblyPropertyWidget::pqInternals
{
public:
  Ui::DataAssemblyPropertyWidget Ui;
  QPointer<pqDataAssemblyTreeModel> AssemblyTreeModel;
  QPointer<pqDAPModel> ProxyModel;

  /**
   * A Table model for advanced editing of the selectors.
   * This is not supported in composite-indices mode.
   */
  QPointer<TableSelectorsModel> SelectorsTableModel;

  ///@{
  // Cached values.
  QStringList Selectors;
  QList<QVariant> CompositeIndices;
  ///@}

  bool BlockUpdates = false;

  bool InCompositeIndicesMode = false;
  // this is only supported in composite-indices mode.
  bool LeafNodesOnly = false;

  // Indicates if input's proxy name is being shown as header.
  bool UseInputNameAsHeader = false;

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

  // grab hints from the group.
  auto groupHints = smgroup->GetHints()
    ? smgroup->GetHints()->FindNestedElementByName("DataAssemblyPropertyWidget")
    : nullptr;

  int tempValue; // use to make `GetScalarAttribute` calls compact.
  const int iconSize = std::max(this->style()->pixelMetric(QStyle::PM_SmallIconSize), 16);

  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.hierarchy->header()->setDefaultSectionSize(iconSize + 4);
  internals.Ui.hierarchy->header()->setMinimumSectionSize(iconSize + 4);
  internals.Ui.hierarchy->setSortingEnabled(true);

  internals.AssemblyTreeModel = new pqDataAssemblyTreeModel(this);

  // Setup proxy model to add check-state support in the header.
  internals.ProxyModel = new pqDAPModel(iconSize, this);
  internals.ProxyModel->setSourceModel(internals.AssemblyTreeModel);
  internals.ProxyModel->setHeaderText(
    QCoreApplication::translate("ServerManagerXML", smgroup->GetXMLLabel()));

  int useInputNameAsHeader = 0;
  if (groupHints &&
    groupHints->GetScalarAttribute("use_inputname_as_header", &useInputNameAsHeader) &&
    useInputNameAsHeader == 1)
  {
    internals.UseInputNameAsHeader = true;
  }

  // Add a sort-filter model to enable sorting and filtering of item in the tree.
  auto sortModel = new QSortFilterProxyModel(this);
  sortModel->setSourceModel(internals.ProxyModel);
  sortModel->setRecursiveFilteringEnabled(true);
  internals.Ui.hierarchy->setModel(sortModel);

  internals.SelectorsTableModel = new ::TableSelectorsModel(this);
  internals.Ui.table->setModel(internals.SelectorsTableModel);

  // change pqTreeView header.
  internals.Ui.hierarchy->setupCustomHeader(/*use_pqHeaderView=*/true);
  new pqTreeViewSelectionHelper(internals.Ui.hierarchy);
  // TODO maintain selections even when another GUI element is selected
  if (auto headerView = qobject_cast<pqHeaderView*>(internals.Ui.hierarchy->header()))
  {
    headerView->setToggleCheckStateOnSectionClick(false);
    // use underlying structure by default - no alphabetic sort until header clicked.
    headerView->setSortIndicator(-1, Qt::AscendingOrder);
  }

  hookupTableView(internals.Ui.table, internals.SelectorsTableModel, internals.Ui.add,
    internals.Ui.remove, internals.Ui.removeAll);

  int linkActiveSelection = 0;
  if (groupHints && groupHints->GetScalarAttribute("link_active_selection", &linkActiveSelection) &&
    linkActiveSelection == 1)
  {
    hookupActiveSelection(smproxy, smgroup->GetProperty("SelectedSelectors"),
      internals.Ui.hierarchy->selectionModel(), internals.AssemblyTreeModel);
  }

  // A domain that can provide us the assembly we use to show in this property.
  vtkSMDomain* domain = nullptr;
  QVariant usingCompositeIndices;
  if (auto smproperty = smgroup->GetProperty("Selectors"))
  {
    const bool userCheckable = (groupHints == nullptr ||
      !groupHints->GetScalarAttribute("is_checkable", &tempValue) || tempValue == 1);
    if (!userCheckable)
    {
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));
    }

    internals.AssemblyTreeModel->setUserCheckable(userCheckable);
    if ((domain = smproperty->FindDomain<vtkSMDataAssemblyDomain>()))
    {
      // not in composite-indices mode.
      usingCompositeIndices = false;

      // monitor SelectorsTableModel data changes.
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::dataChanged, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::rowsInserted,
        this, &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::rowsRemoved, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::modelReset, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);

      if (userCheckable)
      {
        // link with property
        this->addPropertyLink(this, "selectors", SIGNAL(selectorsChanged()), smproperty);
      }
    }
    else if ((domain = smproperty->FindDomain<vtkSMCompositeTreeDomain>()))
    {
      // in composite-indices mode.
      usingCompositeIndices = true;

      // indicates if we should only returns composite indices for leaf nodes.
      internals.LeafNodesOnly = (vtkSMCompositeTreeDomain::SafeDownCast(domain)->GetMode() ==
        vtkSMCompositeTreeDomain::LEAVES);

      // remove selectors tab, since it's not supported in composite-ids mode.
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));

      if (userCheckable)
      {
        // link with property
        this->addPropertyLink(this, "compositeIndices", SIGNAL(selectorsChanged()), smproperty);
      }
    }
    else
    {
      qWarning()
        << "Missing vtkSMDataAssemblyDomain or vtkSMCompositeTreeDomain domain on property '"
        << smproperty->GetXMLName() << "'. pqDataAssemblyPropertyWidget will not work correctly!";
    }
  }
  else
  {
    // no "Selectors" property i.e. we're not showing checkboxes in our widget.
    internals.AssemblyTreeModel->setUserCheckable(false);
    // remove selectors tab, since it's not applicable..
    internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));
  }

  auto assemblyProp = smgroup->GetProperty("ActiveAssembly");
  if (assemblyProp)
  {
    const std::string assemblyName = vtkSMPropertyHelper(assemblyProp).GetAsString();
    const auto availableAssemblyChoices = pqSMAdaptor::getEnumerationPropertyDomain(assemblyProp);
    const bool isFindDataHelper = strcmp(smproxy->GetXMLName(), "FindDataHelper") == 0;
    // we create a property link in 2 cases:
    // 1) if assembly name is not empty, which means we're showing a composite-dataset, and
    //    there are choices available for the assembly.
    // 2) if the proxy is a FindDataHelper, which is a special case because the assembly name is
    //    empty initially but it will be populated in the future.
    if ((!assemblyName.empty() && !availableAssemblyChoices.empty()) || isFindDataHelper)
    {
      auto adaptor = new pqSignalAdaptorComboBox(internals.Ui.assemblyCombo);
      new pqComboBoxDomain(internals.Ui.assemblyCombo, assemblyProp);
      this->addPropertyLink(
        adaptor, "currentText", SIGNAL(currentTextChanged(QString)), assemblyProp);
    }
    else
    {
      internals.Ui.assemblyCombo->hide();
    }
  }
  else
  {
    internals.Ui.assemblyCombo->hide();
  }

  if (auto repr = vtkSMPVRepresentationProxy::SafeDownCast(smproxy))
  {
    QList<QString> blockPropertyNames;
    auto blockColors = vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("BlockColors"));
    if (blockColors && blockColors->FindDomain<vtkSMDataAssemblyDomain>())
    {
      blockPropertyNames.push_back("BlockColors");
    }
    auto blockColorArrayNames =
      vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("BlockColorArrayNames"));
    if (blockColorArrayNames && blockColorArrayNames->FindDomain<vtkSMRepresentedArrayListDomain>())
    {
      blockPropertyNames.push_back("BlockColorArrayNames");
    }
    auto blockUseSeparateColorMaps =
      vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("BlockUseSeparateColorMaps"));
    if (blockUseSeparateColorMaps &&
      blockUseSeparateColorMaps->FindDomain<vtkSMDataAssemblyDomain>())
    {
      blockPropertyNames.push_back("BlockUseSeparateColorMaps");
    }
    auto blockMapScalars =
      vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("BlockMapScalars"));
    if (blockMapScalars && blockMapScalars->FindDomain<vtkSMDataAssemblyDomain>())
    {
      blockPropertyNames.push_back("BlockMapScalars");
    }
    auto blockInterpolateScalarsBeforeMappings = vtkSMStringVectorProperty::SafeDownCast(
      repr->GetProperty("BlockInterpolateScalarsBeforeMappings"));
    if (blockInterpolateScalarsBeforeMappings &&
      blockInterpolateScalarsBeforeMappings->FindDomain<vtkSMDataAssemblyDomain>())
    {
      blockPropertyNames.push_back("BlockInterpolateScalarsBeforeMappings");
    }
    auto blockOpacities =
      vtkSMStringVectorProperty::SafeDownCast(repr->GetProperty("BlockOpacities"));
    if (blockOpacities && blockOpacities->FindDomain<vtkSMDataAssemblyDomain>())
    {
      blockPropertyNames.push_back("BlockOpacities");
    }
    if (!blockPropertyNames.empty())
    {
      Q_ASSERT(!usingCompositeIndices.isValid() || usingCompositeIndices.toBool() == false);

      usingCompositeIndices = false;

      internals.ProxyModel->setRepresentation(repr);
      internals.ProxyModel->setPropertyNames(blockPropertyNames);
      internals.ProxyModel->addColumn(detail::BlockPropertiesStateRole);
      internals.ProxyModel->addColumn(detail::BlockPropertiesResetRole);
      internals.AssemblyTreeModel->setRoleProperty(
        detail::BlockPropertiesStateRole, pqDataAssemblyTreeModel::InheritedUntilOverridden);
      internals.AssemblyTreeModel->setRoleProperty(
        detail::BlockPropertiesResetRole, pqDataAssemblyTreeModel::InheritedUntilOverridden);
    }
  }

  internals.InCompositeIndicesMode =
    (usingCompositeIndices.isValid() && usingCompositeIndices.toBool());

  if (domain)
  {
    // monitor AssemblyTreeModel data changes.
    QObject::connect(internals.AssemblyTreeModel.data(), &pqDataAssemblyTreeModel::modelDataChanged,
      this, &pqDataAssemblyPropertyWidget::assemblyTreeModified);

    // observe the property's domain to update the hierarchy when it changes.
    pqCoreUtilities::connect(
      domain, vtkCommand::DomainModifiedEvent, this, SLOT(updateDataAssembly(vtkObject*)));
    this->updateDataAssembly(domain);
  }
  else
  {
    qWarning("No suitable domain has been found. pqDataAssemblyPropertyWidget will not be "
             "able to show any hierarchy / data assembly. Please check your Proxy XML.");
  }

  // move columns so that all swatches are shown to the right of the tree
  // this is easier to read than the default.
  auto header = internals.Ui.hierarchy->header();
  if (header->count() > 1)
  {
    // hookup pressed to reset state.
    QObject::connect(internals.Ui.hierarchy, &QTreeView::pressed,
      [&](const QModelIndex& idx)
      {
        auto model = internals.Ui.hierarchy->model();
        auto sourceIndex = qobject_cast<QAbstractProxyModel*>(model)->mapToSource(idx);
        switch (internals.ProxyModel->roleForColumn(idx.column()))
        {
          case detail::BlockPropertiesResetRole:
          {
            auto stateWidget = internals.ProxyModel->getStateWidget(sourceIndex);
            if (stateWidget->getResetButton()->isEnabled())
            {
              stateWidget->getResetButton()->setDown(true);
            }
            break;
          }
          default:
            break;
        }
      });
    // hookup clicked to reset state.
    QObject::connect(internals.Ui.hierarchy, &QTreeView::clicked,
      [&](const QModelIndex& idx)
      {
        auto model = internals.Ui.hierarchy->model();
        auto sourceIndex = qobject_cast<QAbstractProxyModel*>(model)->mapToSource(idx);
        switch (internals.ProxyModel->roleForColumn(idx.column()))
        {
          case detail::BlockPropertiesResetRole:
          {
            auto stateWidget = internals.ProxyModel->getStateWidget(sourceIndex);
            if (stateWidget->getResetButton()->isEnabled())
            {
              stateWidget->getResetButton()->setDown(false);
              stateWidget->getResetButton()->click();
            }
            break;
          }
          default:
            break;
        }
      });
  }

  adjustHeader(internals.Ui.hierarchy->header());
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
  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  auto assembly = internals.assembly();

  if (role == Qt::CheckStateRole)
  {
    internals.Selectors = internals.AssemblyTreeModel->checkedNodes();
    internals.SelectorsTableModel->setData(internals.Selectors);
    internals.CompositeIndices.clear();
    if (assembly != nullptr && internals.InCompositeIndicesMode)
    {
      // compute composite indices.
      std::vector<std::string> selectors(internals.Selectors.size());
      std::transform(internals.Selectors.begin(), internals.Selectors.end(), selectors.begin(),
        [](const QString& str) { return str.toStdString(); });
      internals.CompositeIndices =
        compositeIndicesToVariantList(vtkDataAssemblyUtilities::GetSelectedCompositeIds(
          selectors, assembly, nullptr, internals.LeafNodesOnly));
    }
    Q_EMIT this->selectorsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::selectorsTableModified()
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.Selectors = internals.SelectorsTableModel->selectors();
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
  // internals.Ui.hierarchy->setRootIndex(internals.Ui.hierarchy->model()->index(0, 0));
  internals.Ui.hierarchy->expandToDepth(2);
  helper.restore(internals.Ui.hierarchy);

  if (internals.InCompositeIndicesMode)
  {
    this->setCompositeIndices(internals.CompositeIndices);
  }
  else
  {
    this->setSelectors(internals.Selectors);
  }
  internals.ProxyModel->initializeStateWidgets(assembly);

  auto domain = vtkSMDomain::SafeDownCast(sender);
  if (internals.UseInputNameAsHeader && domain)
  {
    vtkSMPropertyHelper inputHelper(domain->GetRequiredProperty("Input"));
    auto proxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(0));
    auto port = inputHelper.GetOutputPort(0);
    if (proxy && proxy)
    {
      auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
      auto pqport = smmodel->findItem<pqOutputPort*>(proxy->GetOutputPort(port));
      if (pqport)
      {
        internals.ProxyModel->setHeaderText(pqport->prettyName());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectors(const QStringList& paths)
{
  auto& internals = (*this->Internals);
  internals.Selectors = paths;

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setCheckedNodes(paths);
  internals.SelectorsTableModel->setData(paths);
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
  auto& internals = (*this->Internals);
  internals.CompositeIndices = values;

  QList<QVariant> cidSelectors;
  std::transform(values.begin(), values.end(), std::back_inserter(cidSelectors),
    [](const QVariant& var) { return QString("//*[@cid=%1]").arg(var.value<unsigned int>()); });
  this->setSelectors(cidSelectors);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::compositeIndicesAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.CompositeIndices;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::updateWidget(bool showing_advanced_properties)
{
  // we want to show the extra tabs for editing color/opacity/selectors manually
  // only in advanced mode.
  auto& internals = (*this->Internals);
  if (showing_advanced_properties && internals.Ui.tabWidget->count() > 1)
  {
    internals.Ui.tabWidget->tabBar()->show();
  }
  else
  {
    internals.Ui.tabWidget->setCurrentIndex(0);
    internals.Ui.tabWidget->tabBar()->hide();
  }
}
