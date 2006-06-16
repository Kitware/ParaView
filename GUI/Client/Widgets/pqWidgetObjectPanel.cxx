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
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqWidgetObjectPanel.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

#include <QPushButton>

/////////////////////////////////////////////////////////////////////////
// pqWidgetObjectPanel::WidgetObserver

class pqWidgetObjectPanel::WidgetObserver :
  public vtkCommand
{
public:
  static WidgetObserver* New()
  {
    return new WidgetObserver();
  }

  void SetPanel(pqWidgetObjectPanel& panel)
  {
    this->Panel = &panel;
  }

  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    if(this->Panel)
      {
      this->Panel->on3DWidgetChanged();
      }
  }

private:
  WidgetObserver() :
    Panel(0)
  {
  }
  
  ~WidgetObserver()
  {
  }
  
  pqWidgetObjectPanel* Panel;
};

/////////////////////////////////////////////////////////////////////////
// pqWidgetObjectPanel

pqWidgetObjectPanel::pqWidgetObjectPanel(QString filename, QWidget* p) :
  pqLoadedFormObjectPanel(filename, p),
  Widget(0),
  Observer(WidgetObserver::New())
{
  this->Observer->SetPanel(*this);
  
  connect(this->findChild<QPushButton*>("useXNormal"), SIGNAL(clicked()), this, SLOT(onUseXNormal()));
  connect(this->findChild<QPushButton*>("useYNormal"), SIGNAL(clicked()), this, SLOT(onUseYNormal()));
  connect(this->findChild<QPushButton*>("useZNormal"), SIGNAL(clicked()), this, SLOT(onUseZNormal()));
  connect(this->findChild<QPushButton*>("useCameraNormal"), SIGNAL(clicked()), this, SLOT(onUseCameraNormal()));
  connect(this->findChild<QPushButton*>("resetBounds"), SIGNAL(clicked()), this, SLOT(onResetBounds()));
  connect(this->findChild<QPushButton*>("useCenterBounds"), SIGNAL(clicked()), this, SLOT(onUseCenterBounds()));
}

pqWidgetObjectPanel::~pqWidgetObjectPanel()
{
  this->Observer->Delete();

  if(this->Widget)
    {
    this->deselect();

    pq3DWidgetFactory* widgetFactory = 
      pqApplicationCore::instance()->get3DWidgetFactory();
    widgetFactory->free3DWidget(this->Widget);
    this->Widget = 0;
    }
}

//-----------------------------------------------------------------------------
void pqWidgetObjectPanel::select()
{
  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();
  if (this->Widget && renModule)
    {
    vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy() ;
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      rm->GetProperty("Displays"));
    pp->AddProxy(this->Widget);
    rm->UpdateVTKObjects();
    renModule->render();
    }
    
  if(this->Widget)
    this->Widget->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);

  this->pqNamedObjectPanel::select();
}

//-----------------------------------------------------------------------------
void pqWidgetObjectPanel::deselect()
{
  if(this->Widget)
    this->Widget->RemoveObserver(this->Observer);

  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();
  if (this->Widget && renModule)
    {
    vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy() ;
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      rm->GetProperty("Displays"));
    pp->RemoveProxy(this->Widget);
    rm->UpdateVTKObjects();
    renModule->render();
    }
  this->pqNamedObjectPanel::deselect();
}

//-----------------------------------------------------------------------------
void pqWidgetObjectPanel::setProxyInternal(pqSMProxy p)
{
  pqLoadedFormObjectPanel::setProxyInternal(p);

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
      getServerManagerModel()->getServer(this->Proxy->GetConnectionID());
    this->Widget = widgetFactory->get3DWidget("ImplicitPlaneWidgetDisplay",
      server);
    }
    
  // Synchronize the 3D widget bounds with the source data ...
  this->onResetBounds();
}

void pqWidgetObjectPanel::onResetBounds()
{
  if(this->Widget)
    {
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
          
          this->Widget->UpdateVTKObjects();
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqWidgetObjectPanel::onUseCenterBounds()
{
  if(this->Widget)
    {
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

        if(vtkSMDoubleVectorProperty* const origin = vtkSMDoubleVectorProperty::SafeDownCast(
          this->Widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          this->Widget->UpdateVTKObjects();
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqWidgetObjectPanel::onUseXNormal()
{
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(1, 0, 0);
      this->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqWidgetObjectPanel::onUseYNormal()
{
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 1, 0);
      this->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqWidgetObjectPanel::onUseZNormal()
{
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 0, 1);
      this->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqWidgetObjectPanel::onUseCameraNormal()
{
  if(this->Widget)
    {
    if(vtkCamera* const camera =
      pqApplicationCore::instance()->getActiveRenderModule()->
      getRenderModuleProxy()->GetRenderer()->GetActiveCamera())
      {
      if(vtkSMDoubleVectorProperty* const normal = vtkSMDoubleVectorProperty::SafeDownCast(
        this->Widget->GetProperty("Normal")))
        {
        double camera_normal[3];
        camera->GetViewPlaneNormal(camera_normal);
        normal->SetElements3(-camera_normal[0], -camera_normal[1], -camera_normal[2]);
        
        this->Widget->UpdateVTKObjects();
        pqApplicationCore::instance()->render();
        }
      }
    }
}

void pqWidgetObjectPanel::on3DWidgetChanged()
{
}
