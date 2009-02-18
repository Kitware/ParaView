/*=========================================================================

   Program:   ParaQ
   Module:    pqParticleTracerPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
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
#include "pqParticleTracerPanel.h"

#include "pqApplicationCore.h"
#include "pqLineSourceWidget.h"
#include "pqNamedWidgets.h"
#include "pqPipelineFilter.h"
#include "pqPointSourceWidget.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include "ui_pqParticleTracerPanel.h"

#include <pqCollapsedGroup.h>

#include <vtkMemberFunctionCommand.h>
#include <vtkPVDataInformation.h>
#include <vtkPVXMLElement.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMSourceProxy.h>

#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqParticleTracerPanel::pqImplementation

class pqParticleTracerPanel::pqImplementation
{
public:
  pqImplementation() :
    PointSourceWidget(NULL),
    LineSourceWidget(NULL)
  {
  }

  ~pqImplementation()
  {
    delete this->LineSourceWidget;
    delete this->PointSourceWidget;
  }
  
  /// Provides a container for Qt controls
  QWidget UIContainer;

  /// Provides a UI for managing a vtkPointSource
  pqPointSourceWidget* PointSourceWidget;
  /// Manages a 3D line widget, plus Qt controls  
  pqLineSourceWidget* LineSourceWidget;
  /// Provides the remaining Qt controls for the panel
  Ui::pqParticleTracerPanel UI;
};

//-----------------------------------------------------------------------------
pqParticleTracerPanel::pqParticleTracerPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p),
  Implementation(new pqImplementation())
{
  // Get the boundaries of the new proxy ...
  double proxy_center[3];
  double proxy_size[3];
  
  if(vtkSMInputProperty* const input_property =
    vtkSMInputProperty::SafeDownCast(
      this->proxy()->GetProperty("Input")))
    {
    if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
      input_property->GetProxy(0)))
      {
      double input_bounds[6];
      input_proxy->GetDataInformation(
        input_property->GetOutputPortForConnection(0))->GetBounds(input_bounds);

      proxy_center[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
      proxy_center[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
      proxy_center[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

      proxy_size[0] = fabs(input_bounds[1] - input_bounds[0]);
      proxy_size[1] = fabs(input_bounds[3] - input_bounds[2]);
      proxy_size[2] = fabs(input_bounds[5] - input_bounds[4]);
      }
    }
  
  vtkSMProxyProperty* source_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->GetProperty("Source"));
  
  QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
    
  for(int i = 0; i != sources.size(); ++i)
    {
    pqSMProxy source = sources[i];
    if(source->GetVTKClassName() == QString("vtkPointSource"))
      {
      if(vtkSMDoubleVectorProperty* const center =
        vtkSMDoubleVectorProperty::SafeDownCast(
          source->GetProperty("Center")))
        {
        center->SetNumberOfElements(3);
        center->SetElement(0, proxy_center[0]);
        center->SetElement(1, proxy_center[1]);
        center->SetElement(2, proxy_center[2]);
        }
          
      if(vtkSMIntVectorProperty* const number_of_points =
        vtkSMIntVectorProperty::SafeDownCast(
          source->GetProperty("NumberOfPoints")))
        {
        number_of_points->SetNumberOfElements(1);
        number_of_points->SetElement(0, 100);
        }
      source->UpdateVTKObjects();

      this->Implementation->PointSourceWidget = 
        new pqPointSourceWidget(this->proxy(), source, NULL);

      if(vtkPVXMLElement* const hints = source->GetHints())
        {
        for(unsigned int cc=0; cc <hints->GetNumberOfNestedElements(); cc++)
          {
          if(vtkPVXMLElement* const elem = hints->GetNestedElement(cc))
            {
            if (QString("PropertyGroup") == elem->GetName() && 
              QString("PointSource") == elem->GetAttribute("type"))
              {
              this->Implementation->PointSourceWidget->setHints(elem);
              break;
              }
            }
          }
        }
      }
    else if(source->GetVTKClassName() == QString("vtkLineSource"))
      {
      if(vtkSMDoubleVectorProperty* const point1 =
        vtkSMDoubleVectorProperty::SafeDownCast(
          source->GetProperty("Point1")))
        {
        point1->SetNumberOfElements(3);
        point1->SetElement(0, proxy_center[0] - proxy_size[0]);
        point1->SetElement(1, proxy_center[1]);
        point1->SetElement(2, proxy_center[2]);
        }
          
      if(vtkSMDoubleVectorProperty* const point2 =
        vtkSMDoubleVectorProperty::SafeDownCast(
          source->GetProperty("Point2")))
        {
        point2->SetNumberOfElements(3);
        point2->SetElement(0, proxy_center[0] + proxy_size[0]);
        point2->SetElement(1, proxy_center[1]);
        point2->SetElement(2, proxy_center[2]);
        }
          
      if(vtkSMIntVectorProperty* const resolution =
        vtkSMIntVectorProperty::SafeDownCast(
          source->GetProperty("Resolution")))
        {
        resolution->SetNumberOfElements(1);
        resolution->SetElement(0, 100);
        }
      source->UpdateVTKObjects();

      this->Implementation->LineSourceWidget = 
        new pqLineSourceWidget(this->proxy(), source, NULL);
      
      vtkSMProperty* const point1 = source->GetProperty("Point1");
      vtkSMProperty* const point2 = source->GetProperty("Point2");
      vtkSMProperty* const resolution = source->GetProperty("Resolution");
          
      if(point1 && point2 && resolution)
        this->Implementation->LineSourceWidget->setControlledProperties(point1, point2, resolution);
      }
    }

  this->Implementation->UI.setupUi(
    &this->Implementation->UIContainer);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->setMargin(0);
  panel_layout->setSpacing(0);
  panel_layout->addWidget(&this->Implementation->UIContainer);
  panel_layout->addStretch();
  this->setLayout(panel_layout);

  QVBoxLayout* const point_layout = new QVBoxLayout(this->Implementation->UI.pointSource);
  point_layout->addWidget(this->Implementation->PointSourceWidget);
  point_layout->addStretch();
  this->Implementation->UI.pointSource->setLayout(point_layout);
  
  QVBoxLayout* const line_layout = new QVBoxLayout(this->Implementation->UI.lineSource);
  line_layout->addWidget(this->Implementation->LineSourceWidget);
  line_layout->addStretch();
  this->Implementation->UI.lineSource->setLayout(line_layout);
  
  QObject::connect(this->Implementation->UI.seedType, SIGNAL(currentIndexChanged(int)), this, SLOT(onSeedTypeChanged(int)));
  
  QObject::connect(
    this,
    SIGNAL(viewChanged(pqRenderView*)),
    this,
    SLOT(onViewChanged(pqRenderView*)));

  QObject::connect(
    this->Implementation->PointSourceWidget, SIGNAL(modified()),
    this, SLOT(setModified()));

  QObject::connect(
    this->Implementation->LineSourceWidget,
    SIGNAL(modified()),
    this, SLOT(setModified()));
  
  QObject::connect(
    this, SIGNAL(onaccept()), this->Implementation->PointSourceWidget, SLOT(accept()));
  QObject::connect(
    this, SIGNAL(onaccept()), this->Implementation->LineSourceWidget, SLOT(accept()));
  QObject::connect(
    this, SIGNAL(onreset()), this->Implementation->PointSourceWidget, SLOT(reset()));
  QObject::connect(
    this, SIGNAL(onreset()), this->Implementation->LineSourceWidget, SLOT(reset()));
  QObject::connect(
    this, SIGNAL(onselect()), this->Implementation->PointSourceWidget, SLOT(select()));
  QObject::connect(
    this, SIGNAL(onselect()), this->Implementation->LineSourceWidget, SLOT(select()));
  QObject::connect(
    this, SIGNAL(ondeselect()), this->Implementation->PointSourceWidget, SLOT(deselect()));
  QObject::connect(
    this, SIGNAL(ondeselect()), this->Implementation->LineSourceWidget, SLOT(deselect()));

#if 0

  if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("Source")))
    {
    // Setup initial defaults for our seed sources ...  
    const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
    for(int i = 0; i != sources.size(); ++i)
      {
      pqSMProxy source = sources[i];
      
      if(source->GetVTKClassName() == QString("vtkPointSource"))
        {
        if(vtkSMDoubleVectorProperty* const center =
          vtkSMDoubleVectorProperty::SafeDownCast(
            source->GetProperty("Center")))
          {
          center->SetNumberOfElements(3);
          center->SetElement(0, proxy_center[0]);
          center->SetElement(1, proxy_center[1]);
          center->SetElement(2, proxy_center[2]);
          }
            
        if(vtkSMIntVectorProperty* const number_of_points =
          vtkSMIntVectorProperty::SafeDownCast(
            source->GetProperty("NumberOfPoints")))
          {
          number_of_points->SetNumberOfElements(1);
          number_of_points->SetElement(0, 100);
          }
        source->UpdateVTKObjects();
        }
      else if(source->GetVTKClassName() == QString("vtkLineSource"))
        {
        if(vtkSMDoubleVectorProperty* const point1 =
          vtkSMDoubleVectorProperty::SafeDownCast(
            source->GetProperty("Point1")))
          {
          point1->SetNumberOfElements(3);
          point1->SetElement(0, proxy_center[0] - proxy_size[0]);
          point1->SetElement(1, proxy_center[1]);
          point1->SetElement(2, proxy_center[2]);
          }
            
        if(vtkSMDoubleVectorProperty* const point2 =
          vtkSMDoubleVectorProperty::SafeDownCast(
            source->GetProperty("Point2")))
          {
          point2->SetNumberOfElements(3);
          point2->SetElement(0, proxy_center[0] + proxy_size[0]);
          point2->SetElement(1, proxy_center[1]);
          point2->SetElement(2, proxy_center[2]);
          }
            
        if(vtkSMIntVectorProperty* const resolution =
          vtkSMIntVectorProperty::SafeDownCast(
            source->GetProperty("Resolution")))
          {
          resolution->SetNumberOfElements(1);
          resolution->SetElement(0, 100);
          }
        source->UpdateVTKObjects();
        }
      }

    // Set the default source ...
    if(sources.size())
      {
      source_property->RemoveAllProxies();
      source_property->AddProxy(sources[0]);
      this->Implementation->UI.seedType->setCurrentIndex(0);
      this->Implementation->PointSourceWidget->setWidgetVisible(true);
      this->Implementation->LineSourceWidget->setWidgetVisible(false);
      }
    }
#endif

  pqNamedWidgets::link(
    &this->Implementation->UIContainer, this->proxy(), this->propertyManager());
  
  // TODO:  remove this as we should just accept the values we get from the
  // server manager
  this->Implementation->UI.seedType->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------
pqParticleTracerPanel::~pqParticleTracerPanel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqParticleTracerPanel::onViewChanged(pqRenderView* render_module)
{
  this->Implementation->PointSourceWidget->setRenderModule(render_module);
  this->Implementation->LineSourceWidget->setRenderModule(render_module);
}

//-----------------------------------------------------------------------------
void pqParticleTracerPanel::onSeedTypeChanged(int type)
{
  switch(type)
    {
    case 0:
      this->onUsePointSource();
      break;
    case 1:
      this->onUseLineSource();
      break;
    }
}

//-----------------------------------------------------------------------------
void pqParticleTracerPanel::onUsePointSource()
{
  if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->GetProperty("Source")))
    {
    const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
    for(int i = 0; i != sources.size(); ++i)
      {
      pqSMProxy source = sources[i];
      if(source->GetVTKClassName() == QString("vtkPointSource"))
        {
        this->Implementation->PointSourceWidget->setWidgetVisible(true);
        this->Implementation->LineSourceWidget->setWidgetVisible(false);
        source_property->RemoveAllProxies();
        source_property->AddProxy(source);
        this->proxy()->UpdateVTKObjects();
        pqApplicationCore::instance()->render();
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqParticleTracerPanel::onUseLineSource()
{
  if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
    this->proxy()->GetProperty("Source")))
    {
    const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
    for(int i = 0; i != sources.size(); ++i)
      {
      pqSMProxy source = sources[i];
      if(source->GetVTKClassName() == QString("vtkLineSource"))
        {
        this->Implementation->PointSourceWidget->setWidgetVisible(false);
        this->Implementation->LineSourceWidget->setWidgetVisible(true);
        source_property->RemoveAllProxies();
        source_property->AddProxy(source);
        this->proxy()->UpdateVTKObjects();
        pqApplicationCore::instance()->render();
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqParticleTracerPanel::accept()
{
  int seedType = this->Implementation->UI.seedType->currentIndex();
  if(seedType == 0)  // point source
    {
    if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
      this->proxy()->GetProperty("Source")))
      {
      const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
      for(int i = 0; i != sources.size(); ++i)
        {
        pqSMProxy source = sources[i];
        if(source->GetVTKClassName() == QString("vtkPointSource"))
          {
          pqSMAdaptor::setProxyProperty(source_property, source);
          break;
          }
        }
      }
    }
  else if(seedType == 1) // line source
    {
    if(vtkSMProxyProperty* const source_property = vtkSMProxyProperty::SafeDownCast(
      this->proxy()->GetProperty("Source")))
      {
      const QList<pqSMProxy> sources = pqSMAdaptor::getProxyPropertyDomain(source_property);
      for(int i = 0; i != sources.size(); ++i)
        {
        pqSMProxy source = sources[i];
        if(source->GetVTKClassName() == QString("vtkLineSource"))
          {
          pqSMAdaptor::setProxyProperty(source_property, source);
          break;
          }
        }
      }
    }
  pqObjectPanel::accept();
}

