/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineDisplay.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineDisplay.cxx
/// \date 4/24/2006

#include "pqPipelineDisplay.h"


// ParaView includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h" 
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMInputProperty.h"

// Qt includes.
#include <QPointer>
#include <QtDebug>

// ParaQ includes.
#include "pqParts.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqPipelineDisplayInternal
{
public:
  vtkSmartPointer<vtkSMDataObjectDisplayProxy> DisplayProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqPipelineSource> Input;
};

//-----------------------------------------------------------------------------
pqPipelineDisplay::pqPipelineDisplay(const QString& name,
  vtkSMDataObjectDisplayProxy* display,
  pqServer* server, QObject* p/*=null*/):
  pqProxy("displays", name, display, server, p)
{
  this->Internal = new pqPipelineDisplayInternal();
  this->Internal->DisplayProxy = display;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->Input = 0;
  if (display)
    {
    this->Internal->VTKConnect->Connect(display->GetProperty("Input"),
      vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
    }
  // This will make sure that if the input is already set.
  this->onInputChanged();
}

//-----------------------------------------------------------------------------
pqPipelineDisplay::~pqPipelineDisplay()
{
  if (this->Internal->DisplayProxy)
    {
    this->Internal->VTKConnect->Disconnect(
      this->Internal->DisplayProxy->GetProperty("Input"));
    }
  if (this->Internal->Input)
    {
    this->Internal->Input->removeDisplay(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy* pqPipelineDisplay::getDisplayProxy() const
{
  return this->Internal->DisplayProxy;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineDisplay::getInput() const
{
  return this->Internal->Input;
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::onInputChanged()
{
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    this->Internal->DisplayProxy->GetProperty("Input"));
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
    pqServerManagerModel* model = pqServerManagerModel::instance();
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
void pqPipelineDisplay::setDefaultColorParametes()
{
  // eventually the implementation from pqPart must move here.
  pqPart::Color(this->getDisplayProxy());
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::colorByArray(const char* arrayname, int fieldtype)
{
  pqPart::Color(this->getDisplayProxy(), arrayname, fieldtype);
}

//-----------------------------------------------------------------------------
/*
// This save state must go away. GUI just needs to save which window
// contains rendermodule with what ID.  ProxyManager will manage the display.
void pqPipelineDisplay::SaveState(vtkPVXMLElement *root,
    pqMultiView *multiView)
{
  if(!root || !multiView || !this->Internal)
    {
    return;
    }

  vtkPVXMLElement *element = 0;
  QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if((*iter)->Window && !(*iter)->DisplayName.isEmpty())
      {
      element = vtkPVXMLElement::New();
      element->SetName("Display");
      element->AddAttribute("name", (*iter)->DisplayName.toAscii().data());
      element->AddAttribute("windowID", pqXMLUtil::GetStringFromIntList(
          multiView->indexOf((*iter)->Window->parentWidget())).toAscii().data());
      root->AddNestedElement(element);
      element->Delete();
      }
    }
}*/


