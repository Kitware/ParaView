// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyInformationWidget.h"
#include "ui_pqProxyInformationWidget.h"

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLocale>
#include <QToolButton>

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDoubleLineEdit.h"
#include "pqNonEditableStyledItemDelegate.h"
#include "pqOutputPort.h"
#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkMath.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include <memory>
#include <vtksys/SystemTools.hxx>

#include <tuple>

// ParaView components includes

static QString formatMemory(vtkTypeInt64 msize)
{
  auto l = QLocale::system();
  if (msize < 1024)
  {
    return l.toString(msize) + " KB";
  }
  else if (msize < 1024 * 1024)
  {
    return l.toString(msize / 1024.0) + " MB";
  }
  else
  {
    return l.toString(msize / (1024 * 1024.0)) + " GB";
  }
}

namespace
{
//=============================================================================
/**
 * QAbstractTableModel subclass that shows all arrays.
 */
//=============================================================================
class pqArraysModel : public QAbstractTableModel
{
  using Superclass = QAbstractTableModel;

public:
  pqArraysModel(QObject* parentObj = nullptr)
    : Superclass(parentObj){};

  /**
   * Get/Set the vtkPVDataInformation instance.
   */
  void setDataInformation(vtkPVDataInformation* info);
  vtkPVDataInformation* dataInformation() const { return this->DataInformation; }

  int rowCount(const QModelIndex&) const override
  {
    return static_cast<int>(this->ArrayInformations.size());
  }
  int columnCount(const QModelIndex&) const override { return 3; }
  QVariant data(const QModelIndex& indx, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
      return this->Superclass::headerData(section, orientation, role);
    }

    switch (section)
    {
      case 0:
        return "Name";
      case 1:
        return "Type";
      case 2:
        return "Ranges";
    }
    return this->Superclass::headerData(section, orientation, role);
  }
  Qt::ItemFlags flags(const QModelIndex& indx) const override
  {
    // make editable to allow copy/paste of text (see
    // pqNonEditableStyledItemDelegate).
    return this->Superclass::flags(indx) | Qt::ItemIsEditable;
  }

private:
  Q_DISABLE_COPY(pqArraysModel);

  QIcon Pixmaps[vtkDataObject::NUMBER_OF_ASSOCIATIONS] = {
    QIcon(":/pqWidgets/Icons/pqPointData.svg"),
    QIcon(":/pqWidgets/Icons/pqCellData.svg"),
    QIcon(":/pqWidgets/Icons/pqGlobalData.svg"),
    QIcon(),
    QIcon(":/pqWidgets/Icons/pqPointData.svg"),
    QIcon(":/pqWidgets/Icons/pqCellData.svg"),
    QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg"),
  };

  // elements have the structure [association, array index, array info]
  std::vector<std::tuple<int, int, vtkSmartPointer<vtkPVArrayInformation>>> ArrayInformations;
  vtkSmartPointer<vtkPVDataSetAttributesInformation>
    AttributeInformations[vtkDataObject::NUMBER_OF_ASSOCIATIONS];
  vtkSmartPointer<vtkPVDataInformation> DataInformation;
};

void pqArraysModel::setDataInformation(vtkPVDataInformation* dinfo)
{
  this->beginResetModel();
  this->DataInformation = dinfo;
  this->ArrayInformations.clear();
  for (int attributeIdx = 0; dinfo && attributeIdx < vtkDataObject::NUMBER_OF_ASSOCIATIONS;
       ++attributeIdx)
  {
    if (auto dsa = dinfo->GetAttributeInformation(attributeIdx))
    {
      this->AttributeInformations[attributeIdx] = dsa;

      std::shared_ptr<vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator> iter(
        dsa->NewAlphabeticalArrayInformationIterator());
      int arrayIdx = 0;
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        this->ArrayInformations.emplace_back(
          attributeIdx, arrayIdx, iter->GetCurrentArrayInformation());
        ++arrayIdx;
      }
    }
    else
    {
      this->AttributeInformations[attributeIdx] = nullptr;
    }
  }
  this->endResetModel();
}

QVariant pqArraysModel::data(const QModelIndex& indx, int role) const
{
  const int r = indx.row();
  const int c = indx.column();
  auto& tuple = this->ArrayInformations.at(r);
  int fieldAssociation = std::get<0>(tuple);
  int arrayIndex = std::get<1>(tuple);
  auto ainfo = std::get<2>(tuple);

  if (role == Qt::DecorationRole && c == 0)
  {
    return this->Pixmaps[fieldAssociation];
  }
  else if (role == Qt::ForegroundRole)
  {
    if (pqCoreUtilities::isDarkTheme())
    {
      return ainfo->GetIsPartial() ? QBrush(Qt::blue) : QBrush(Qt::green);
    }
    else
    {
      return ainfo->GetIsPartial() ? QBrush(Qt::darkBlue) : QBrush(Qt::darkGreen);
    }
  }
  else if (role == Qt::ToolTipRole)
  {
    vtkPVDataSetAttributesInformation* dsa = this->AttributeInformations[fieldAssociation];
    int attributeType = dsa ? dsa->IsArrayAnAttribute(arrayIndex) : -1;

    QString retStr = QString("Attribute Type: ");
    if (attributeType != -1)
    {
      return retStr + QString(vtkDataSetAttributes::GetAttributeTypeAsString(attributeType));
    }
    return retStr + QString("None");
  }
  else if (role != Qt::DisplayRole && role != Qt::EditRole)
  {
    return QVariant();
  }

  switch (indx.column())
  {
    case 0:
      return ainfo->GetIsPartial() ? QString("%1 (partial)").arg(ainfo->GetName())
                                   : QString(ainfo->GetName());

    case 1:
      return ainfo->GetDataTypeAsString();

    case 2:
      auto settings = vtkPVGeneralSettings::GetInstance();
      int lowExponent = settings->GetFullNotationLowExponent();
      int highExponent = settings->GetFullNotationHighExponent();
      return ainfo->GetRangesAsString(lowExponent, highExponent).c_str();
  }
  return QVariant();
}

//=============================================================================
/**
 * QAbstractTableModel subclass for showing timestep values
 */
//=============================================================================
class pqTimestepValuesModel : public QAbstractTableModel
{
  using Superclass = QAbstractTableModel;

public:
  pqTimestepValuesModel(QObject* parentObj = nullptr)
    : Superclass(parentObj)
  {
  }
  ~pqTimestepValuesModel() override = default;

  bool setTimeSteps(const std::vector<double>& timesteps)
  {
    if (this->TimeSteps != timesteps)
    {
      this->beginResetModel();
      this->TimeSteps = timesteps;
      this->endResetModel();
      return true;
    }
    return false;
  }

  int rowCount(const QModelIndex&) const override
  {
    return static_cast<int>(this->TimeSteps.size());
  }

  int columnCount(const QModelIndex&) const override { return 2; }

  QVariant data(const QModelIndex& indx, int role) const override
  {
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::EditRole)
    {
      switch (indx.column())
      {
        case 0:
          return indx.row();
        case 1:
          return pqCoreUtilities::formatTime(this->TimeSteps[indx.row()]);
      }
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
      switch (section)
      {
        case 0:
          return "Index";
        case 1:
          return "Time";
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  Qt::ItemFlags flags(const QModelIndex& indx) const override
  {
    // make editable to allow copy/paste of text (see
    // pqNonEditableStyledItemDelegate).
    return this->Superclass::flags(indx) | Qt::ItemIsEditable;
  }

private:
  Q_DISABLE_COPY(pqTimestepValuesModel);
  std::vector<double> TimeSteps;
};
} // namespace

class pqProxyInformationWidget::pqInternals
{
  QItemSelectionModel* activeSelectionModel() const
  {
    if (this->Ui.compositeTree->isVisible())
    {
      return this->Ui.compositeTree->selectionModel();
    }
    else if (this->Ui.assemblyTree->isVisible())
    {
      return this->Ui.assemblyTree->selectionModel();
    }
    return nullptr;
  }
  vtkDataAssembly* activeGrouping() const
  {
    if (this->Ui.compositeTree->isVisible())
    {
      return this->HierarchyModel.dataAssembly();
    }
    else if (this->Ui.assemblyTree->isVisible())
    {
      return this->AssemblyModel.dataAssembly();
    }
    return nullptr;
  }

public:
  Ui::ProxyInformationWidget Ui;
  pqDataAssemblyTreeModel HierarchyModel;
  pqDataAssemblyTreeModel AssemblyModel;
  pqArraysModel ArraysModel;
  pqTimestepValuesModel TimestepValuesModel;

  // The currently shown output port.
  QPointer<pqOutputPort> Port;
  vtkNew<vtkPVDataInformation> SubsetDataInformation;

  vtkSMSourceProxy* sourceProxy() const
  {
    return this->Port ? this->Port->getSourceProxy() : nullptr;
  }

  // returns true of we want to show any subset of the data information.
  bool showSubsetInformation() const
  {
    auto selModel = this->activeSelectionModel();
    return (selModel && selModel->currentIndex().isValid());
  }

  vtkPVDataInformation* subsetDataInformation() const
  {
    auto dinfo = this->Port ? this->Port->getDataInformation() : nullptr;
    if (!this->showSubsetInformation() || !this->Port)
    {
      return dinfo;
    }

    auto selModel = this->activeSelectionModel();
    auto assembly = this->activeGrouping();
    assert(selModel && assembly);

    const bool useHierarchy = assembly == this->HierarchyModel.dataAssembly();
    auto nodeId = useHierarchy ? this->HierarchyModel.nodeId(selModel->currentIndex())
                               : this->AssemblyModel.nodeId(selModel->currentIndex());

    if (useHierarchy && nodeId == vtkDataAssembly::GetRootNode())
    {
      // if root is selected on the hierarchy, it's simply the full info.
      return dinfo;
    }

    return this->sourceProxy()->GetSubsetDataInformation(
      /*outputIdx=*/dinfo->GetPortNumber(),
      /*selector=*/assembly->GetNodePath(nodeId).c_str(),
      /*assemblyName=*/useHierarchy ? vtkDataAssemblyUtilities::HierarchyName() : "Assembly");
  }

  // populate filename related widgets
  void setFileName(vtkSMProxy* proxy)
  {
    std::string fname;
    if (auto pname = vtkSMCoreUtilities::GetFileNameProperty(proxy))
    {
      auto property = proxy->GetProperty(pname);
      assert(property != nullptr);
      // use info-property, if any.
      property = property->GetInformationProperty() ? property->GetInformationProperty() : property;
      vtkSMPropertyHelper helper(property);
      fname = helper.GetNumberOfElements() > 0 ? helper.GetAsString(0) : "";
    }
    if (!fname.empty())
    {
      this->Ui.filename->setText(vtksys::SystemTools::GetFilenameName(fname).c_str());
      this->Ui.path->setText(vtksys::SystemTools::GetFilenamePath(fname).c_str());
      this->Ui.filename->setVisible(true);
      this->Ui.path->setVisible(true);
      this->Ui.labelFilename->setVisible(true);
      this->Ui.labelPath->setVisible(true);
      this->Ui.labelFilePropertiesNA->setVisible(false);
    }
    else
    {
      this->Ui.filename->setText(QString("(%1)").arg(tr("n/a")));
      this->Ui.path->setText(QString("(%1)").arg(tr("n/a")));
      this->Ui.filename->setVisible(false);
      this->Ui.path->setVisible(false);
      this->Ui.labelFilename->setVisible(false);
      this->Ui.labelPath->setVisible(false);
      this->Ui.labelFilePropertiesNA->setVisible(true);
    }
  }

  // populate widgets that show composite data grouping
  void setDataGrouping(vtkDataAssembly* hierarchy, vtkDataAssembly* assembly)
  {
    this->HierarchyModel.setDataAssembly(hierarchy);
    this->AssemblyModel.setDataAssembly(assembly);

    if (hierarchy)
    {
      this->Ui.rawCompositeTree->setPlainText(
        hierarchy->SerializeToXML(vtkIndent().GetNextIndent()).c_str());
    }
    else
    {
      this->Ui.rawCompositeTree->setPlainText(QString());
    }

    if (assembly)
    {
      this->Ui.rawAssemblyTree->setPlainText(
        assembly->SerializeToXML(vtkIndent().GetNextIndent()).c_str());
    }
    else
    {
      this->Ui.rawAssemblyTree->setPlainText(QString());
    }

    // if assembly doesn't exists, hide the "Assembly" tab.
    const auto idxOf = this->Ui.dataGroupingTab->indexOf(this->Ui.assemblyTab);
    if (idxOf == -1 && assembly != nullptr)
    {
      this->Ui.dataGroupingTab->addTab(this->Ui.assemblyTab, "Assembly");
    }
    else if (idxOf != -1 && assembly == nullptr)
    {
      this->Ui.dataGroupingTab->removeTab(idxOf);
    }

    // TODO: save/restore expand and selection state if possible.
    this->Ui.compositeTree->expandAll();
    this->Ui.assemblyTree->expandAll();

    this->Ui.compositeTree->updateGeometry();
    this->Ui.assemblyTree->updateGeometry();

    if (hierarchy || assembly)
    {
      this->Ui.labelDataGrouping->hide();
      this->Ui.dataGroupingTab->show();
    }
    else
    {
      this->Ui.labelDataGrouping->show();
      this->Ui.dataGroupingTab->hide();
    }
  }

  // populate arrays info
  void setDataArrays(vtkPVDataInformation* dinfo)
  {
    this->ArraysModel.setDataInformation(dinfo);
    this->Ui.dataArraysTable->header()->resizeSections(QHeaderView::ResizeToContents);
    this->Ui.dataArraysTable->updateGeometry();
  }

  // populate timesteps info.
  void setTimeSteps(vtkPVDataInformation* dinfo)
  {
    bool updated = false;
    if (dinfo)
    {
      const std::set<double>& timeSteps = dinfo->GetTimeSteps();
      std::vector<double> vec;
      std::copy(timeSteps.begin(), timeSteps.end(), std::back_inserter(vec));
      updated = this->TimestepValuesModel.setTimeSteps(vec);
    }
    else
    {
      updated = this->TimestepValuesModel.setTimeSteps({});
    }
    if (updated)
    {
      this->Ui.timeValues->header()->resizeSections(QHeaderView::ResizeToContents);
    }
  }

  // update data statistics widgets
  void setDataStatistics(vtkPVDataInformation* dinfo)
  {
    auto l = QLocale::system();
    auto& ui = this->Ui;
    ui.dataType->setEnabled(dinfo && !dinfo->IsNull());
    ui.labelDataType->setEnabled(dinfo && !dinfo->IsNull());

    if (!dinfo)
    {
      ui.dataType->setText(QString("(%1)").arg(tr("n/a")));
    }
    else
    {
      if (dinfo->GetCompositeDataSetType() != -1)
      {
        std::vector<int> uniqueTypes = dinfo->GetUniqueBlockTypes();
        std::string dataTypeText = dinfo->GetPrettyDataTypeString();
        dataTypeText += " (";

        if (uniqueTypes.size() >= 2)
        {
          dataTypeText += "Mixed Data Types";
        }
        else
        {
          dataTypeText += dinfo->GetPrettyDataTypeString(dinfo->GetDataSetType());
        }

        dataTypeText += ")";
        ui.dataType->setText(dataTypeText.c_str());
      }
      else
      {
        ui.dataType->setText(dinfo->GetPrettyDataTypeString());
      }
    }

    ui.labelDataStatistics->setText((dinfo && dinfo->GetNumberOfDataSets() > 1)
        ? tr("Data Statistics (# of datasets: %1)").arg(l.toString(dinfo->GetNumberOfDataSets()))
        : tr("Data Statistics"));

    std::map<int, std::pair<QLabel*, QLabel*>> labelMap;
    labelMap[vtkDataObject::POINT] = std::make_pair(ui.labelPointCount, ui.pointCount);
    labelMap[vtkDataObject::CELL] = std::make_pair(ui.labelCellCount, ui.cellCount);
    labelMap[vtkDataObject::VERTEX] = std::make_pair(ui.labelVertexCount, ui.vertexCount);
    labelMap[vtkDataObject::EDGE] = std::make_pair(ui.labelEdgeCount, ui.edgeCount);
    labelMap[vtkDataObject::ROW] = std::make_pair(ui.labelRowCount, ui.rowCount);
    for (auto& item : labelMap)
    {
      if (dinfo && dinfo->IsAttributeValid(item.first))
      {
        item.second.second->setText(l.toString(dinfo->GetNumberOfElements(item.first)));
        item.second.first->setVisible(true);
        item.second.second->setVisible(true);
        if (item.first == vtkDataObject::POINT)
        {
          auto pointInfo = dinfo->GetPointArrayInformation();
          auto text = item.second.second->text();
          text.append(" (").append(pointInfo->GetDataTypeAsString()).append(")");
          item.second.second->setText(text);
        }
      }
      else
      {
        item.second.second->setText(QString("(%1)").arg(tr("n/a")));
        item.second.first->setVisible(false);
        item.second.second->setVisible(false);
      }
    }

    ui.memory->setEnabled(dinfo != nullptr);
    ui.labelMemory->setEnabled(dinfo != nullptr);
    ui.memory->setText(
      dinfo ? formatMemory(dinfo->GetMemorySize()) : QString("(%1)").arg(tr("n/a")));

    if (dinfo && vtkMath::AreBoundsInitialized(dinfo->GetBounds()))
    {
      double bds[6];
      dinfo->GetBounds(bds);
      ui.bounds->setText(tr("%1 to %2 (delta: %3)\n"
                            "%4 to %5 (delta: %6)\n"
                            "%7 to %8 (delta: %9)")
                           .arg(bds[0])
                           .arg(bds[1])
                           .arg(bds[1] - bds[0])
                           .arg(bds[2])
                           .arg(bds[3])
                           .arg(bds[3] - bds[2])
                           .arg(bds[4])
                           .arg(bds[5])
                           .arg(bds[5] - bds[4]));
      ui.labelBounds->setVisible(true);
      ui.bounds->setVisible(true);
    }
    else
    {
      ui.bounds->setText(QString("%1\n%2\n%3")
                           .arg(QString("(%1)").arg(tr("n/a")))
                           .arg(QString("(%1)").arg(tr("n/a")))
                           .arg(QString("(%1)").arg(tr("n/a"))));
      ui.labelBounds->setVisible(false);
      ui.bounds->setVisible(false);
    }

    if (dinfo && dinfo->GetExtent()[0] <= dinfo->GetExtent()[1])
    {
      int exts[6];
      dinfo->GetExtent(exts);
      ui.extents->setText(tr("%1 to %2 (dimension: %3)\n"
                             "%4 to %5 (dimension: %6)\n"
                             "%7 to %8 (dimension: %9)")
                            .arg(l.toString(exts[0]))
                            .arg(l.toString(exts[1]))
                            .arg(l.toString(exts[1] - exts[0] + 1))
                            .arg(l.toString(exts[2]))
                            .arg(l.toString(exts[3]))
                            .arg(l.toString(exts[3] - exts[2] + 1))
                            .arg(l.toString(exts[4]))
                            .arg(l.toString(exts[5]))
                            .arg(l.toString(exts[5] - exts[4] + 1)));

      ui.labelExtents->setVisible(true);
      ui.extents->setVisible(true);
    }
    else
    {
      ui.extents->setText(QString("%1\n%2\n%3")
                            .arg(QString("(%1)").arg(tr("n/a")))
                            .arg(QString("(%1)").arg(tr("n/a")))
                            .arg(QString("(%1)").arg(tr("n/a"))));
      ui.labelExtents->setVisible(false);
      ui.extents->setVisible(false);
    }

    ui.labelTimeSteps->setVisible(dinfo && dinfo->GetHasTime());
    ui.timeSteps->setVisible(dinfo && dinfo->GetHasTime());
    if (dinfo && dinfo->GetHasTime())
    {
      ui.timeSteps->setText(l.toString(dinfo->GetNumberOfTimeSteps()));
    }

    ui.labelCurrentTime->setVisible(dinfo && dinfo->GetHasTime());
    ui.currentTime->setVisible(dinfo && dinfo->GetHasTime());
    if (dinfo && dinfo->GetHasTime())
    {
      ui.currentTime->setText(tr("%1 (range: [%2, %3])")
                                .arg(pqCoreUtilities::formatTime(dinfo->GetTime()))
                                .arg(dinfo->GetTimeRange()[0])
                                .arg(dinfo->GetTimeRange()[1]));
    }
  }
};

//-----------------------------------------------------------------------------
pqProxyInformationWidget::pqProxyInformationWidget(QWidget* parentWdg)
  : Superclass(parentWdg)
  , Internals(new pqProxyInformationWidget::pqInternals())
{
  auto& internals = (*this->Internals);
  auto& ui = internals.Ui;
  ui.setupUi(this);

  // setup delegates to make it easy to copy text from tree/table widgets.
  ui.compositeTree->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  ui.assemblyTree->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  ui.dataArraysTable->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  ui.timeValues->setItemDelegate(new pqNonEditableStyledItemDelegate(this));

  // setup model for tree views.
  ui.compositeTree->setModel(&internals.HierarchyModel);
  ui.assemblyTree->setModel(&internals.AssemblyModel);
  ui.dataArraysTable->setModel(&internals.ArraysModel);
  ui.timeValues->setModel(&internals.TimestepValuesModel);

  // add corner widget for data-grouping tab box.
  QToolButton* button = new QToolButton(this);
  button->setCheckable(true);
  button->setChecked(false);
  button->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced.svg"));
  button->setObjectName("showRawDataGrouping");
  button->setToolTip(tr("Show hierarchy / assembly as text"));
  ui.rawCompositeTree->hide();
  ui.rawAssemblyTree->hide();

  QObject::connect(button, &QToolButton::toggled,
    [&ui](bool showRaw)
    {
      ui.rawCompositeTree->setVisible(showRaw);
      ui.rawAssemblyTree->setVisible(showRaw);
    });
  ui.dataGroupingTab->setCornerWidget(button);

  // monitor active port.
  auto& activeObjects = pqActiveObjects::instance();
  this->connect(
    &activeObjects, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(setOutputPort(pqOutputPort*)));

  // when settings changes, they may impact how time values are shown, hence
  // refresh.
  pqCoreUtilities::connect(
    vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this, SLOT(updateUI()));

  // show active data information.
  if (auto port = activeObjects.activePort())
  {
    this->setOutputPort(port);
  }
  else
  {
    this->updateUI();
  }

  this->connect(ui.compositeTree->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), SLOT(updateSubsetUI()));
  this->connect(ui.assemblyTree->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), SLOT(updateSubsetUI()));
}

//-----------------------------------------------------------------------------
pqProxyInformationWidget::~pqProxyInformationWidget() = default;

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::setOutputPort(pqOutputPort* port)
{
  auto& internals = (*this->Internals);
  if (internals.Port == port)
  {
    return;
  }
  if (internals.Port)
  {
    this->disconnect(internals.Port->getSource());
  }
  internals.Port = port;
  if (port)
  {
    this->connect(port->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(updateUI()));
  }
  this->updateUI();
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqOutputPort* pqProxyInformationWidget::outputPort() const
{
  auto& internals = (*this->Internals);
  return internals.Port;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqProxyInformationWidget::dataInformation() const
{
  auto port = this->outputPort();
  return port ? port->getDataInformation() : nullptr;
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::updateUI()
{
  auto& internals = (*this->Internals);
  auto proxy = internals.sourceProxy();
  auto dinfo = this->dataInformation();
  internals.setFileName(proxy);
  internals.setDataGrouping(
    dinfo ? dinfo->GetHierarchy() : nullptr, dinfo ? dinfo->GetDataAssembly() : nullptr);
  internals.setTimeSteps(dinfo);

  this->updateSubsetUI();
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::updateSubsetUI()
{
  auto& internals = (*this->Internals);

  // all the following depends on subset data information, if the user chose to
  // see only part of the hierarchy.
  auto subsetInfo = internals.subsetDataInformation();
  internals.setDataStatistics(subsetInfo);
  internals.setDataArrays(subsetInfo);
}
