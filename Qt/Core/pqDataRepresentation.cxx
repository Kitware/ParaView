/*=========================================================================

   Program: ParaView
   Module:    pqDataRepresentation.cxx

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
#include "pqDataRepresentation.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkDataObject.h"

#include <QtDebug>
#include <QPointer>
#include <QColor>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqDataRepresentationInternal
{
public:
  QPointer<pqOutputPort> InputPort;
};

//-----------------------------------------------------------------------------
pqDataRepresentation::pqDataRepresentation(const QString& group,
  const QString& name, vtkSMProxy* repr, pqServer* server,
  QObject *_p)
: pqRepresentation(group, name, repr, server, _p)
{
  this->Internal = new pqDataRepresentationInternal;
  vtkEventQtSlotConnect* vtkconnector = this->getConnector();

  vtkconnector->Connect(repr->GetProperty("Input"),
    vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
  vtkconnector->Connect(repr, vtkCommand::UpdateDataEvent,
    this, SIGNAL(dataUpdated()));

  // fire signals when LUT changes.
  if (vtkSMProperty* prop = repr->GetProperty("LookupTable"))
    {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent,
      this, SIGNAL(colorTransferFunctionModified()));
    }
  if (vtkSMProperty* prop = repr->GetProperty("ColorArrayName"))
    {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent,
      this, SIGNAL(colorArrayNameModified()));
    }
}

//-----------------------------------------------------------------------------
pqDataRepresentation::~pqDataRepresentation()
{
  if (this->Internal->InputPort)
    {
    this->Internal->InputPort->removeRepresentation(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqDataRepresentation::getInput() const
{
  return (this->Internal->InputPort?
    this->Internal->InputPort->getSource() : 0);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqDataRepresentation::getOutputPortFromInput() const
{
  return this->Internal->InputPort;
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::onInputChanged()
{
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    this->getProxy()->GetProperty("Input"));
  if (!ivp)
    {
    qDebug() << "Representation proxy has no input property!";
    return;
    }

  pqOutputPort* oldValue = this->Internal->InputPort;

  int new_proxes_count = ivp->GetNumberOfProxies();
  if (new_proxes_count == 0)
    {
    this->Internal->InputPort = 0;
    }
  else if (new_proxes_count == 1)
    {
    pqServerManagerModel* smModel =
      pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineSource* input = smModel->findItem<pqPipelineSource*>(ivp->GetProxy(0));
    if (ivp->GetProxy(0) && !input)
      {
      qDebug() << "Representation could not locate the pqPipelineSource object "
        << "for the input proxy.";
      }
    else
      {
      int portnumber = ivp->GetOutputPortForConnection(0);
      this->Internal->InputPort = input->getOutputPort(portnumber);
      }
    }
  else if (new_proxes_count > 1)
    {
    qDebug() << "Representations with more than 1 inputs are not handled.";
    return;
    }

  if (oldValue != this->Internal->InputPort)
    {
    // Now tell the pqPipelineSource about the changes in the representations.
    if (oldValue)
      {
      oldValue->removeRepresentation(this);
      }
    if (this->Internal->InputPort)
      {
      this->Internal->InputPort->addRepresentation(this);
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqDataRepresentation::getLookupTableProxy()
{
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("LookupTable"));
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqDataRepresentation::getLookupTable()
{
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxy* lut = this->getLookupTableProxy();

  return (lut? smmodel->findItem<pqScalarsToColors*>(lut): 0);
}

//-----------------------------------------------------------------------------
unsigned long pqDataRepresentation::getFullResMemorySize()
{
  vtkPVDataInformation* info = this->getRepresentedDataInformation(true);
  if (!info)
    {
    return 0;
    }
  return static_cast<unsigned long>(info->GetMemorySize());
}

//-----------------------------------------------------------------------------
bool pqDataRepresentation::getDataBounds(double bounds[6])
{
  vtkPVDataInformation* info = this->getRepresentedDataInformation(true);
  if (!info)
    {
    return false;
    }
  info->GetBounds(bounds);
  return true;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqDataRepresentation::getRepresentedDataInformation(
  bool vtkNotUsed(update)/*=true*/) const
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    this->getProxy());
  if (repr)
    {
    return repr->GetRepresentedDataInformation();
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqDataRepresentation::getInputDataInformation() const
{
  if (!this->getOutputPortFromInput())
    {
    return 0;
    }

  return this->getOutputPortFromInput()->getDataInformation();
}

//-----------------------------------------------------------------------------
vtkPVTemporalDataInformation* pqDataRepresentation::getInputTemporalDataInformation() const
{
  if (!this->getOutputPortFromInput())
    {
    return 0;
    }

  return this->getOutputPortFromInput()->getTemporalDataInformation();
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqDataRepresentation::getRepresentationForUpstreamSource() const
{
 pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(this->getInput());
 pqView* view = this->getView();
 if (!filter || filter->getInputCount() == 0 || view == 0)
   {
   return 0;
   }

 // find a repre for the input of the filter
 pqOutputPort* input = filter->getInputs()[0];
 if (!input)
   {
   return 0;
   }

 return input->getRepresentation(view);
}

//-----------------------------------------------------------------------------
QString pqDataRepresentation::getProxyColorArrayName()
{
  QVariant colorArrayName = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty( "ColorArrayName" ) );

  return colorArrayName.toString();
}

//-----------------------------------------------------------------------------
int pqDataRepresentation::getProxyScalarMode()
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast( this->getProxy() );
  if (!repr)
    {
    return 0;
    }

  // we check on if there is a color array name first since
  // color attribute type may still be POINT_DATA or CELL_DATA even though
  // the object isn't colored by field data
  QVariant colorArrayName = pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName"));

  if(colorArrayName.isValid() == false || colorArrayName.isNull() == true ||
     colorArrayName == "")
    {
    return vtkDataObject::FIELD_ASSOCIATION_NONE;
    }

  QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
    repr->GetProperty("ColorAttributeType"));

  if(scalarMode == "CELL_DATA")
    {
    return vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(scalarMode == "POINT_DATA")
    {
    return vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  return vtkDataObject::FIELD_ASSOCIATION_NONE;
}


//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqDataRepresentation::getProxyColorArrayInfo()
{
  const char* colorArray = 0;
  vtkSMStringVectorProperty* caProp =
    vtkSMStringVectorProperty::SafeDownCast(
      this->getProxy()->GetProperty( "ColorArrayName" ));
  if ( caProp )
    {
    colorArray = caProp->GetElement( 0 );
    }
  int fieldType = this->getProxyScalarMode();
  vtkPVArrayInformation* retval = colorArray ?
    this->getArrayInformation( colorArray, fieldType ) : 0;
  return retval;
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqDataRepresentation::getArrayInformation(
  const char* arrayname, const int &fieldType )
{

  vtkPVDataInformation* dataInfo = this->getRepresentedDataInformation(true);
  vtkPVArrayInformation* info = NULL;
  if(fieldType == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    vtkPVDataSetAttributesInformation* cellinfo =
      dataInfo->GetCellDataInformation();
    info = cellinfo->GetArrayInformation(arrayname);
    }
  else if ( fieldType == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    vtkPVDataSetAttributesInformation* pointinfo =
      dataInfo->GetPointDataInformation();
    info = pointinfo->GetArrayInformation(arrayname);
    }
  return info;
}

//-----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
pqDataRepresentation::getProxyColorProminentValuesInfo(
  double uncertainty, double fraction)
{
  const char* colorArray = 0;
  vtkSMStringVectorProperty* caProp =
    vtkSMStringVectorProperty::SafeDownCast(
      this->getProxy()->GetProperty( "ColorArrayName" ));
  if ( caProp )
    {
    colorArray = caProp->GetElement( 0 );
    }
  int fieldType = this->getProxyScalarMode();
  vtkPVProminentValuesInformation* retval = colorArray ?
    this->getProminentValuesInformation(
      colorArray, fieldType, uncertainty, fraction ) :
    0;
  return retval;
}

//-----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
pqDataRepresentation::getProminentValuesInformation(
  const char* arrayName, const int &fieldType,
  double uncertainty, double minFraction )
{
  vtkPVArrayInformation* ainfo =
    this->getArrayInformation(arrayName, fieldType);
  if (!ainfo)
    {
    return 0;
    }
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    this->getProxy());
  // repr == 0 can't happen since (ainfo != 0) => (repr != 0)
  vtkPVProminentValuesInformation* pvinfo =
    repr->GetProminentValuesInformation(
      arrayName, fieldType, ainfo->GetNumberOfComponents(),
      uncertainty, minFraction );
  return pvinfo;
}

//-----------------------------------------------------------------------------
int pqDataRepresentation::getNumberOfComponents(const char* arrayname, int fieldType)
{
  vtkPVArrayInformation *info = this->getArrayInformation( arrayname, fieldType );
  return ( info ) ? info->GetNumberOfComponents() : 0;
}

//-----------------------------------------------------------------------------
QString pqDataRepresentation::getComponentName( const char* arrayname, int fieldType, int component)
{
  vtkPVArrayInformation *info = this->getArrayInformation( arrayname, fieldType );
  if ( info )
     {
     return QString(info->GetComponentName( component ));
     }
  return QString();
}
