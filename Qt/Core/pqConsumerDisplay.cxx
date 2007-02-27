/*=========================================================================

   Program: ParaView
   Module:    pqConsumerDisplay.cxx

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
#include "pqConsumerDisplay.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPropertyIterator.h"

#include <QtDebug>
#include <QPointer>
#include <QColor>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqConsumerDisplayInternal
{
public:
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqPipelineSource> Input;

  pqConsumerDisplayInternal()
    {
    this->VTKConnect = vtkEventQtSlotConnect::New();;
    }
  ~pqConsumerDisplayInternal()
    {
    this->VTKConnect->Delete();
    }
};


//-----------------------------------------------------------------------------
pqConsumerDisplay::pqConsumerDisplay(const QString& group,
  const QString& name, vtkSMProxy* display, pqServer* server,
  QObject *_p)
: pqDisplay(group, name, display, server, _p)
{
  this->Internal = new pqConsumerDisplayInternal;
  this->Internal->VTKConnect->Connect(display->GetProperty("Input"),
    vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
  this->Internal->VTKConnect->Connect(display->GetProperty("Visibility"),
    vtkCommand::ModifiedEvent, this, SLOT(onVisibilityChanged()));

  // This will make sure that if the input is already set.
  this->onInputChanged();
}

//-----------------------------------------------------------------------------
pqConsumerDisplay::~pqConsumerDisplay()
{
  if (this->Internal->Input)
    {
    this->Internal->Input->removeDisplay(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqConsumerDisplay::getInput() const
{
  return this->Internal->Input;
}

//-----------------------------------------------------------------------------
void pqConsumerDisplay::onInputChanged()
{
  vtkSMProxyProperty* ivp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Input"));
  if (!ivp)
    {
    qDebug() << "Display proxy has no input property!";
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
    pqServerManagerModel* model = 
      pqApplicationCore::instance()->getServerManagerModel();
    removed = this->Internal->Input;
    this->Internal->Input = model->getPQSource(ivp->GetProxy(0));
    added = this->Internal->Input;
    if (ivp->GetProxy(0) && !this->Internal->Input)
      {
      qDebug() << "Display could not locate the pqPipelineSource object "
        << "for the input proxy.";
      }
    }
  else if (new_proxes_count > 1)
    {
    qDebug() << "Displays with more than 1 input are not handled.";
    return;
    }

  // Now tell the pqPipelineSource about the changes in the displays.
  if (removed)
    {
    removed->removeDisplay(this);
    }
  if (added)
    {
    added->addDisplay(this);
    }
}

//-----------------------------------------------------------------------------
void pqConsumerDisplay::setDefaults()
{
  if (!this->isVisible())
    {
    // For any non-visible display, we don't set its defaults.
    return;
    }

  // Set default arrays and lookup table.
  vtkSMAbstractDisplayProxy* proxy = vtkSMAbstractDisplayProxy::SafeDownCast(
    this->getProxy());
  
  // setDefaults() can always call Update on the display. 
  // This is safe since setDefaults() will typically be called only after having
  // added the display to the render module, which ensures that the
  // update time has been set correctly on the display.
  proxy->Update();

  proxy->GetProperty("Input")->UpdateDependentDomains();
  proxy->UpdatePropertyInformation();


  // This will setup default array names. Just reset-to-default all properties,
  // the vtkSMArrayListDomain will do the rest.
  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if (!iter->GetProperty()->GetInformationOnly())
      {
      iter->GetProperty()->ResetToDefault();
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
