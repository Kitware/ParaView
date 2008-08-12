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
#include "vtkEventQtSlotConnect.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimeStamp.h"

#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqSMAdaptor.h"

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
  vtkRectilinearGrid* data = this->getClientSideData();
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
  pqSMAdaptor::setProxyProperty(
    proxy->GetProperty("LookupTable"), lut);
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
  
  // By default, we use the 1st point array as the X axis. If no point data
  // is present we use the X coordinate of the points themselves.
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("XArrayName"));
  bool use_points = (svp->GetElement(0) == 0);
  pqSMAdaptor::setElementProperty(
    proxy->GetProperty("XAxisUsePoints"), use_points);
  if (this->getInput()->getProxy()->GetXMLName() == QString("ExtractHistogram"))
    {
    pqSMAdaptor::setEnumerationProperty(
      proxy->GetProperty("ReductionType"), "FIRST_NODE_ONLY");
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(
      proxy->GetProperty("ReductionType"), "RECTILINEAR_GRID_APPEND");
    }
  pqSMAdaptor::setElementProperty(
    proxy->GetProperty("OutputDataType"),"vtkRectilinearGrid");
  proxy->UpdateVTKObjects();

  // Need to update since we would have changed the reduction type.
  vtkSMClientDeliveryRepresentationProxy::SafeDownCast(proxy)->Update();

  // Now initialize the lookup table.
  this->updateLookupTable();
}

//-----------------------------------------------------------------------------
void pqBarChartRepresentation::updateLookupTable()
{
  bool use_points = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("XAxisUsePoints")).toBool();

  vtkDataArray* xarray  = this->getXArray();
  if (!xarray)
    {
    qDebug() << "Cannot set up lookup table, no X array.";
    return;
    }

  pqScalarsToColors* lut;
  // Now set up default lookup table.
  if (use_points || !xarray->GetName())
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
vtkRectilinearGrid* pqBarChartRepresentation::getClientSideData() const
{
  vtkSMClientDeliveryRepresentationProxy* proxy = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkRectilinearGrid::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqBarChartRepresentation::getXArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }

  bool use_points = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("XAxisUsePoints")).toBool();
  if (use_points)
    {
    int component = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("XAxisPointComponent")).toInt();
    switch (component)
      {
    case 0:
      return data->GetXCoordinates();

    case 1:
      return data->GetYCoordinates();

    case 2:
      return data->GetZCoordinates();
      }
    }
  else
    {
    QString xarrayName = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("XArrayName")).toString();
    return data->GetPointData()->GetArray(xarrayName.toAscii().data());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataArray* pqBarChartRepresentation::getYArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }
  QString yarrayName = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("YArrayName")).toString();
  return data->GetCellData()->GetArray(yarrayName.toAscii().data());
}
