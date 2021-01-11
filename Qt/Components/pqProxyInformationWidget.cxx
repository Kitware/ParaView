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

// this include
#include "pqProxyInformationWidget.h"
#include "ui_pqProxyInformationWidget.h"

// Qt includes
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QStackedWidget>
#include <QStringList>
#include <QTabBar>

// VTK includes
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include <vtksys/SystemTools.hxx>

// ParaView Server Manager includes
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVLogger.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSmartPointer.h"

#include "vtkPVGeneralSettings.h"

// ParaView widget includes

// ParaView core includes
#include "pqActiveObjects.h"
#include "pqCompositeDataInformationTreeModel.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDoubleLineEdit.h"
#include "pqNonEditableStyledItemDelegate.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"

// ParaView components includes

static QString formatTime(double time)
{
  auto settings = vtkPVGeneralSettings::GetInstance();
  const auto precision = settings->GetAnimationTimePrecision();
  const auto notation =
    static_cast<pqDoubleLineEdit::RealNumberNotation>(settings->GetAnimationTimeNotation());
  return pqDoubleLineEdit::formatDouble(time, notation, precision);
}

class pqProxyInformationWidget::pqUi : public Ui::pqProxyInformationWidget
{
public:
  pqUi(QObject* p)
    : compositeTreeModel(new pqCompositeDataInformationTreeModel(p))
    , assemblyTreeModel(new pqDataAssemblyTreeModel(p))
  {
  }

  QPointer<pqCompositeDataInformationTreeModel> compositeTreeModel;
  QPointer<pqDataAssemblyTreeModel> assemblyTreeModel;
};

//-----------------------------------------------------------------------------
pqProxyInformationWidget::pqProxyInformationWidget(QWidget* p)
  : QWidget(p)
  , OutputPort(NULL)
{
  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->Ui = new pqUi(this);
  this->Ui->setupUi(this);
  this->Ui->dataArrays->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  this->Ui->timeValues->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  this->Ui->compositeTree->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  this->Ui->compositeTree->setModel(this->Ui->compositeTreeModel);
  this->connect(this->Ui->compositeTree->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
    SLOT(onCurrentChanged(const QModelIndex&)));
  this->Ui->assemblyTree->setModel(this->Ui->assemblyTreeModel);
  this->connect(this->Ui->dataTypeProperties, SIGNAL(currentChanged(int)),
    SLOT(onDataTypePropertiesWidgetChanged(int)));
  this->onDataTypePropertiesWidgetChanged(0); // set initial size
  this->updateInformation();                  // initialize state.

  this->connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)), this,
    SLOT(setOutputPort(pqOutputPort*)));

  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(updateInformation()));
}

//-----------------------------------------------------------------------------
pqProxyInformationWidget::~pqProxyInformationWidget()
{
  this->VTKConnect->Disconnect();
  this->VTKConnect->Delete();
  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::setOutputPort(pqOutputPort* source)
{
  if (this->OutputPort == source)
  {
    return;
  }

  this->VTKConnect->Disconnect();
  if (this->OutputPort)
  {
    QObject::disconnect(this->OutputPort->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), this,
      SLOT(updateInformation()));
  }

  this->OutputPort = source;

  if (this->OutputPort)
  {
    QObject::connect(this->OutputPort->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), this,
      SLOT(updateInformation()));
  }

  this->updateInformation();
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqOutputPort* pqProxyInformationWidget::getOutputPort()
{
  return this->OutputPort;
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::updateInformation()
{
  this->Ui->compositeTreeModel->reset(nullptr);
  this->Ui->hierarchyTabWidget->setVisible(false);
  this->Ui->filename->setText(tr("NA"));
  this->Ui->filename->setToolTip(tr("NA"));
  this->Ui->filename->setStatusTip(tr("NA"));
  this->Ui->path->setText(tr("NA"));
  this->Ui->path->setToolTip(tr("NA"));
  this->Ui->path->setStatusTip(tr("NA"));

  vtkPVDataInformation* dataInformation = NULL;
  pqPipelineSource* source = NULL;
  vtkDataAssembly* assembly = nullptr;
  if (this->OutputPort)
  {
    source = this->OutputPort->getSource();
    if (this->OutputPort->getOutputPortProxy())
    {
      dataInformation = this->OutputPort->getDataInformation();
      assembly = this->OutputPort->dataAssembly();
    }
  }

  if (!source || !dataInformation)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "update-information-panel (nullptr)");
    this->fillDataInformation(0);
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "update-information-panel for `%s`",
    source->getProxy()->GetLogNameOrDefault());

  if (this->Ui->compositeTreeModel->reset(dataInformation))
  {
    this->Ui->hierarchyTabWidget->setVisible(true);
    this->Ui->compositeTree->expandToDepth(1);
    this->Ui->compositeTree->selectionModel()->setCurrentIndex(
      this->Ui->compositeTreeModel->rootIndex(), QItemSelectionModel::ClearAndSelect);
    this->Ui->assemblyTreeModel->setDataAssembly(assembly);
    this->Ui->assemblyTree->expandToDepth(1);

    if (assembly == nullptr)
    {
      this->Ui->hierarchyTabWidget->setCurrentIndex(0);
      this->Ui->hierarchyTabWidget->tabBar()->hide();
    }
    else
    {
      this->Ui->hierarchyTabWidget->tabBar()->show();
    }
  }

  this->fillDataInformation(dataInformation);

  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(source->getProxy()->NewPropertyIterator());
  for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
  {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop->IsA("vtkSMStringVectorProperty"))
    {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      for (diter->Begin(); !diter->IsAtEnd(); diter->Next())
      {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
        {
          vtkSMProperty* smprop = piter->GetProperty();
          if (smprop->GetInformationProperty())
          {
            smprop = smprop->GetInformationProperty();
            source->getProxy()->UpdatePropertyInformation(smprop);
          }
          QString filename = pqSMAdaptor::getElementProperty(smprop).toString();
          QString path = vtksys::SystemTools::GetFilenamePath(filename.toUtf8().data()).c_str();

          this->Ui->properties->show();
          this->Ui->filename->setText(
            vtksys::SystemTools::GetFilenameName(filename.toUtf8().data()).c_str());
          this->Ui->filename->setToolTip(filename);
          this->Ui->filename->setStatusTip(filename);
          this->Ui->path->setText(path);
          this->Ui->path->setToolTip(path);
          this->Ui->path->setStatusTip(path);
          break;
        }
      }
      if (!diter->IsAtEnd())
      {
        break;
      }
    }
  }

  // Check if there are timestep values. If yes, display them.
  vtkSMDoubleVectorProperty* tsv =
    vtkSMDoubleVectorProperty::SafeDownCast(source->getProxy()->GetProperty("TimestepValues"));
  this->Ui->timeValues->clear();
  //
  QAbstractItemModel* pModel = this->Ui->timeValues->model();
  pModel->blockSignals(true);
  this->Ui->timeValues->blockSignals(true);
  //

  if (tsv)
  {
    const unsigned int numElems = tsv->GetNumberOfElements();
    for (unsigned int i = 0; i < numElems; i++)
    {
      QTreeWidgetItem* item = new QTreeWidgetItem(this->Ui->timeValues);
      item->setData(0, Qt::DisplayRole, i);
      auto label = ::formatTime(tsv->GetElement(i));
      item->setData(1, Qt::DisplayRole, label);
      item->setData(1, Qt::ToolTipRole, label);
      item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
  }
  this->Ui->timeValues->blockSignals(false);
  pModel->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::fillDataInformation(vtkPVDataInformation* dataInformation)
{
  this->Ui->properties->setVisible(false);
  // out with the old
  this->Ui->type->setText(tr("NA"));
  this->Ui->numberOfCells->setText(tr("NA"));
  this->Ui->numberOfPoints->setText(tr("NA"));
  this->Ui->numberOfRows->setText(tr("NA"));
  this->Ui->numberOfColumns->setText(tr("NA"));
  this->Ui->numberOfTrees->setText(tr("NA"));
  this->Ui->numberOfVertices->setText(tr("NA"));
  this->Ui->numberOfLeaves->setText(tr("NA"));
  this->Ui->numberOfGraphVertices->setText(tr("NA"));
  this->Ui->numberOfGraphEdges->setText(tr("NA"));
  this->Ui->memory->setText(tr("NA"));

  this->Ui->dataArrays->clear();

  this->Ui->xRange->setText(tr("NA"));
  this->Ui->yRange->setText(tr("NA"));
  this->Ui->zRange->setText(tr("NA"));

  this->Ui->groupExtent->setVisible(false);
  this->Ui->dataTimeLabel->setVisible(false);

  // if dataInformation->GetNumberOfDataSets() == 0, that means that the data
  // information does not have any valid values.
  if (!dataInformation || dataInformation->GetNumberOfDataSets() == 0)
  {
    return;
  }

  this->Ui->type->setText(tr(dataInformation->GetPrettyDataTypeString()));

  QString numCells = QString("%1").arg(dataInformation->GetNumberOfCells());
  this->Ui->numberOfCells->setText(numCells);

  QString numPoints = QString("%1").arg(dataInformation->GetNumberOfPoints());
  this->Ui->numberOfPoints->setText(numPoints);

  QString numRows = QString("%1").arg(dataInformation->GetNumberOfRows());
  this->Ui->numberOfRows->setText(numRows);

  QString numColumns =
    QString("%1").arg(dataInformation->GetRowDataInformation()->GetNumberOfArrays());
  this->Ui->numberOfColumns->setText(numColumns);

  QString numTrees = QString("%1").arg(dataInformation->GetNumberOfTrees());
  this->Ui->numberOfTrees->setText(numTrees);

  QString numVertices = QString("%1").arg(dataInformation->GetNumberOfVertices());
  this->Ui->numberOfVertices->setText(numVertices);

  QString numLeaves = QString("%1").arg(dataInformation->GetNumberOfLeaves());
  this->Ui->numberOfLeaves->setText(numLeaves);

  this->Ui->numberOfGraphVertices->setText(numVertices);

  QString numBonds = QString("%1").arg(dataInformation->GetNumberOfEdges());
  this->Ui->numberOfGraphEdges->setText(numBonds);

  switch (dataInformation->GetDataSetType())
  {
    case VTK_TABLE:
      this->Ui->dataTypeProperties->setCurrentWidget(this->Ui->Table);
      break;

    case VTK_HYPER_TREE_GRID:
      this->Ui->dataTypeProperties->setCurrentWidget(this->Ui->HyperTreeGrid);
      break;

    case VTK_DIRECTED_GRAPH:
    case VTK_UNDIRECTED_GRAPH:
    case VTK_GRAPH:
      this->Ui->graphVertexLabel->setText("Number of Vertices");
      this->Ui->graphEdgeLabel->setText("Number of Edges");
      this->Ui->dataTypeProperties->setCurrentWidget(this->Ui->Graph);
      break;

    case VTK_MOLECULE:
      this->Ui->graphVertexLabel->setText("Number of Atoms");
      this->Ui->graphEdgeLabel->setText("Number of Bonds");
      this->Ui->dataTypeProperties->setCurrentWidget(this->Ui->Graph);
      break;
    default:
      this->Ui->dataTypeProperties->setCurrentWidget(this->Ui->DataSet);
      break;
  }

  QString memory = QString("%1 MB").arg(dataInformation->GetMemorySize() / 1000.0, 0, 'g', 2);
  this->Ui->memory->setText(memory);

  if (dataInformation->GetHasTime())
  {
    this->Ui->dataTimeLabel->setVisible(true);
    const char* timeLabel =
      dataInformation->GetTimeLabel() ? dataInformation->GetTimeLabel() : "time";
    auto capitalizedTimeLabel = vtksys::SystemTools::Capitalized(timeLabel);
    this->Ui->dataTimeLabel->setText(
      QString("Current data %2: %1").arg(::formatTime(dataInformation->GetTime())).arg(timeLabel));
    this->Ui->groupDataTime->setTitle(capitalizedTimeLabel.c_str());
    this->Ui->timeValues->setHeaderLabels({ "Index", capitalizedTimeLabel.c_str() });
  }
  else
  {
    this->Ui->groupDataTime->setTitle(tr("Time"));
    this->Ui->timeValues->setHeaderLabels({ "Index", "Time" });
  }

  vtkPVDataSetAttributesInformation* info[6];
  info[0] = dataInformation->GetPointDataInformation();
  info[1] = dataInformation->GetCellDataInformation();
  info[2] = dataInformation->GetVertexDataInformation();
  info[3] = dataInformation->GetEdgeDataInformation();
  info[4] = dataInformation->GetRowDataInformation();
  info[5] = dataInformation->GetFieldDataInformation();

  QIcon pixmaps[6] = { QIcon(":/pqWidgets/Icons/pqPointData.svg"),
    QIcon(":/pqWidgets/Icons/pqCellData.svg"), QIcon(":/pqWidgets/Icons/pqPointData.svg"),
    QIcon(":/pqWidgets/Icons/pqCellData.svg"), QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg"),
    QIcon(":/pqWidgets/Icons/pqGlobalData.svg") };

  if (dataInformation->IsDataStructured())
  {
    this->Ui->groupExtent->setVisible(true);
    int ext[6];
    dataInformation->GetExtent(ext);
    if (ext[1] >= ext[0] && ext[3] >= ext[2] && ext[5] >= ext[4])
    {
      int dims[3];
      dims[0] = ext[1] - ext[0] + 1;
      dims[1] = ext[3] - ext[2] + 1;
      dims[2] = ext[5] - ext[4] + 1;

      this->Ui->xExtent->setText(
        QString("%1 to %2 (dimension: %3)").arg(ext[0]).arg(ext[1]).arg(dims[0]));
      this->Ui->yExtent->setText(
        QString("%1 to %2 (dimension: %3)").arg(ext[2]).arg(ext[3]).arg(dims[1]));
      this->Ui->zExtent->setText(
        QString("%1 to %2 (dimension: %3)").arg(ext[4]).arg(ext[5]).arg(dims[2]));
    }
    else
    {
      this->Ui->xExtent->setText(tr("NA"));
      this->Ui->yExtent->setText(tr("NA"));
      this->Ui->zExtent->setText(tr("NA"));
    }
  }

  for (int k = 0; k < 6; k++)
  {
    if (info[k])
    {
      int numArrays = info[k]->GetNumberOfArrays();
      for (int i = 0; i < numArrays; i++)
      {
        vtkPVArrayInformation* arrayInfo;
        arrayInfo = info[k]->GetArrayInformation(i);
        // name, type, data type, data range
        QTreeWidgetItem* item = new QTreeWidgetItem(this->Ui->dataArrays);
        item->setData(0, Qt::DisplayRole, arrayInfo->GetName());
        item->setData(0, Qt::DecorationRole, pixmaps[k]);
        item->setData(1, Qt::DisplayRole, vtkImageScalarTypeNameMacro(arrayInfo->GetDataType()));
        int numComponents = arrayInfo->GetNumberOfComponents();
        if (arrayInfo->GetDataType() == VTK_STRING)
        {
          QStringList values;
          for (int j = 0; j < arrayInfo->GetNumberOfStringValues(); ++j)
          {
            values.push_back(arrayInfo->GetStringValue(j));
          }
          auto csv = values.join(",");
          item->setData(2, Qt::DisplayRole, csv);
          item->setData(2, Qt::ToolTipRole, csv);
        }
        else
        {
          QString dataRange;
          double range[2];
          for (int j = 0; j < numComponents; j++)
          {
            if (j != 0)
            {
              dataRange.append(", ");
            }
            arrayInfo->GetComponentRange(j, range);
            QString componentRange = QString("[%1, %2]").arg(range[0]).arg(range[1]);
            dataRange.append(componentRange);
          }
          item->setData(2, Qt::DisplayRole, dataRange);
          item->setData(2, Qt::ToolTipRole, dataRange);
        }
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        if (arrayInfo->GetIsPartial())
        {
          item->setForeground(0, QBrush(QColor("darkBlue")));
          item->setData(0, Qt::DisplayRole, QString("%1 (partial)").arg(arrayInfo->GetName()));
        }
        else
        {
          item->setForeground(0, QBrush(QColor("darkGreen")));
        }
      }
    }
  }
  this->Ui->dataArrays->header()->resizeSections(QHeaderView::ResizeToContents);

  double bounds[6];
  dataInformation->GetBounds(bounds);
  QString xrange;
  if (bounds[0] == VTK_DOUBLE_MAX && bounds[1] == -VTK_DOUBLE_MAX)
  {
    xrange = tr("Not available");
  }
  else
  {
    xrange = QString("%1 to %2 (delta: %3)");
    xrange = xrange.arg(bounds[0], -1, 'g', 6);
    xrange = xrange.arg(bounds[1], -1, 'g', 6);
    xrange = xrange.arg(bounds[1] - bounds[0], -1, 'g', 6);
  }
  this->Ui->xRange->setText(xrange);

  QString yrange;
  if (bounds[2] == VTK_DOUBLE_MAX && bounds[3] == -VTK_DOUBLE_MAX)
  {
    yrange = tr("Not available");
  }
  else
  {
    yrange = QString("%1 to %2 (delta: %3)");
    yrange = yrange.arg(bounds[2], -1, 'g', 6);
    yrange = yrange.arg(bounds[3], -1, 'g', 6);
    yrange = yrange.arg(bounds[3] - bounds[2], -1, 'g', 6);
  }
  this->Ui->yRange->setText(yrange);

  QString zrange;
  if (bounds[4] == VTK_DOUBLE_MAX && bounds[5] == -VTK_DOUBLE_MAX)
  {
    zrange = tr("Not available");
  }
  else
  {
    zrange = QString("%1 to %2 (delta: %3)");
    zrange = zrange.arg(bounds[4], -1, 'g', 6);
    zrange = zrange.arg(bounds[5], -1, 'g', 6);
    zrange = zrange.arg(bounds[5] - bounds[4], -1, 'g', 6);
  }
  this->Ui->zRange->setText(zrange);
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::onCurrentChanged(const QModelIndex& idx)
{
  vtkPVDataInformation* dataInformation =
    this->OutputPort ? this->OutputPort->getDataInformation() : nullptr;
  if (dataInformation && idx.isValid())
  {
    unsigned int cid = this->Ui->compositeTreeModel->compositeIndex(idx);
    vtkPVDataInformation* info = dataInformation->GetDataInformationForCompositeIndex(cid);
    this->fillDataInformation(info);
  }
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::onDataTypePropertiesWidgetChanged(int idx)
{
  QStackedWidget* stack = this->Ui->dataTypeProperties;
  for (int i = 0; i < stack->count(); ++i)
  {
    QSizePolicy::Policy policy = i == idx ? QSizePolicy::Expanding : QSizePolicy::Ignored;
    stack->widget(i)->setSizePolicy(policy, policy);
  }
}
