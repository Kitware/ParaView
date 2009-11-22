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
#include "vtkPVDataInformation.h"
#include "vtkSMDataRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

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
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqOutputPort> InputPort;

  pqDataRepresentationInternal()
    {
    this->VTKConnect = vtkEventQtSlotConnect::New();;
    }
  ~pqDataRepresentationInternal()
    {
    this->VTKConnect->Delete();
    }
};


//-----------------------------------------------------------------------------
pqDataRepresentation::pqDataRepresentation(const QString& group,
  const QString& name, vtkSMProxy* repr, pqServer* server,
  QObject *_p)
: pqRepresentation(group, name, repr, server, _p)
{
  this->Internal = new pqDataRepresentationInternal;
  this->Internal->VTKConnect->Connect(repr->GetProperty("Input"),
    vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
  this->Internal->VTKConnect->Connect(repr, vtkCommand::UpdateDataEvent,
    this, SIGNAL(dataUpdated()));
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
void pqDataRepresentation::setDefaultPropertyValues()
{
  if (!this->isVisible())
    {
    // For any non-visible representation, we don't set its defaults.
    return;
    }

  // Set default arrays and lookup table.
  vtkSMRepresentationProxy* proxy = vtkSMRepresentationProxy::SafeDownCast(
    this->getProxy());
  
  // setDefaultPropertyValues() can always call Update on the display. 
  // This is safe since setDefaultPropertyValues is called only after having
  // added the display to the render module, which ensures that the
  // update time has been set correctly on the display.
  proxy->Update();
  proxy->GetProperty("Input")->UpdateDependentDomains();
  
  this->Superclass::setDefaultPropertyValues();
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
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    this->getProxy());
  if (repr)
    {
    return repr->GetFullResMemorySize();
    }

  return 0;
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
  bool update/*=true*/) const
{
  vtkSMDataRepresentationProxy* repr = vtkSMDataRepresentationProxy::SafeDownCast(
    this->getProxy());
  if (repr)
    {
    return repr->GetRepresentedDataInformation(update);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqDataRepresentation::getInputDataInformation() const
{
  if (!this->getInput())
    {
    return 0;
    }

  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(
    this->getInput()->getProxy());
  if (source)
    {
    return source->GetDataInformation(
      this->getOutputPortFromInput()->getPortNumber());
    }
  return 0;
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
