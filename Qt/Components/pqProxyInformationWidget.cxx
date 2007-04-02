/*=========================================================================

   Program: ParaView
   Module:    pqProxyInformationWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

// VTK includes
#include <vtksys/SystemTools.hxx>

// ParaView Server Manager includes
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSourceProxy.h"

// ParaView widget includes

// ParaView core includes
#include "pqProxy.h"
#include "pqSMAdaptor.h"

// ParaView components includes


class pqProxyInformationWidget::pqUi 
  : public QObject, public Ui::pqProxyInformationWidget
{
public:
  pqUi(QObject* p) : QObject(p) {}
};

//-----------------------------------------------------------------------------
pqProxyInformationWidget::pqProxyInformationWidget(QWidget* p)
  : QWidget(p), Source(NULL)
{
  this->Ui = new pqUi(this);
  this->Ui->setupUi(this);
}

//-----------------------------------------------------------------------------
pqProxyInformationWidget::~pqProxyInformationWidget()
{
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::setProxy(pqProxy* source) 
{
  if(this->Source == source)
    {
    return;
    }

  this->Source = source;

  this->updateInformation();
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqProxy* pqProxyInformationWidget::getProxy()
{
  return this->Source;
}

//-----------------------------------------------------------------------------
void pqProxyInformationWidget::updateInformation()
{
  this->Ui->properties->setVisible(false);
  this->Ui->filename->setText(tr("NA"));
  this->Ui->filename->setToolTip(tr("NA"));
  this->Ui->filename->setStatusTip(tr("NA"));

  // out with the old
  this->Ui->type->setText(tr("NA"));
  this->Ui->numberOfCells->setText(tr("NA"));
  this->Ui->numberOfPoints->setText(tr("NA"));
  this->Ui->memory->setText(tr("NA"));
  
  this->Ui->dataArrays->clear();

  this->Ui->xRange->setText(tr("NA"));
  this->Ui->yRange->setText(tr("NA"));
  this->Ui->zRange->setText(tr("NA"));

  // in with the new
  vtkSMSourceProxy* sourceProxy = NULL;
  vtkPVDataInformation* dataInformation = NULL;
  if(this->Source)
    {
    sourceProxy = vtkSMSourceProxy::SafeDownCast(this->Source->getProxy());
    }
  // if source proxy, and if parts have been made 
  // ( never force the creation of parts by getting data information)
  if(sourceProxy && sourceProxy->GetNumberOfParts())
    {
    dataInformation = sourceProxy->GetDataInformation();
    }

  if(dataInformation)
    {
    this->Ui->type->setText(tr(dataInformation->GetPrettyDataTypeString()));
    
    QString numCells = QString("%1").arg(dataInformation->GetNumberOfCells());
    this->Ui->numberOfCells->setText(numCells);
    
    QString numPoints = QString("%1").arg(dataInformation->GetNumberOfPoints());
    this->Ui->numberOfPoints->setText(numPoints);
    
    QString memory = QString("%1 MB").arg(dataInformation->GetMemorySize()/1000.0,
                                       0, 'e', 2);
    this->Ui->memory->setText(memory);

    vtkPVDataSetAttributesInformation* info[2];
    info[0] = dataInformation->GetPointDataInformation();
    info[1] = dataInformation->GetCellDataInformation();
  
    QPixmap pixmaps[2] = 
      {
      QPixmap(":/pqWidgets/Icons/pqPointData16.png"),
      QPixmap(":/pqWidgets/Icons/pqCellData16.png")
      };

    for(int k=0; k<2; k++)
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
      xrange = xrange.arg(bounds[0], -1, 'f', 3);
      xrange = xrange.arg(bounds[1], -1, 'f', 3);
      xrange = xrange.arg(bounds[1] - bounds[0], -1, 'f', 3);
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
      yrange = yrange.arg(bounds[2], -1, 'f', 3);
      yrange = yrange.arg(bounds[3], -1, 'f', 3);
      yrange = yrange.arg(bounds[3] - bounds[2], -1, 'f', 3);
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
      zrange = zrange.arg(bounds[4], -1, 'f', 3);
      zrange = zrange.arg(bounds[5], -1, 'f', 3);
      zrange = zrange.arg(bounds[5] - bounds[4], -1, 'f', 3);
      }
    this->Ui->zRange->setText(zrange);

    }

  if (sourceProxy)
    {
    // Find the first property that has a vtkSMFileListDomain. Assume that
    // it is the property used to set the filename.
    vtkSmartPointer<vtkSMPropertyIterator> piter;
    piter.TakeReference(sourceProxy->NewPropertyIterator());
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
            QString filename = 
              pqSMAdaptor::getElementProperty(piter->GetProperty()).toString();
            this->Ui->properties->show();
            this->Ui->filename->setText(vtksys::SystemTools::GetFilenameName(
                filename.toAscii().data()).c_str());
            this->Ui->filename->setToolTip(filename);
            this->Ui->filename->setStatusTip(filename);
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
      sourceProxy->GetProperty("TimestepValues"));
    this->Ui->timeValues->clear();
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
    }
}

