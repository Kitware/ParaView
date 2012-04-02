/*=========================================================================

   Program:   ParaQ
   Module:    pqStreamTracerPanel.cxx

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
#include "pqStreamTracerPanel.h"

#include "pqApplicationCore.h"
#include "pqLineSourceWidget.h"
#include "pqNamedWidgets.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPointSourceWidget.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include "ui_pqStreamTracerPanel.h"

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
// pqStreamTracerPanel::pqImplementation

class pqStreamTracerPanel::pqImplementation
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
  Ui::pqStreamTracerPanel UI;
};

//-----------------------------------------------------------------------------
pqStreamTracerPanel::pqStreamTracerPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p),
  Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(&this->Implementation->UIContainer);

  // Get the boundaries of the new proxy ...
  double proxy_center[3] = {0.0, 0.0, 0.0};

  if(vtkSMInputProperty* const input_property =
    vtkSMInputProperty::SafeDownCast(
      this->proxy()->GetProperty("Input")))
    {
    if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
        input_property->GetProxy(0)))
      {
      double input_bounds[6];
      input_proxy->GetDataInformation(
        input_property->GetOutputPortForConnection(0))->GetBounds(
        input_bounds);

      proxy_center[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
      proxy_center[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
      proxy_center[2] = (input_bounds[4] + input_bounds[5]) / 2.0;
      }
    }


  vtkSMProperty* const source_property =
    this->proxy()->GetProperty("Source");

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

      this->Implementation->PointSourceWidget =
        new pqPointSourceWidget(this->proxy(), source, NULL);
      this->Implementation->PointSourceWidget->hideWidget();

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
      this->Implementation->LineSourceWidget =
        new pqLineSourceWidget(this->proxy(), source, NULL);
      this->Implementation->LineSourceWidget->hideWidget();

      if(vtkPVXMLElement* const hints = source->GetHints())
        {
        for(unsigned int cc=0; cc <hints->GetNumberOfNestedElements(); cc++)
          {
          if(vtkPVXMLElement* const elem = hints->GetNestedElement(cc))
            {
            if (QString("PropertyGroup") == elem->GetName() &&
              QString("LineSource") == elem->GetAttribute("type"))
              {
              this->Implementation->LineSourceWidget->setHints(elem);
              break;
              }
            }
          }
        }

      }
    }


  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->setMargin(0);
  panel_layout->setSpacing(0);
  panel_layout->addWidget(&this->Implementation->UIContainer);
  panel_layout->addStretch();

  QVBoxLayout* l = new QVBoxLayout(this->Implementation->UI.pointSource);
  l->setMargin(0);
  this->Implementation->PointSourceWidget->layout()->setMargin(0);
  l->addWidget(this->Implementation->PointSourceWidget);
  l->addStretch();

  l = new QVBoxLayout(this->Implementation->UI.lineSource);
  l->setMargin(0);
  this->Implementation->LineSourceWidget->layout()->setMargin(0);
  l->addWidget(this->Implementation->LineSourceWidget);
  l->addStretch();

  pqSMProxy current = pqSMAdaptor::getProxyProperty(source_property);

  int idx = sources.indexOf(current);
  this->Implementation->UI.seedType->setCurrentIndex(idx);
  if(idx == 0)
    {
    this->Implementation->UI.stackedWidget->setCurrentWidget(
                    this->Implementation->UI.pointSource);
    this->Implementation->LineSourceWidget->setWidgetVisible(false);
    this->Implementation->PointSourceWidget->setWidgetVisible(true);
    }
  else
    {
    this->Implementation->UI.stackedWidget->setCurrentWidget(
                    this->Implementation->UI.lineSource);
    this->Implementation->PointSourceWidget->setWidgetVisible(false);
    this->Implementation->LineSourceWidget->setWidgetVisible(true);
    }

  this->Implementation->LineSourceWidget->resetBounds();
  this->Implementation->PointSourceWidget->resetBounds();
  this->Implementation->LineSourceWidget->reset();
  this->Implementation->PointSourceWidget->reset();


  QObject::connect(this->Implementation->UI.IntegratorType,
    SIGNAL(currentIndexChanged(int)),
    this,
    SLOT(onIntegratorTypeChanged(int)));

  QObject::connect(this->Implementation->UI.seedType,
    SIGNAL(currentIndexChanged(int)),
    this,
    SLOT(onSeedTypeChanged(int)));

  QObject::connect(this, SIGNAL(viewChanged(pqView*)),
    this->Implementation->PointSourceWidget, SLOT(setView(pqView*)));
  QObject::connect(this, SIGNAL(viewChanged(pqView*)),
    this->Implementation->LineSourceWidget, SLOT(setView(pqView*)));

  QObject::connect(
    this, SIGNAL(onaccept()), this->Implementation->PointSourceWidget, SLOT(accept()));
  QObject::connect(
    this, SIGNAL(onaccept()), this->Implementation->LineSourceWidget, SLOT(accept()));
  QObject::connect(
    this, SIGNAL(onreset()), this->Implementation->PointSourceWidget, SLOT(reset()));
  QObject::connect(
    this, SIGNAL(onreset()), this->Implementation->LineSourceWidget, SLOT(reset()));

  pqNamedWidgets::link(this, this->proxy(),
    this->propertyManager());

  QObject::connect(
    this->Implementation->PointSourceWidget, SIGNAL(modified()),
    this, SLOT(setModified()));

  QObject::connect(
    this->Implementation->LineSourceWidget, SIGNAL(modified()),
    this, SLOT(setModified()));

  QObject::connect(object_proxy, SIGNAL(producerChanged(const QString&)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();

}

//-----------------------------------------------------------------------------
pqStreamTracerPanel::~pqStreamTracerPanel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqStreamTracerPanel::onSeedTypeChanged(int type)
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
void pqStreamTracerPanel::onUsePointSource()
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
        this->Implementation->UI.stackedWidget->setCurrentWidget(
                        this->Implementation->UI.pointSource);
        if(this->selected())
          {
          this->Implementation->PointSourceWidget->select();
          this->Implementation->LineSourceWidget->deselect();
          }
        this->Implementation->PointSourceWidget->resetBounds();
        this->Implementation->PointSourceWidget->setWidgetVisible(true);
        this->Implementation->LineSourceWidget->setWidgetVisible(false);
        pqSMAdaptor::setUncheckedProxyProperty(source_property, source);
        this->setModified();
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqStreamTracerPanel::onUseLineSource()
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
        this->Implementation->UI.stackedWidget->setCurrentWidget(
                        this->Implementation->UI.lineSource);
        if(this->selected())
          {
          this->Implementation->PointSourceWidget->deselect();
          this->Implementation->LineSourceWidget->select();
          }
        this->Implementation->LineSourceWidget->resetBounds();
        this->Implementation->PointSourceWidget->setWidgetVisible(false);
        this->Implementation->LineSourceWidget->setWidgetVisible(true);
        pqSMAdaptor::setUncheckedProxyProperty(source_property, source);
        this->setModified();
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqStreamTracerPanel::accept()
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

//-----------------------------------------------------------------------------
void pqStreamTracerPanel::onIntegratorTypeChanged(int index)
{
  bool enabled = false;
  QString type = this->Implementation->UI.IntegratorType->itemText(index);
  if(type == "Runge-Kutta 4-5")
    {
    enabled = true;
    }
  this->Implementation->UI.MinimumIntegrationStep->setEnabled(enabled);
  this->Implementation->UI.MaximumIntegrationStep->setEnabled(enabled);
  this->Implementation->UI.MaximumError->setEnabled(enabled);
}

void pqStreamTracerPanel::select()
{
  pqObjectPanel::select();
  if(this->Implementation->UI.seedType->currentIndex() == 0)
    {
    this->Implementation->PointSourceWidget->select();
    }
  else
    {
    this->Implementation->LineSourceWidget->select();
    }
}

void pqStreamTracerPanel::deselect()
{
  pqObjectPanel::deselect();
  if(this->Implementation->UI.seedType->currentIndex() == 0)
    {
    this->Implementation->PointSourceWidget->deselect();
    }
  else
    {
    this->Implementation->LineSourceWidget->deselect();
    }
}

void pqStreamTracerPanel::updateEnableState()
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(
    this->referenceProxy());
  pqOutputPort* cur_input = NULL;
  if (filter)
    {
    QList<pqOutputPort*> ports = filter->getAllInputs();
    cur_input = ports.size() > 0? ports[0] : NULL;
    }

  bool isAMR = false;
  if (cur_input)
    {
    vtkPVDataInformation* dataInfo = cur_input->getDataInformation();
    isAMR = dataInfo->GetCompositeDataSetType() == VTK_NON_OVERLAPPING_AMR;
    }
  this->Implementation->UI.InterpolatorType->setEnabled(!isAMR);
}


