/*=========================================================================

   Program:   ParaQ
   Module:    pqContourPanel.cxx

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
#include "pqContourPanel.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqSampleScalarWidget.h"
#include "pqServerManagerModel.h"
#include "pqArrayMenu.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqContourPanel::pqImplementation

class pqContourPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    ArrayWidget(parent),
    ComputeNormalsWidget(tr("Compute Normals")),
    ComputeGradientsWidget(tr("Compute Gradients")),
    ComputeScalarsWidget(tr("Compute Scalars")),
    SampleScalarWidget(parent)
  {
  }
  
  /// Provides a Qt control for selecting the array to be used for computing contours
  pqArrayMenu ArrayWidget;
  /// Provides a Qt control for the "Compute Normals" property of the Contour filter
  QCheckBox ComputeNormalsWidget;
  /// Provides a Qt control for the "Compute Gradients" property of the Contour filter
  QCheckBox ComputeGradientsWidget;
  /// Provides a Qt control for the "Compute Scalars" property of the Contour filter
  QCheckBox ComputeScalarsWidget;
  /// Controls the number and values of contours
  pqSampleScalarWidget SampleScalarWidget;
};

pqContourPanel::pqContourPanel(QWidget* p) :
  base(p),
  Implementation(new pqImplementation(this))
{
  QFrame* const separator = new QFrame();
  separator->setFrameShape(QFrame::HLine);

  QVBoxLayout* const panel_layout = new QVBoxLayout();
  panel_layout->addWidget(&this->Implementation->ArrayWidget);
  panel_layout->addWidget(&this->Implementation->ComputeNormalsWidget);
  panel_layout->addWidget(&this->Implementation->ComputeGradientsWidget);
  panel_layout->addWidget(&this->Implementation->ComputeScalarsWidget);
  panel_layout->addWidget(separator);
  panel_layout->addWidget(&this->Implementation->SampleScalarWidget);
  this->setLayout(panel_layout);

  connect(
    &this->Implementation->ArrayWidget,
    SIGNAL(arrayChanged()),
    this,
    SLOT(onArrayWidgetChanged()));
  
  connect(
    &this->Implementation->SampleScalarWidget,
    SIGNAL(samplesChanged()),
    this,
    SLOT(onSampleScalarWidgetChanged()));
    
  connect(this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqContourPanel::~pqContourPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqContourPanel::onArrayWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqContourPanel::onSampleScalarWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqContourPanel::onAccepted()
{
  // Get the current values from the variable selection widget and push them to the filter
  pqVariableType type;
  QString name;
  this->Implementation->ArrayWidget.getCurrent(type, name);
  
  if(this->Proxy)
    {
    if(vtkSMStringVectorProperty* const array =
      vtkSMStringVectorProperty::SafeDownCast(this->Proxy->GetProperty("SelectInputScalars")))
      {
      array->SetNumberOfElements(5);
      array->SetElement(0, 0);
      array->SetElement(1, 0);
      array->SetElement(2, 0);
      array->SetElement(3, 0);
      array->SetElement(4, name.toAscii().data());
      }
      
    this->Proxy->UpdateVTKObjects();
    }

/*  
    if (!(this->FieldMenu && svp->GetUncheckedElement(3)))
      {
      // Don't reset the unchecked element if it has already been set
      // by changing the value of the field menu on which this array menu
      // depends. (This is used by the Threshold filter.)
      svp->SetUncheckedElement(3, svp->GetElement(3));
      }
    svp->SetUncheckedElement(0, this->InputAttributeIndex);
    svp->SetUncheckedElement(1, svp->GetElement(1));
    svp->SetUncheckedElement(2, svp->GetElement(2));
    svp->SetUncheckedElement(4, this->ArrayName);
    svp->UpdateDependentDomains();
*/

  // Get the current values from the sample scalar widget and push them to the filter
  const QList<double> samples = this->Implementation->SampleScalarWidget.getSamples();
  
  // Push the new values into the cut filter ...  
  if(this->Proxy)
    {
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      contours->SetNumberOfElements(samples.size());
      for(int i = 0; i != samples.size(); ++i)
        {
        contours->SetElement(i, samples[i]);
        }
        
      this->Proxy->UpdateVTKObjects();
      }
    }
  
  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter =
    dynamic_cast<pqPipelineFilter*>(pqServerManagerModel::instance()->getPQSource(this->Proxy)))
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
  // Reset the state of the sample scalar widget ...
  QList<double> values;
  if(this->Proxy)
    {
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      const int value_count = contours->GetNumberOfElements();
      for(int i = 0; i != value_count; ++i)
        {
        values.push_back(contours->GetElement(i));
        }
      }
    }
  this->Implementation->SampleScalarWidget.setSamples(values);
}

void pqContourPanel::setProxyInternal(pqSMProxy p)
{
  base::setProxyInternal(p);
 
  if(!this->Proxy)
    return;
  
  // Setup the variable selection widget ...
  this->Implementation->ArrayWidget.clear();
  if(pqPipelineFilter* const pipeline_filter =
    dynamic_cast<pqPipelineFilter*>(
    pqServerManagerModel::instance()->getPQSource(this->Proxy)))
    {
    for(int i = 0; i != pipeline_filter->getInputCount(); ++i)
      {
      if(pqPipelineSource* const pipeline_source = pipeline_filter->getInput(i))
        {
        this->Implementation->ArrayWidget.add(
          vtkSMSourceProxy::SafeDownCast(pipeline_source->getProxy()));
        }
      }
    }
  
  // Setup the sample scalar widget ...
  QList<double> values;
  
  if(this->Proxy)
    {
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      const int value_count = contours->GetNumberOfElements();
      for(int i = 0; i != value_count; ++i)
        {
        values.push_back(contours->GetElement(i));
        }
      }
    }
    
  this->Implementation->SampleScalarWidget.setSamples(values);
}

void pqContourPanel::select()
{
  this->PropertyManager->registerLink(
    &this->Implementation->ComputeNormalsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeNormals"));

  this->PropertyManager->registerLink(
    &this->Implementation->ComputeGradientsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeGradients"));

  this->PropertyManager->registerLink(
    &this->Implementation->ComputeScalarsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeScalars"));
}

void pqContourPanel::deselect()
{
  this->PropertyManager->unregisterLink(
    &this->Implementation->ComputeNormalsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeNormals"));

  this->PropertyManager->unregisterLink(
    &this->Implementation->ComputeGradientsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeGradients"));

  this->PropertyManager->unregisterLink(
    &this->Implementation->ComputeScalarsWidget, "checked", SIGNAL(toggled(bool)),
    this->Proxy, this->Proxy->GetProperty("ComputeScalars"));
}
