/*=========================================================================

   Program: ParaView
   Module:    pqCutPanel.cxx

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
#include "pqCutPanel.h"
#include "pqImplicitPlaneWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqSampleScalarWidget.h"
#include "pqServerManagerModel.h"

#include <vtkPVXMLElement.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>

#include <QFrame>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqCutPanel::pqImplementation

class pqCutPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    ImplicitPlaneWidget(parent),
    SampleScalarWidget(parent)
  {
  }
  
  /// Manages a 3D implicit plane widget, plus Qt controls  
  pqImplicitPlaneWidget ImplicitPlaneWidget;
  /// Controls the number and position of "slices"
  pqSampleScalarWidget SampleScalarWidget;
};

pqCutPanel::pqCutPanel(QWidget* p) :
  Superclass(p),
  Implementation(new pqImplementation(this))
{
  QFrame* const separator = new QFrame();
  separator->setFrameShape(QFrame::HLine);

  QVBoxLayout* const panel_layout = new QVBoxLayout();
  panel_layout->addWidget(&this->Implementation->ImplicitPlaneWidget);
  panel_layout->addWidget(separator);
  panel_layout->addWidget(&this->Implementation->SampleScalarWidget);
  this->setLayout(panel_layout);

  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetStartInteraction()),
    &this->Implementation->ImplicitPlaneWidget,
    SLOT(showPlane()));
  
  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetEndInteraction()),
    &this->Implementation->ImplicitPlaneWidget,
    SLOT(hidePlane()));
  
  connect(
    &this->Implementation->ImplicitPlaneWidget,
    SIGNAL(widgetChanged()),
    this->getPropertyManager(),
    SLOT(propertyChanged()));
    
  connect(
    &this->Implementation->SampleScalarWidget,
    SIGNAL(samplesChanged()),
    this->getPropertyManager(),
    SLOT(propertyChanged()));
  
  connect(
    this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
    
  connect(
    this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqCutPanel::~pqCutPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqCutPanel::onAccepted()
{
  this->Implementation->ImplicitPlaneWidget.accept();
  this->Implementation->SampleScalarWidget.accept();
      
  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter =
    qobject_cast<pqPipelineFilter*>(this->Proxy))
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

void pqCutPanel::onRejected()
{
  this->Implementation->ImplicitPlaneWidget.reset();
  this->Implementation->SampleScalarWidget.reset();
}

void pqCutPanel::setProxyInternal(pqProxy* p)
{
  Superclass::setProxyInternal(p);

  // Setup the implicit plane widget ...
  pqProxy* reference_proxy = this->Proxy;
  pqSMProxy controlled_proxy = NULL;
   
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const cut_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->getProxy()->GetProperty("CutFunction")))
      {
      controlled_proxy = cut_function_property->GetProxy(0);
      }
    }
  this->Implementation->ImplicitPlaneWidget.setReferenceProxy(reference_proxy);
  this->Implementation->ImplicitPlaneWidget.setControlledProxy(controlled_proxy);

  if (controlled_proxy)
    {
    vtkPVXMLElement* hints = controlled_proxy->GetHints();
    for (unsigned int cc=0; cc <hints->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* elem = hints->GetNestedElement(cc);
      if (QString("PropertyGroup") == elem->GetName() && 
        QString("Plane") == elem->GetAttribute("type"))
        {
        this->Implementation->ImplicitPlaneWidget.setHints(elem);
        break;
        }
      }
    }

  // Setup the sample scalar widget ...
  this->Implementation->SampleScalarWidget.setDataSources(
    this->Proxy ? this->Proxy->getProxy() : NULL,
    this->Proxy ?
    vtkSMDoubleVectorProperty::SafeDownCast(this->Proxy->getProxy()->GetProperty("ContourValues")) : 0);
}

void pqCutPanel::select()
{
  this->Implementation->ImplicitPlaneWidget.select();
}

void pqCutPanel::deselect()
{
  this->Implementation->ImplicitPlaneWidget.deselect();
}
