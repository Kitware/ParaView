/*=========================================================================

   Program: ParaView
   Module:    pqDataRepresentation.cxx

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
#include "pqDataRepresentation.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"

#include <QtDebug>
#include <QPointer>
#include <QColor>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqDataRepresentationInternal
{
public:
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqPipelineSource> Input;

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

  // This will make sure that if the input is already set.
  this->onInputChanged();
}

//-----------------------------------------------------------------------------
pqDataRepresentation::~pqDataRepresentation()
{
  if (this->Internal->Input)
    {
    this->Internal->Input->removeRepresentation(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqDataRepresentation::getInput() const
{
  return this->Internal->Input;
}

//-----------------------------------------------------------------------------
void pqDataRepresentation::onInputChanged()
{
  vtkSMProxyProperty* ivp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Input"));
  if (!ivp)
    {
    qDebug() << "Representation proxy has no input property!";
    return;
    }

  pqPipelineSource* added = 0;
  pqPipelineSource* removed = 0;

  int new_proxes_count = ivp->GetNumberOfProxies();
  if (new_proxes_count == 0)
    {
    removed = this->Internal->Input;
    this->Internal->Input = 0;
    }
  else if (new_proxes_count == 1)
    {
    pqServerManagerModel* smModel = 
      pqApplicationCore::instance()->getServerManagerModel();
    removed = this->Internal->Input;
    this->Internal->Input = smModel->findItem<pqPipelineSource*>(ivp->GetProxy(0));
    added = this->Internal->Input;
    if (ivp->GetProxy(0) && !this->Internal->Input)
      {
      qDebug() << "Representation could not locate the pqPipelineSource object "
        << "for the input proxy.";
      }
    }
  else if (new_proxes_count > 1)
    {
    qDebug() << "Representation with more than 1 input are not handled.";
    return;
    }

  // Now tell the pqPipelineSource about the changes in the representations.
  if (removed)
    {
    removed->removeRepresentation(this);
    }
  if (added)
    {
    added->addRepresentation(this);
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

