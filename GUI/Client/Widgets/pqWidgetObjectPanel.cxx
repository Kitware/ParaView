/*=========================================================================

   Program:   ParaQ
   Module:    pqWidgetObjectPanel.cxx

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

#include "pqApplicationCore.h"
#include "pqRenderModule.h"
#include "pqWidgetObjectPanel.h"

#include <vtkImplicitPlaneRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>

pqWidgetObjectPanel::pqWidgetObjectPanel(QString filename, QWidget* p) :
  pqLoadedFormObjectPanel(filename, p),
  Widget(0)
{
  this->Widget = vtkSMNew3DWidgetProxy::SafeDownCast(
    vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "ImplicitPlaneWidgetDisplay"));

  this->Widget->UpdateVTKObjects();

//  this->Widget->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);

  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();

  vtkSMRenderModuleProxy* rm = renModule->getProxy() ;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  pp->AddProxy(this->Widget);
  rm->UpdateVTKObjects();

}

pqWidgetObjectPanel::~pqWidgetObjectPanel()
{
  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();

  vtkSMRenderModuleProxy* rm = renModule->getProxy() ;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  pp->RemoveProxy(this->Widget);
  rm->UpdateVTKObjects();

  this->Widget->Delete();
  this->Widget = 0;
}

void pqWidgetObjectPanel::setProxy(pqSMProxy proxy)
{
  pqLoadedFormObjectPanel::setProxy(proxy);

  if(!this->Proxy)
    return;

  // Set widget bounds here
}
