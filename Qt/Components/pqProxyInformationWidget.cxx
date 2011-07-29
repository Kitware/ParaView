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
#include <QStringList>
#include <QTimer>

// VTK includes
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include <vtksys/SystemTools.hxx>

// ParaView Server Manager includes
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyIterator.h"

// ParaView widget includes

// ParaView core includes
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"

// ParaView components includes


class pqProxyInformationWidget::pqUi 
  : public QObject, public Ui::pqProxyInformationWidget
{
public:
  pqUi(QObject* p) : QObject(p) {}
};

//-----------------------------------------------------------------------------
pqProxyInformationWidget::pqProxyInformationWidget(QWidget* p)
  : QWidget(p), OutputPort(NULL)
{
  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->Ui = new pqUi(this);
  this->Ui->setupUi(this);
  QObject::connect(this->Ui->compositeTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
    this, SLOT(onItemClicked(QTreeWidgetItem*)), Qt::QueuedConnection);
  this->updateInformation(); // initialize state.
}

//-----------------------------------------------------------------------------
pqProxyInformationWidget::~pqProxyInformationWidget()
{
  this->VTKConnect->Disconnect();
  this->VTKConnect->Delete();
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
    QObject::disconnect(this->OutputPort->getSource(), 
        SIGNAL(dataUpdated(pqPipelineSource*)),
        this, SLOT(updateInformation()));
    }

  this->OutputPort = source;
  
  if (this->OutputPort)
    {
    QObject::connect(this->OutputPort->getSource(), 
        SIGNAL(dataUpdated(pqPipelineSource*)),
        this, SLOT(updateInformation()));
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
  this->Ui->compositeTree->clear();
  this->Ui->compositeTree->setVisible(false);
  this->Ui->filename->setText(tr("NA"));
  this->Ui->filename->setToolTip(tr("NA"));
  this->Ui->filename->setStatusTip(tr("NA"));
  this->Ui->path->setText(tr("NA"));
  this->Ui->path->setToolTip(tr("NA"));
  this->Ui->path->setStatusTip(tr("NA"));

  vtkPVDataInformation* dataInformation = NULL;
  pqPipelineSource* source = NULL;
  if(this->OutputPort)
    {
    source = this->OutputPort->getSource();
    if (this->OutputPort->getOutputPortProxy())
      {
      dataInformation = this->OutputPort->getDataInformation();
      }
    }


  if (!source || !dataInformation)
    {
    this->fillDataInformation(0);
    return;
    }

  vtkPVCompositeDataInformation* compositeInformation = 
    dataInformation->GetCompositeDataInformation();

  if (compositeInformation->GetDataIsComposite())
    {
    QTreeWidgetItem* root = this->fillCompositeInformation(dataInformation);
    this->Ui->compositeTree->setVisible(true);
    root->setExpanded(true);
    root->setSelected(true);
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
      for(diter->Begin(); !diter->IsAtEnd(); diter->Next())
        {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
          {
          vtkSMProperty* smprop = piter->GetProperty();
          if (smprop->GetInformationProperty())
            {
            smprop = smprop->GetInformationProperty();
            source->getProxy()->UpdatePropertyInformation(smprop);
            }
          QString filename = 
            pqSMAdaptor::getElementProperty(smprop).toString();
          QString path = vtksys::SystemTools::GetFilenamePath(
            filename.toAscii().data()).c_str();

          this->Ui->properties->show();
          this->Ui->filename->setText(vtksys::SystemTools::GetFilenameName(
              filename.toAscii().data()).c_str());
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
  vtkSMDoubleVectorProperty* tsv = vtkSMDoubleVectorProperty::SafeDownCast(
    source->getProxy()->GetProperty("TimestepValues"));
  this->Ui->timeValues->clear();
  //
  QAbstractItemModel *pModel = this->Ui->timeValues->model();
  pModel->blockSignals(true);
  this->Ui->timeValues->blockSignals(true);
  //
  if (tsv)
    {
    unsigned int numElems = tsv->GetNumberOfElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      QTreeWidgetItem * item = new QTreeWidgetItem(this->Ui->timeValues);
      item->setData(0, Qt::DisplayRole, i);
      item->setData(1, Qt::DisplayRole, tsv->GetElement(i));
      item->setData(1, Qt::ToolTipRole, tsv->GetElement(i));
      }
    }
  this->Ui->timeValues->blockSignals(false);
  pModel->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::fillDataInformation(
  vtkPVDataInformation* dataInformation)
{
  this->Ui->properties->setVisible(false);
  // out with the old
  this->Ui->type->setText(tr("NA"));
  this->Ui->numberOfCells->setText(tr("NA"));
  this->Ui->numberOfPoints->setText(tr("NA"));
  this->Ui->numberOfRows->setText(tr("NA"));
  this->Ui->numberOfColumns->setText(tr("NA"));
  this->Ui->memory->setText(tr("NA"));
  
  this->Ui->dataArrays->clear();

  this->Ui->xRange->setText(tr("NA"));
  this->Ui->yRange->setText(tr("NA"));
  this->Ui->zRange->setText(tr("NA"));

  this->Ui->groupExtent->setVisible(false);
  this->Ui->dataTimeLabel->setVisible(false);

  // if dataInformation->GetNumberOfDataSets() == 0, that means that the data
  // information does not have any valid values.
  if (!dataInformation || dataInformation->GetNumberOfDataSets()==0)
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

  if (dataInformation->GetDataSetType() == VTK_TABLE)
    {
    this->Ui->dataTypeProperties->setCurrentWidget(
      this->Ui->Table);
    }
  else
    {
    this->Ui->dataTypeProperties->setCurrentWidget(
      this->Ui->DataSet);
    }
  
  QString memory = QString("%1 MB").arg(dataInformation->GetMemorySize()/1000.0,
                                     0, 'g', 2);
  this->Ui->memory->setText(memory);

  if (dataInformation->GetHasTime())
    {
    this->Ui->dataTimeLabel->setVisible(true);
    this->Ui->dataTimeLabel->setText(
      QString("Current data time: <b>%1</b>").arg(dataInformation->GetTime()));
    }

  vtkPVDataSetAttributesInformation* info[6];
  info[0] = dataInformation->GetPointDataInformation();
  info[1] = dataInformation->GetCellDataInformation();
  info[2] = dataInformation->GetVertexDataInformation();
  info[3] = dataInformation->GetEdgeDataInformation();
  info[4] = dataInformation->GetRowDataInformation();
  info[5] = dataInformation->GetFieldDataInformation();

  QPixmap pixmaps[6] = 
    {
    QPixmap(":/pqWidgets/Icons/pqPointData16.png"),
    QPixmap(":/pqWidgets/Icons/pqCellData16.png"),
    QPixmap(":/pqWidgets/Icons/pqPointData16.png"),
    QPixmap(":/pqWidgets/Icons/pqCellData16.png"),
    QPixmap(":/pqWidgets/Icons/pqSpreadsheet16.png"),
    QPixmap(":/pqWidgets/Icons/pqGlobalData16.png")
    };

  if (dataInformation->IsDataStructured())
    {
    this->Ui->groupExtent->setVisible(true);
    int ext[6];
    dataInformation->GetExtent(ext);
    if (ext[1]>=ext[0] && ext[3] >=ext[2] && ext[5]>=ext[4])
      {
      int dims[3];
      dims[0] = ext[1]-ext[0]+1;
      dims[1] = ext[3]-ext[2]+1;
      dims[2] = ext[5]-ext[4]+1;

      this->Ui->xExtent->setText(QString(
          "%1 to %2 (dimension: %3)").arg(ext[0]).arg(ext[1]).arg(dims[0]));
      this->Ui->yExtent->setText(QString(
          "%1 to %2 (dimension: %3)").arg(ext[2]).arg(ext[3]).arg(dims[1]));
      this->Ui->zExtent->setText(QString(
          "%1 to %2 (dimension: %3)").arg(ext[4]).arg(ext[5]).arg(dims[2]));
      }
    else
      {
      this->Ui->xExtent->setText("NA");
      this->Ui->yExtent->setText("NA");
      this->Ui->zExtent->setText("NA");
      }
    }

  for(int k=0; k<6; k++)
    {
    if(info[k])
      {
      int numArrays = info[k]->GetNumberOfArrays();
      for(int i=0; i<numArrays; i++)
        {
        vtkPVArrayInformation* arrayInfo;
        arrayInfo = info[k]->GetArrayInformation(i);
        // name, type, data type, data range
        QTreeWidgetItem * item = new QTreeWidgetItem(this->Ui->dataArrays);
        item->setData(0, Qt::DisplayRole, arrayInfo->GetName());
        item->setData(0, Qt::DecorationRole, pixmaps[k]);
        QString dataType = vtkImageScalarTypeNameMacro(arrayInfo->GetDataType());
        item->setData(1, Qt::DisplayRole, dataType);
        int numComponents = arrayInfo->GetNumberOfComponents();
        QString dataRange;
        double range[2];
        for(int j=0; j<numComponents; j++)
          {
          if(j != 0)
            {
            dataRange.append(", ");
            }
          arrayInfo->GetComponentRange(j, range);
          QString componentRange = QString("[%1, %2]").
                             arg(range[0]).arg(range[1]);
          dataRange.append(componentRange);
          }
        item->setData(2, Qt::DisplayRole, dataRange);
        item->setData(2, Qt::ToolTipRole, dataRange);
        if (arrayInfo->GetIsPartial())
          {
          item->setForeground(0, QBrush(QColor("darkBlue")));
          item->setData(0, Qt::DisplayRole, 
            QString("%1 (partial)").arg(arrayInfo->GetName()));
          }
        else
          {
          item->setForeground(0, QBrush(QColor("darkGreen")));
          }
        }
      }
    }
  this->Ui->dataArrays->header()->resizeSections(
    QHeaderView::ResizeToContents);

  double bounds[6];
  dataInformation->GetBounds(bounds);
  QString xrange;
  if (bounds[0] == VTK_DOUBLE_MAX && bounds[1] == -VTK_DOUBLE_MAX)
    {
    xrange = "Not available";
    }
  else
    {
    xrange = QString("%1 to %2 (delta: %3)");
    xrange = xrange.arg(bounds[0], -1, 'g', 3);
    xrange = xrange.arg(bounds[1], -1, 'g', 3);
    xrange = xrange.arg(bounds[1] - bounds[0], -1, 'g', 3);
    }
  this->Ui->xRange->setText(xrange);

  QString yrange;
  if (bounds[2] == VTK_DOUBLE_MAX && bounds[3] == -VTK_DOUBLE_MAX)
    {
    yrange = "Not available";
    }
  else
    {
    yrange = QString("%1 to %2 (delta: %3)");
    yrange = yrange.arg(bounds[2], -1, 'g', 3);
    yrange = yrange.arg(bounds[3], -1, 'g', 3);
    yrange = yrange.arg(bounds[3] - bounds[2], -1, 'g', 3);
    }
  this->Ui->yRange->setText(yrange);

  QString zrange;
  if (bounds[4] == VTK_DOUBLE_MAX && bounds[5] == -VTK_DOUBLE_MAX)
    {
    zrange = "Not available";
    }
  else
    {
    zrange = QString("%1 to %2 (delta: %3)");
    zrange = zrange.arg(bounds[4], -1, 'g', 3);
    zrange = zrange.arg(bounds[5], -1, 'g', 3);
    zrange = zrange.arg(bounds[5] - bounds[4], -1, 'g', 3);
    }
  this->Ui->zRange->setText(zrange);
}

//-----------------------------------------------------------------------------
QTreeWidgetItem* pqProxyInformationWidget::fillCompositeInformation(
  vtkPVDataInformation* info, QTreeWidgetItem* parentItem/*=0*/)
{
  QTreeWidgetItem* node = 0;

  QString label = info? info->GetPrettyDataTypeString() : "NA";
  if (parentItem)
    {
    node = new QTreeWidgetItem(parentItem, QStringList(label));
    }
  else
    {
    node = new QTreeWidgetItem(this->Ui->compositeTree, QStringList(label));
    }
  if (!info)
    {
    return node;
    }

  // we save a ptr to the data information to easily locate the data
  // information.
  node->setData(0, Qt::UserRole, QVariant::fromValue((void*)info));

  vtkPVCompositeDataInformation* compositeInformation = 
    info->GetCompositeDataInformation();

  if (!compositeInformation->GetDataIsComposite() || 
    compositeInformation->GetDataIsMultiPiece())
    {
    return node;
    }

  bool isHB = 
    (strcmp(info->GetCompositeDataClassName(),"vtkHierarchicalBoxDataSet") ==0);

  unsigned int numChildren = compositeInformation->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkPVDataInformation* childInfo = 
      compositeInformation->GetDataInformation(cc);
    QTreeWidgetItem* childItem = 
      this->fillCompositeInformation(childInfo, node);
    const char* name = compositeInformation->GetName(cc);
    if (name && name[0])
      {
      // use name given to the block.
      childItem->setText(0, name);
      }
    else if (isHB)
      {
      childItem->setText(0, QString("Level %1").arg(cc));
      }
    else if (childInfo && childInfo->GetCompositeDataClassName())
      {
      childItem->setText(0, QString("Block %1").arg(cc));
      }
    else
      {
      // Use the data classname in the name for the leaf node.
      childItem->setText(0, QString("%1: %2").arg(cc).arg(childItem->text(0)));
      }
    }

  return node;
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::onItemClicked(QTreeWidgetItem* item)
{
  vtkPVDataInformation* info = reinterpret_cast<vtkPVDataInformation*>(
    item->data(0, Qt::UserRole).value<void*>());
  this->fillDataInformation(info);
}
