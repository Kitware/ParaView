/*=========================================================================

   Program: ParaView
   Module:    pqProxyInformationWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqProxyInformationWidget.h"
#include "ui_pqProxyInformationWidget.h"

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLocale>

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDoubleLineEdit.h"
#include "pqNonEditableStyledItemDelegate.h"
#include "pqOutputPort.h"
#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
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
#include <vtksys/SystemTools.hxx>

// ParaView components includes

static QString formatTime(double time)
{
  auto settings = vtkPVGeneralSettings::GetInstance();
  const auto precision = settings->GetAnimationTimePrecision();
  const auto notation =
    static_cast<pqDoubleLineEdit::RealNumberNotation>(settings->GetAnimationTimeNotation());
  return pqDoubleLineEdit::formatDouble(time, notation, precision);
}

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
    QIcon(":/pqWidgets/Icons/pqPointData.svg"), QIcon(":/pqWidgets/Icons/pqCellData.svg"),
    QIcon(":/pqWidgets/Icons/pqGlobalData.svg"), QIcon(),
    QIcon(":/pqWidgets/Icons/pqPointData.svg"), QIcon(":/pqWidgets/Icons/pqCellData.svg"),
    QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg"),
  };

  std::vector<std::pair<int, vtkSmartPointer<vtkPVArrayInformation> > > ArrayInformations;
  vtkSmartPointer<vtkPVDataInformation> DataInformation;
};

void pqArraysModel::setDataInformation(vtkPVDataInformation* dinfo)
{
  this->beginResetModel();
  this->DataInformation = dinfo;
  this->ArrayInformations.clear();
  for (int cc = 0; dinfo && cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++cc)
  {
    if (auto dsa = dinfo->GetAttributeInformation(cc))
    {
      for (int kk = 0, max = dsa->GetNumberOfArrays(); kk < max; ++kk)
      {
        this->ArrayInformations.push_back(std::pair<int, vtkSmartPointer<vtkPVArrayInformation> >(
          cc, dsa->GetArrayInformation(kk)));
      }
    }
  }
  this->endResetModel();
}

QVariant pqArraysModel::data(const QModelIndex& indx, int role) const
{
  const int r = indx.row();
  const int c = indx.column();
  auto& pair = this->ArrayInformations.at(r);
  auto fieldAssociation = pair.first;
  auto ainfo = pair.second;

  if (role == Qt::DecorationRole && c == 0)
  {
    return this->Pixmaps[fieldAssociation];
  }
  else if (role == Qt::ForegroundRole)
  {
    return ainfo->GetIsPartial() ? QBrush(Qt::darkBlue) : QBrush(Qt::darkGreen);
  }
  else if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::EditRole)
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
      return ainfo->GetRangesAsString().c_str();
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

  void setTimeSteps(const std::vector<double>& timesteps)
  {
    if (this->TimeSteps != timesteps)
    {
      this->beginResetModel();
      this->TimeSteps = timesteps;
      this->endResetModel();
    }
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
          return formatTime(this->TimeSteps[indx.row()]);
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
      this->Ui.filename->setText("(n/a)");
      this->Ui.path->setText("(n/a)");
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
    if (dinfo && dinfo->GetHasTime())
    {
      this->Ui.labelDataArrays->setText(
        QString("Data Arrays (time: %1)").arg(formatTime(dinfo->GetTime())));
    }
    else
    {
      this->Ui.labelDataArrays->setText("Data Arrays");
    }

    this->ArraysModel.setDataInformation(dinfo);
    this->Ui.dataArraysTable->header()->resizeSections(QHeaderView::ResizeToContents);
    this->Ui.dataArraysTable->updateGeometry();
  }

  // populate timesteps info.
  void setTimeSteps(vtkSMProxy* source)
  {
    if (auto property = (source ? source->GetProperty("TimestepValues") : nullptr))
    {
      vtkSMPropertyHelper helper(property);
      std::vector<double> timesteps(helper.GetNumberOfElements());
      if (helper.GetNumberOfElements() > 0)
      {
        helper.Get(&timesteps[0], helper.GetNumberOfElements());
      };
      this->TimestepValuesModel.setTimeSteps(timesteps);
    }
    else
    {
      this->TimestepValuesModel.setTimeSteps({});
    }
    this->Ui.timeValues->header()->resizeSections(QHeaderView::ResizeToContents);
  }

  // update data statistics widgets
  void setDataStatistics(vtkPVDataInformation* dinfo)
  {
    auto l = QLocale::system();
    auto& ui = this->Ui;
    ui.dataType->setEnabled(dinfo && !dinfo->IsNull());
    ui.labelDataType->setEnabled(dinfo && !dinfo->IsNull());
    ui.dataType->setText(dinfo ? dinfo->GetPrettyDataTypeString() : "(n/a)");
    ui.labelDataStatistics->setText((dinfo && dinfo->GetNumberOfDataSets() > 1)
        ? QString("Data Statistics (# of datasets: %1)")
            .arg(l.toString(dinfo->GetNumberOfDataSets()))
        : QString("Data Statistics"));

    std::map<int, std::pair<QLabel*, QLabel*> > labelMap;
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
      }
      else
      {
        item.second.second->setText("(n/a)");
        item.second.first->setVisible(false);
        item.second.second->setVisible(false);
      }
    }

    ui.memory->setEnabled(dinfo != nullptr);
    ui.labelMemory->setEnabled(dinfo != nullptr);
    ui.memory->setText(dinfo ? formatMemory(dinfo->GetMemorySize()) : QString("(n/a)"));

    if (dinfo && vtkMath::AreBoundsInitialized(dinfo->GetBounds()))
    {
      double bds[6];
      dinfo->GetBounds(bds);
      ui.bounds->setText(QString("%1 to %2 (delta: %3)\n"
                                 "%4 to %5 (delta: %6)\n"
                                 "%7 to %8 (delta: %9)")
                           .arg(bds[0])
                           .arg(bds[1])
                           .arg(bds[1] - bds[0])
                           .arg(bds[2])
                           .arg(bds[3])
                           .arg(bds[3] - bds[1])
                           .arg(bds[4])
                           .arg(bds[5])
                           .arg(bds[5] - bds[4]));
      ui.labelBounds->setVisible(true);
      ui.bounds->setVisible(true);
    }
    else
    {
      ui.bounds->setText("(n/a)\n(n/a)\n(n/a)");
      ui.labelBounds->setVisible(false);
      ui.bounds->setVisible(false);
    }

    if (dinfo && dinfo->GetExtent()[0] <= dinfo->GetExtent()[1])
    {
      int exts[6];
      dinfo->GetExtent(exts);
      ui.extents->setText(QString("%1 to %2 (dimension: %3)\n"
                                  "%4 to %5 (dimension: %6)\n"
                                  "%7 to %8 (dimension: %9)")
                            .arg(l.toString(exts[0]))
                            .arg(l.toString(exts[1]))
                            .arg(l.toString(exts[1] - exts[0] + 1))
                            .arg(l.toString(exts[2]))
                            .arg(l.toString(exts[3]))
                            .arg(l.toString(exts[3] - exts[1] + 1))
                            .arg(l.toString(exts[4]))
                            .arg(l.toString(exts[5]))
                            .arg(l.toString(exts[5] - exts[4] + 1)));

      ui.labelExtents->setVisible(true);
      ui.extents->setVisible(true);
    }
    else
    {
      ui.extents->setText("(n/a)\n(n/a)\n(n/a)");
      ui.labelExtents->setVisible(false);
      ui.extents->setVisible(false);
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
pqProxyInformationWidget::~pqProxyInformationWidget()
{
}

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
  internals.setTimeSteps(proxy);

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
