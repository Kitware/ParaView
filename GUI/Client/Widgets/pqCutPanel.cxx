/*=========================================================================

   Program:   ParaQ
   Module:    pqCutPanel.cxx

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
#include "pqCutPanel.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyProperty.h>

#include <QLineEdit>
#include <QPushButton>

pqCutPanel::pqCutPanel(QWidget* p) :
  pqWidgetObjectPanel(":/pqWidgets/CutPanel.ui", p),
  IgnoreQtWidgets(false),
  Ignore3DWidget(false)
{
  QLineEdit* const originX = this->findChild<QLineEdit*>("originX");
  QLineEdit* const originY = this->findChild<QLineEdit*>("originY");
  QLineEdit* const originZ = this->findChild<QLineEdit*>("originZ");
  QLineEdit* const normalX = this->findChild<QLineEdit*>("normalX");
  QLineEdit* const normalY = this->findChild<QLineEdit*>("normalY");
  QLineEdit* const normalZ = this->findChild<QLineEdit*>("normalZ");
  
  connect(originX, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(originY, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(originZ, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(normalX, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(normalY, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(normalZ, SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));

  connect(this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

/// destructor
pqCutPanel::~pqCutPanel()
{
  this->setProxy(NULL);
}

void pqCutPanel::setProxyInternal(pqSMProxy p)
{
  pqWidgetObjectPanel::setProxyInternal(p);
  
  if(!this->Proxy)
    return;
  
  this->pullImplicitPlane();
}

void pqCutPanel::onQtWidgetChanged()
{
  if(IgnoreQtWidgets)
    return;

  // Get the new values from the Qt widgets ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };

  QLineEdit* const originX = this->findChild<QLineEdit*>("originX");
  QLineEdit* const originY = this->findChild<QLineEdit*>("originY");
  QLineEdit* const originZ = this->findChild<QLineEdit*>("originZ");
  QLineEdit* const normalX = this->findChild<QLineEdit*>("normalX");
  QLineEdit* const normalY = this->findChild<QLineEdit*>("normalY");
  QLineEdit* const normalZ = this->findChild<QLineEdit*>("normalZ");

  origin[0] = originX->text().toDouble();
  origin[1] = originY->text().toDouble();
  origin[2] = originZ->text().toDouble();
  normal[0] = normalX->text().toDouble();
  normal[1] = normalY->text().toDouble();
  normal[2] = normalZ->text().toDouble();
  
  // Push the new values into the 3D widget (ideally, this should happen automatically when the implicit plane is updated)
  this->update3DWidget(origin, normal);
  
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqCutPanel::on3DWidgetChanged()
{
  if(Ignore3DWidget)
    return;
    
  // Get the new values from the 3D widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Origin")))
      {
      origin[0] = widget_origin->GetElement(0);
      origin[1] = widget_origin->GetElement(1);
      origin[2] = widget_origin->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      normal[0] = widget_normal->GetElement(0);
      normal[1] = widget_normal->GetElement(1);
      normal[2] = widget_normal->GetElement(2);
      }
    }
  
  // Push the new values into the Qt widgets (ideally, this should happen automatically when the implicit plane is updated)
  this->updateQtWidgets(origin, normal);

  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqCutPanel::pullImplicitPlane()
{
  // Get the current values from the implicit plane ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("CutFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          origin[0] = plane_origin->GetElement(0);
          origin[1] = plane_origin->GetElement(1);
          origin[2] = plane_origin->GetElement(2);
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          normal[0] = plane_normal->GetElement(0);
          normal[1] = plane_normal->GetElement(1);
          normal[2] = plane_normal->GetElement(2);
          }
        }
      }
    }

  // Push the current values into the Qt widgets ...
  this->updateQtWidgets(origin, normal);
  
  // Push the current values into the 3D widget ...
  this->update3DWidget(origin, normal);
}

void pqCutPanel::updateQtWidgets(const double* origin, const double* normal)
{
  this->IgnoreQtWidgets = true;
  
  QLineEdit* const originX = this->findChild<QLineEdit*>("originX");
  QLineEdit* const originY = this->findChild<QLineEdit*>("originY");
  QLineEdit* const originZ = this->findChild<QLineEdit*>("originZ");
  QLineEdit* const normalX = this->findChild<QLineEdit*>("normalX");
  QLineEdit* const normalY = this->findChild<QLineEdit*>("normalY");
  QLineEdit* const normalZ = this->findChild<QLineEdit*>("normalZ");

  originX->setText(QString::number(origin[0], 'g', 3));  
  originY->setText(QString::number(origin[1], 'g', 3));  
  originZ->setText(QString::number(origin[2], 'g', 3));  
  normalX->setText(QString::number(normal[0], 'g', 3));  
  normalY->setText(QString::number(normal[1], 'g', 3));  
  normalZ->setText(QString::number(normal[2], 'g', 3));
  
  this->IgnoreQtWidgets = false;
}

void pqCutPanel::update3DWidget(const double* origin, const double* normal)
{
  this->Ignore3DWidget = true;
   
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Origin")))
      {
      widget_origin->SetElements(origin);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      widget_normal->SetElements(normal);
      }
    
    this->Widget->UpdateVTKObjects();
    
    pqApplicationCore::instance()->render();
    }
    
  this->Ignore3DWidget = false;
}

void pqCutPanel::pushImplicitPlane(const double* origin, const double* normal)
{
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("CutFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          plane_origin->SetElements(origin);
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          plane_normal->SetElements(normal);
          }

        clip_function->UpdateVTKObjects();
        }
      }
    }
}

void pqCutPanel::onAccepted()
{
  // Get the updated values from the 3D widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Origin")))
      {
      origin[0] = widget_origin->GetElement(0);
      origin[1] = widget_origin->GetElement(1);
      origin[2] = widget_origin->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Widget->GetProperty("Normal")))
      {
      normal[0] = widget_normal->GetElement(0);
      normal[1] = widget_normal->GetElement(1);
      normal[2] = widget_normal->GetElement(2);
      }
    }
  
  this->pushImplicitPlane(origin, normal);
  
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
              pipeline_display->getProxy();

            display_proxy->SetVisibilityCM(false);
            }
          }
        }
      }
    }
}

void pqCutPanel::onRejected()
{
  this->pullImplicitPlane();
}

