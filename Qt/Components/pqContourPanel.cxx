/*=========================================================================

   Program: ParaView
   Module:    pqContourPanel.cxx

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

#include "pqApplicationCore.h"
#include "pqContourPanel.h"
#include "pqNamedWidgets.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqSampleScalarWidget.h"
#include "pqServerManagerModel.h"

#include "ui_pqContourControls.h"

#include <pqCollapsedGroup.h>

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QCheckBox>
#include <QVBoxLayout>

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

QString pqContourPanelInterface::name() const
{
  return "Contour";
}

pqObjectPanel* pqContourPanelInterface::createPanel(pqProxy& proxy, QWidget* p)
{
  return new pqContourPanel(proxy, p);
}

bool pqContourPanelInterface::canCreatePanel(pqProxy& proxy) const
{
  return (proxy.getProxy()->GetXMLName() == QString("Contour") 
    && proxy.getProxy()->GetXMLGroup() == QString("filters"));
}

Q_EXPORT_PLUGIN(pqContourPanelInterface)
Q_IMPORT_PLUGIN(pqContourPanelInterface)


//////////////////////////////////////////////////////////////////////////////
// pqContourPanel::pqImplementation

class pqContourPanel::pqImplementation
{
public:
  pqImplementation() :
    SampleScalarWidget()
  {
  }

  /// Provides a container for Qt controls
  QWidget ControlsContainer;
  /// Provides the Qt controls for the panel
  Ui::pqContourControls Controls;
  /// Controls the number and values of contours
  pqSampleScalarWidget SampleScalarWidget;
};

pqContourPanel::pqContourPanel(pqProxy& object_proxy, QWidget* p) :
  base(object_proxy, p),
  Implementation(new pqImplementation())
{
  this->Implementation->Controls.setupUi(
    &this->Implementation->ControlsContainer);

  pqCollapsedGroup* const group1 = new pqCollapsedGroup(tr("Contour"));
  group1->setWidget(&this->Implementation->ControlsContainer);

  pqCollapsedGroup* const group2 = new pqCollapsedGroup(tr("Isosurfaces"));
  group2->setWidget(&this->Implementation->SampleScalarWidget);
  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->setMargin(0);
  panel_layout->setSpacing(0);
  panel_layout->addWidget(group1);
  panel_layout->addWidget(group2);
  panel_layout->addStretch();
  
  this->setLayout(panel_layout);

  connect(
    &this->Implementation->SampleScalarWidget,
    SIGNAL(samplesChanged()),
    &this->propertyManager(),
    SLOT(propertyChanged()));
    
  connect(&this->propertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(&this->propertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));

  // Setup the sample scalar widget ...
  this->Implementation->SampleScalarWidget.setDataSources(
    this->proxy().getProxy(),
    vtkSMDoubleVectorProperty::SafeDownCast(this->proxy().getProxy()->GetProperty("ContourValues")),
    this->proxy().getProxy()->GetProperty("SelectInputScalars"));
    
  pqNamedWidgets::link(
    &this->Implementation->ControlsContainer, this->proxy().getProxy(), &this->propertyManager());
}

pqContourPanel::~pqContourPanel()
{
  delete this->Implementation;
}

void pqContourPanel::onAccepted()
{
  this->Implementation->SampleScalarWidget.accept();
  
  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter =
    qobject_cast<pqPipelineFilter*>(&this->proxy()))
    {
    if(0 == pipeline_filter->getDisplayCount())
      {
      for(int i = 0; i != pipeline_filter->getInputCount(); ++i)
        {
        if(pqPipelineSource* const pipeline_source = pipeline_filter->getInput(i))
          {
          for(int j = 0; j != pipeline_source->getDisplayCount(); ++j)
            {
            pqPipelineDisplay* const pipeline_display =
              pipeline_source->getDisplay(j);
              
            vtkSMDataObjectDisplayProxy* const display_proxy =
              pipeline_display->getDisplayProxy();

            display_proxy->SetVisibilityCM(false);
            }
          }
        }
      }
    }
}

void pqContourPanel::onRejected()
{
  this->Implementation->SampleScalarWidget.reset();
}
