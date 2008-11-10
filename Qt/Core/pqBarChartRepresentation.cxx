/*=========================================================================

   Program: ParaView
   Module:    pqBarChartRepresentation.cxx

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
#include "pqBarChartRepresentation.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTable.h"
#include "vtkTimeStamp.h"

#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"

//-----------------------------------------------------------------------------
class pqBarChartRepresentation::pqInternals
{
public:
  vtkTimeStamp MTime;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqBarChartRepresentation::pqBarChartRepresentation(const QString& group, const QString& name,
  vtkSMProxy* display, pqServer* server, QObject* _parent)
: Superclass(group, name, display, server, _parent)
{
  this->Internal = new pqInternals();
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->VTKConnect->Connect(display, vtkCommand::PropertyModifiedEvent,
    this, SLOT(markModified()), 0, 0, Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqBarChartRepresentation::~pqBarChartRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqBarChartRepresentation::markModified()
{
  this->Internal->MTime.Modified();
}

//-----------------------------------------------------------------------------
vtkTimeStamp pqBarChartRepresentation::getMTime() const
{
  vtkTable* data = this->getClientSideData();
  if (data && data->GetMTime() > this->Internal->MTime)
    {
    this->Internal->MTime.Modified();
    }

  return this->Internal->MTime;
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqBarChartRepresentation::setLookupTable(const char* arrayname)
{
  // Now set up default lookup table.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;
  pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
    this->getServer(), arrayname, 1, 0);
  lut = (pqlut)? pqlut->getProxy() : 0;

  vtkSMProxy* proxy = this->getProxy();
  vtkSMPropertyHelper(proxy, "LookupTable").Set(lut);
  proxy->UpdateVTKObjects();

  return pqlut;
}

//-----------------------------------------------------------------------------
void pqBarChartRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  if (!this->isVisible())
    {
    // For any non-visible display, we don't set its defaults.
    return;
    }

  // Set default arrays and lookup table.
  vtkSMProxy* proxy = this->getProxy();
 
  vtkPVDataInformation* input_di = this->getInputDataInformation();
  if (input_di)
    {
    int field_association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    switch (input_di->GetDataSetType())
      {
    case VTK_TABLE:
      field_association = vtkDataObject::FIELD_ASSOCIATION_ROWS;
      break;
     
    case VTK_GRAPH:
      field_association = vtkDataObject::FIELD_ASSOCIATION_VERTICES;
      break;
      }
    vtkSMPropertyHelper(proxy, "FieldAssociation").Set(field_association);
    }
  proxy->UpdateVTKObjects();

  // Need to update since we would have changed the field Association changed.
  vtkSMClientDeliveryRepresentationProxy::SafeDownCast(proxy)->Update();
  proxy->GetProperty("XArrayName")->ResetToDefault();
  proxy->GetProperty("YArrayName")->ResetToDefault();

  // Now initialize the lookup table.
  this->updateLookupTable();
}

//-----------------------------------------------------------------------------
void pqBarChartRepresentation::updateLookupTable()
{
  vtkDataArray* xarray  = this->getXArray();
  if (!xarray)
    {
    qDebug() << "Cannot set up lookup table, no X array.";
    return;
    }

  pqScalarsToColors* lut;
  // Now set up default lookup table.
  if (!xarray->GetName())
    {
    lut = this->setLookupTable("unnamedArray"); 
    }
  else
    {
    lut = this->setLookupTable(xarray->GetName());
    }
  if (lut)
    {
    // reset range of LUT respecting the range locks.
    double range[2];
    xarray->GetRange(range);
    lut->setWholeScalarRange(range[0], range[1]);
    }
}

//-----------------------------------------------------------------------------
void pqBarChartRepresentation::resetLookupTableScalarRange()
{
  vtkDataArray* xArray = this->getXArray();
  pqScalarsToColors* lut = this->getLookupTable();
  if(lut && xArray)
    {
    double range[2];
    xArray->GetRange(range);
    lut->setScalarRange(range[0], range[1]);
    }
}

//-----------------------------------------------------------------------------
vtkTable* pqBarChartRepresentation::getClientSideData() const
{
  vtkSMClientDeliveryRepresentationProxy* proxy = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkTable::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqBarChartRepresentation::getXArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkTable* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }

  return data->GetRowData()->GetArray(
    vtkSMPropertyHelper(proxy, "XArrayName").GetAsString());
}

//----------------------------------------------------------------------------
vtkDataArray* pqBarChartRepresentation::getYArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkTable* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }

  return data->GetRowData()->GetArray(
    vtkSMPropertyHelper(proxy, "YArrayName").GetAsString());
}

//----------------------------------------------------------------------------
int pqBarChartRepresentation::getXArrayComponent()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return 0;
    }

  return vtkSMPropertyHelper(proxy, "XArrayComponent").GetAsInt();
}

//----------------------------------------------------------------------------
int pqBarChartRepresentation::getYArrayComponent()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return 0;
    }

  return vtkSMPropertyHelper(proxy, "YArrayComponent").GetAsInt();
}
