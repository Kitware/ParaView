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

#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionManager.h"

#include <QColor>
#include <QPointer>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqDataRepresentationInternal
{
public:
  QPointer<pqOutputPort> InputPort;
  bool VisibilityChangedSinceLastUpdate;
};

//-----------------------------------------------------------------------------
pqDataRepresentation::pqDataRepresentation(
  const QString& group, const QString& name, vtkSMProxy* repr, pqServer* server, QObject* _p)
  : pqRepresentation(group, name, repr, server, _p)
{
  this->Internal = new pqDataRepresentationInternal;
  this->Internal->VisibilityChangedSinceLastUpdate = false;
  vtkEventQtSlotConnect* vtkconnector = this->getConnector();

  vtkconnector->Connect(
    repr->GetProperty("Input"), vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
  vtkconnector->Connect(repr, vtkCommand::UpdateDataEvent, this, SIGNAL(dataUpdated()));

  // fire signals when LUT changes.
  if (vtkSMProperty* prop = repr->GetProperty("LookupTable"))
  {
    vtkconnector->Connect(
      prop, vtkCommand::ModifiedEvent, this, SIGNAL(colorTransferFunctionModified()));
  }
  if (vtkSMProperty* prop = repr->GetProperty("ColorArrayName"))
  {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent, this, SIGNAL(colorArrayNameModified()));
  }
  if (vtkSMProperty* prop = repr->GetProperty("SelectNormalArray"))
  {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent, this, SIGNAL(attrArrayNameModified()));
  }
  if (vtkSMProperty* prop = repr->GetProperty("SelectTCoordArray"))
  {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent, this, SIGNAL(attrArrayNameModified()));
  }
  if (vtkSMProperty* prop = repr->GetProperty("SelectTangentArray"))
  {
    vtkconnector->Connect(prop, vtkCommand::ModifiedEvent, this, SIGNAL(attrArrayNameModified()));
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
  return (this->Internal->InputPort ? this->Internal->InputPort->getSource() : nullptr);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqDataRepresentation::getOutputPortFromInput() const
{
  return this->Internal->InputPort;
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::onInputChanged()
{
  vtkSMInputProperty* ivp =
    vtkSMInputProperty::SafeDownCast(this->getProxy()->GetProperty("Input"));
  if (!ivp)
  {
    qDebug() << "Representation proxy has no input property!";
    return;
  }

  pqOutputPort* oldValue = this->Internal->InputPort;

  int new_proxes_count = ivp->GetNumberOfProxies();
  if (new_proxes_count == 0)
  {
    this->Internal->InputPort = nullptr;
  }
  else if (new_proxes_count == 1)
  {
    pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
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
  return pqSMAdaptor::getProxyProperty(this->getProxy()->GetProperty("LookupTable"));
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqDataRepresentation::getLookupTable()
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxy* lut = this->getLookupTableProxy();

  return (lut ? smmodel->findItem<pqScalarsToColors*>(lut) : 0);
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
  bool vtkNotUsed(update) /*=true*/) const
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(this->getProxy());
  if (repr)
  {
    return repr->GetRepresentedDataInformation();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqDataRepresentation::getInputDataInformation() const
{
  if (!this->getOutputPortFromInput())
  {
    return nullptr;
  }

  return this->getOutputPortFromInput()->getDataInformation();
}

//-----------------------------------------------------------------------------
vtkPVTemporalDataInformation* pqDataRepresentation::getInputTemporalDataInformation() const
{
  if (!this->getOutputPortFromInput())
  {
    return nullptr;
  }

  return this->getOutputPortFromInput()->getTemporalDataInformation();
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqDataRepresentation::getRepresentationForUpstreamSource() const
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(this->getInput());
  pqView* view = this->getView();
  if (!filter || filter->getInputCount() == 0 || view == nullptr)
  {
    return nullptr;
  }

  // find a repre for the input of the filter
  pqOutputPort* input = filter->getInputs()[0];
  if (!input)
  {
    return nullptr;
  }

  return input->getRepresentation(view);
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::onVisibilityChanged()
{
  this->Superclass::onVisibilityChanged();

  this->Internal->VisibilityChangedSinceLastUpdate = true;
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::updateLookupTable()
{
  // Only update the LookupTable when the setting tells us to
  vtkSMProxy* representationProxy = this->getProxy();
  vtkSMProxy* lut = vtkSMPropertyHelper(representationProxy, "LookupTable").GetAsProxy();
  if (!lut)
  {
    return;
  }

  int rescaleOnVisibilityChange =
    vtkSMPropertyHelper(lut, "RescaleOnVisibilityChange", 1).GetAsInt(0);
  if (rescaleOnVisibilityChange && this->Internal->VisibilityChangedSinceLastUpdate)
  {
    this->resetAllTransferFunctionRangesUsingCurrentData();
  }
  this->Internal->VisibilityChangedSinceLastUpdate = false;
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::resetAllTransferFunctionRangesUsingCurrentData()
{
  vtkSMProxy* representationProxy = this->getProxy();
  vtkSMProxy* lut = vtkSMPropertyHelper(representationProxy, "LookupTable").GetAsProxy();
  if (lut)
  {
    vtkNew<vtkSMTransferFunctionManager> tfmgr;
    tfmgr->ResetAllTransferFunctionRangesUsingCurrentData(this->getServer()->proxyManager(), false);
  }
}
