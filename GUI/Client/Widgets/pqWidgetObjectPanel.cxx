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

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqWidgetObjectPanel.h"

#include <vtkImplicitPlaneRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

pqWidgetObjectPanel::pqWidgetObjectPanel(QString filename, QWidget* p) :
  pqLoadedFormObjectPanel(filename, p),
  PropertyLinks(new pqPropertyLinks()),
  Widget(0)
{
}

pqWidgetObjectPanel::~pqWidgetObjectPanel()
{
  if(this->Widget)
    {
    pqRenderModule* renModule = 
      pqApplicationCore::instance()->getActiveRenderModule();
    if (renModule)
      {
      vtkSMRenderModuleProxy* rm = renModule->getProxy() ;
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        rm->GetProperty("Displays"));
      pp->RemoveProxy(this->Widget);
      rm->UpdateVTKObjects();
      renModule->render();
      }

    pq3DWidgetFactory* widgetFactory = 
      pqApplicationCore::instance()->get3DWidgetFactory();
    widgetFactory->free3DWidget(this->Widget);
    this->Widget = 0;
    }

  delete this->PropertyLinks;
}

pqPropertyLinks& pqWidgetObjectPanel::getPropertyLinks()
{
  return *this->PropertyLinks;
}

void pqWidgetObjectPanel::setProxy(pqSMProxy proxy)
{
  pqLoadedFormObjectPanel::setProxy(proxy);

  if(!this->Proxy)
    {
    return;
    }

  // Create the 3D widget ...
  if(!this->Widget)
    {
    pq3DWidgetFactory* widgetFactory = 
      pqApplicationCore::instance()->get3DWidgetFactory();
    // We won't have to do this once setProxy() takes
    // pqProxy as an argument.
    pqServer* server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(proxy->GetConnectionID());
    this->Widget = widgetFactory->get3DWidget("ImplicitPlaneWidgetDisplay",
      server);

    // Resize the 3D widget based on the bounds of the input data ...
    if(vtkSMProxyProperty* const input_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("Input")))
      {
      if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
        input_property->GetProxy(0)))
        {
        double input_bounds[6];
        input_proxy->GetDataInformation()->GetBounds(input_bounds);
       
        double input_origin[3];
        input_origin[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
        input_origin[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
        input_origin[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

        double input_size[3];
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 1.2;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 1.2;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 1.2;
        
        if(vtkSMDoubleVectorProperty* const origin = vtkSMDoubleVectorProperty::SafeDownCast(
          this->Widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          }
        
        if(vtkSMDoubleVectorProperty* const place_widget = vtkSMDoubleVectorProperty::SafeDownCast(
          this->Widget->GetProperty("PlaceWidget")))
          {
          double widget_bounds[6];
          widget_bounds[0] = input_origin[0] - input_size[0];
          widget_bounds[1] = input_origin[0] + input_size[0];
          widget_bounds[2] = input_origin[1] - input_size[1];
          widget_bounds[3] = input_origin[1] + input_size[1];
          widget_bounds[4] = input_origin[2] - input_size[2];
          widget_bounds[5] = input_origin[2] + input_size[2];
          
          place_widget->SetElements(widget_bounds);
          }
        }
      }

    this->Widget->UpdateVTKObjects();

    pqRenderModule* renModule = 
      pqApplicationCore::instance()->getActiveRenderModule();

    vtkSMRenderModuleProxy* rm = renModule->getProxy() ;
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      rm->GetProperty("Displays"));
    pp->AddProxy(this->Widget);
    rm->UpdateVTKObjects();
    renModule->render();
    }
}
