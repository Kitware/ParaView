/*=========================================================================

   Program: ParaView
   Module:    pq3DViewPropertiesWidget.cxx

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
#include "pq3DViewPropertiesWidget.h"
#include "ui_pq3DViewPropertiesWidget.h"

// ParaView Server Manager includes.
#include "vtkSMProxy.h"
#include "vtkSMLODRenderModuleProxy.h"
#include "vtkSMCompositeRenderModuleProxy.h"
#include "vtkSMIceTDesktopRenderModuleProxy.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QDoubleValidator>

// ParaView Client includes.
#include "pqApplicationCore.h"
#include "pqPropertyManager.h"
#include "pqRenderViewModule.h"
#include "pqSettings.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqNamedWidgets.h"

//-----------------------------------------------------------------------------
class pq3DViewPropertiesWidgetInternal : public Ui::pq3DViewProperties
{
public:
  pqPropertyManager Links;
  pqSignalAdaptorColor *ColorAdaptor;
  QPointer<pqGenericViewModule> ViewModule;

  pq3DViewPropertiesWidgetInternal() 
    {
    this->ColorAdaptor = 0;
    }

  ~pq3DViewPropertiesWidgetInternal()
    {
    delete this->ColorAdaptor;
    }

  void updateLODThresholdLabel(int value)
    {
    this->lodThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MBytes");
    }

  void updateLODResolutionLabel(int value)
    {
    QVariant val(160-value + 10);

    this->lodResolutionLabel->setText(
      val.toString() + "x" + val.toString() + "x" + val.toString());
    }
  void updateOutlineThresholdLabel(int value)
    {
    this->outlineThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MCells");
    }

  void updateCompositeThresholdLabel(int value)
    {
    this->compositeThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MBytes");
    }
  void updateSubsamplingRateLabel(int value)
    {
    this->subsamplingRateLabel->setText(QVariant(value).toString() 
      + " Pixels");
    }
  void updateSquirtLevelLabel(int val)
    {
    static int bitValues[] = {24, 24, 22, 19, 16, 13, 10};
    val = (val < 0 )? 0 : val;
    val = ( val >6)? 6 : val;
    this->squirtLevelLabel->setText(
      QVariant(bitValues[val]).toString() + " Bits");
    }


  void loadValues(pqGenericViewModule* proxy);
  void accept();
};

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::loadValues(pqGenericViewModule* viewModule)
{
  if (!this->ColorAdaptor)
    {
    this->ColorAdaptor = new pqSignalAdaptorColor(this->backgroundColor, 
      "chosenColor", SIGNAL(chosenColorChanged(const QColor&)), false);
    }
  
  this->ViewModule = viewModule;
  vtkSMProxy* proxy = viewModule->getProxy();

  this->Links.registerLink(this->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("Background"));

  this->Links.registerLink(this->parallelProjection, "checked",
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("CameraParallelProjection"));

  this->Links.registerLink(this->triangleStrips, "checked", 
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("UseTriangleStrips"));

  this->Links.registerLink(this->immediateModeRendering, "checked", 
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("UseImmediateMode"));

  // link default light params
  this->Links.registerLink(this->DefaultLightSwitch, "checked", 
                           SIGNAL(toggled(bool)),
                           proxy, proxy->GetProperty("LightSwitch"));
  pqSignalAdaptorSliderRange* sliderAdaptor;
  sliderAdaptor = new pqSignalAdaptorSliderRange(this->LightIntensity);
  this->Links.registerLink(sliderAdaptor, "value",
                           SIGNAL(valueChanged(double)),
                           proxy, proxy->GetProperty("LightIntensity"));
  this->Links.registerLink(this->LightIntensity_Edit, "text",
                           SIGNAL(textChanged(const QString&)),
                           proxy, proxy->GetProperty("LightIntensity"));
  pqSignalAdaptorColor* lightColorAdaptor;
  lightColorAdaptor = new pqSignalAdaptorColor(this->SetLightColor,
                                   "chosenColor",
                                   SIGNAL(chosenColorChanged(const QColor&)),
                                   false);
  this->Links.registerLink(lightColorAdaptor, "color",
                           SIGNAL(colorChanged(const QVariant&)),
                           proxy, proxy->GetProperty("LightDiffuseColor"));


  // link light kit params
  pqNamedWidgets::link(this->UseLight, proxy, &this->Links);
  this->Links.registerLink(this->UseLight, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseLight"));



  if (vtkSMLODRenderModuleProxy::SafeDownCast(proxy))
    {
    this->lodParameters->setVisible(true);
    double val = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("LODThreshold")).toDouble();
    if (val >= VTK_LARGE_FLOAT)
      {
      this->enableLOD->setCheckState(Qt::Unchecked);
      this->updateLODThresholdLabel(this->lodThreshold->value());
      }
    else
      {
      this->enableLOD->setCheckState(Qt::Checked);
      this->lodThreshold->setValue(static_cast<int>(val*10));
      this->updateLODThresholdLabel(this->lodThreshold->value());
      }

    val = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("LODResolution")).toDouble();
    this->lodResolution->setValue(static_cast<int>(160-val + 10));
    this->updateLODResolutionLabel(this->lodResolution->value());

    this->Links.registerLink(this->renderingInterrupts, "checked",
      SIGNAL(stateChanged(int)),
      proxy, proxy->GetProperty("RenderInterruptsEnabled"));
    }
  else
    {
    this->lodParameters->setVisible(false);
    }

  if (vtkSMCompositeRenderModuleProxy::SafeDownCast(proxy))
    {
    this->compositingParameters->setVisible(true);
    double val = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("CompositeThreshold")).toDouble();
    if (val >= VTK_LARGE_FLOAT)
      {
      this->enableCompositing->setCheckState(Qt::Unchecked);
      this->updateCompositeThresholdLabel(this->compositeThreshold->value());
      }
    else
      {
      this->enableCompositing->setCheckState(Qt::Checked);
      this->compositeThreshold->setValue(static_cast<int>(val*10));
      this->updateCompositeThresholdLabel(this->compositeThreshold->value());
      }

    int ival = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ReductionFactor")).toInt();
    if (ival == 1)
      {
      this->enableSubsampling->setCheckState(Qt::Unchecked);
      this->updateSubsamplingRateLabel(this->subsamplingRate->value());
      }
    else
      {
      this->enableSubsampling->setCheckState(Qt::Checked);
      this->subsamplingRate->setValue(ival);
      this->updateSubsamplingRateLabel(this->subsamplingRate->value());
      }

    ival = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("SquirtLevel")).toInt();
    if (ival == 0)
      {
      this->enableSquirt->setCheckState(Qt::Unchecked);
      this->updateSquirtLevelLabel(this->squirtLevel->value());
      }
    else
      {
      this->enableSquirt->setCheckState(Qt::Checked);
      this->squirtLevel->setValue(ival);
      this->updateSquirtLevelLabel(this->squirtLevel->value());
      }
    }
  else
    {
    this->compositingParameters->setVisible(false);
    }

  if (vtkSMIceTDesktopRenderModuleProxy::SafeDownCast(proxy))
    {
    // Only available for IceT.
    this->orderedCompositing->setVisible(true);
    this->Links.registerLink(this->orderedCompositing, "checked", 
      SIGNAL(stateChanged(int)),
      proxy, proxy->GetProperty("OrderedCompositing"));
    }
  else
    {
    this->orderedCompositing->setVisible(false);
    }

  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(this->ViewModule);
  if(rm)
    {
    this->OrientationAxes->setChecked(rm->getOrientationAxesVisibility());
    this->OrientationAxesInteraction->setCheckState(
      rm->getOrientationAxesInteractivity()? Qt::Checked : Qt::Unchecked);
    this->OrientationAxesOutlineColor->setChosenColor(
      rm->getOrientationAxesOutlineColor());
    this->OrientationAxesLabelColor->setChosenColor(
      rm->getOrientationAxesLabelColor());

    this->CustomCenter->setCheckState(Qt::Unchecked);
    this->AutoResetCenterOfRotation->setCheckState(
      rm->getResetCenterWithCamera()? Qt::Checked : Qt::Unchecked);
    this->CenterAxesVisibility->setCheckState(
      rm->getCenterAxesVisibility()? Qt::Checked : Qt::Unchecked);
    double center[3];
    rm->getCenterOfRotation(center);
    this->CenterX->setText(QString::number(center[0],'g',3));
    this->CenterY->setText(QString::number(center[1],'g',3));
    this->CenterZ->setText(QString::number(center[2],'g',3));
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::accept()
{

  if(!this->ViewModule)
    {
    return;
    }

  // We need to accept user changes.
  this->Links.accept();

  vtkSMProxy* renModule = this->ViewModule->getProxy();
  if (vtkSMLODRenderModuleProxy::SafeDownCast(renModule))
    {
    // Push changes for LOD parameters.
    if (this->enableLOD->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("LODThreshold"), 
        this->lodThreshold->value() / 10.0);

      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("LODResolution"),
        160-this->lodResolution->value() + 10);
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("LODThreshold"), VTK_DOUBLE_MAX);
      }
    }

  if (vtkSMCompositeRenderModuleProxy::SafeDownCast(renModule))
    {
    if (this->enableCompositing->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("CompositeThreshold"),
        this->compositeThreshold->value() / 10.0);
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("CompositeThreshold"), VTK_DOUBLE_MAX);
      }

    if (this->enableSubsampling->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("ReductionFactor"),
        this->subsamplingRate->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("ReductionFactor"), 1);
      }

    if (this->enableSquirt->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("SquirtLevel"),
        this->squirtLevel->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("SquirtLevel"), 0);
      }
    }
  renModule->UpdateVTKObjects();
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(this->ViewModule);
  if(rm)
    {
    rm->setOrientationAxesVisibility(this->OrientationAxes->isChecked());

    rm->setOrientationAxesInteractivity(
      this->OrientationAxesInteraction->checkState() == Qt::Checked);
    rm->setOrientationAxesOutlineColor(
      this->OrientationAxesOutlineColor->chosenColor());
    rm->setOrientationAxesLabelColor(
      this->OrientationAxesLabelColor->chosenColor());
  
    rm->setCenterAxesVisibility(
      this->CenterAxesVisibility->checkState() == Qt::Checked);
    rm->setResetCenterWithCamera(
      this->AutoResetCenterOfRotation->checkState() == Qt::Checked);
    if (this->CustomCenter->checkState() == Qt::Checked)
      {
      double center[3];
      center[0] = this->CenterX->text().toDouble();
      center[1] = this->CenterY->text().toDouble();
      center[2] = this->CenterZ->text().toDouble();
      rm->setCenterOfRotation(center);
      }
    rm->saveSettings();
    }
}

//-----------------------------------------------------------------------------
pq3DViewPropertiesWidget::pq3DViewPropertiesWidget(QWidget* _parent):
  QWidget(_parent)
{
  this->Internal = new pq3DViewPropertiesWidgetInternal;
  this->Internal->setupUi(this);

  QDoubleValidator* dv = new QDoubleValidator(this);
  this->Internal->CenterX->setValidator(dv);
  this->Internal->CenterY->setValidator(dv);
  this->Internal->CenterZ->setValidator(dv);

  this->Internal->label_3->hide();
  this->Internal->outlineThresholdLabel->hide();
  this->Internal->outlineThreshold->hide();
  QObject::connect(this->Internal->lodThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(lodThresholdSliderChanged(int)));

  QObject::connect(this->Internal->lodResolution,
    SIGNAL(valueChanged(int)), this, SLOT(lodResolutionSliderChanged(int)));

  QObject::connect(this->Internal->outlineThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(outlineThresholdSliderChanged(int)));

  QObject::connect(this->Internal->compositeThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(compositeThresholdSliderChanged(int)));

  QObject::connect(this->Internal->subsamplingRate,
    SIGNAL(valueChanged(int)), this, SLOT(subsamplingRateSliderChanged(int)));

  QObject::connect(this->Internal->squirtLevel,
    SIGNAL(valueChanged(int)), this, SLOT(squirtLevelRateSliderChanged(int)));
  
  QObject::connect(this->Internal->restoreDefault,
    SIGNAL(clicked(bool)), this, SLOT(restoreDefaultBackground()));
  
  QObject::connect(this->Internal->ResetLight,
    SIGNAL(clicked(bool)), this, SLOT(resetLights()));
  
}

//-----------------------------------------------------------------------------
pq3DViewPropertiesWidget::~pq3DViewPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::setRenderModule(pqGenericViewModule* renderModule)
{
  this->Internal->loadValues(renderModule);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::accept()
{
  this->Internal->accept();
  if(this->Internal->ViewModule)
    {
    this->Internal->ViewModule->render();
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::lodThresholdSliderChanged(int value)
{
  this->Internal->updateLODThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::lodResolutionSliderChanged(int value)
{
  this->Internal->updateLODResolutionLabel(value);
}
//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::outlineThresholdSliderChanged(int value)
{
  this->Internal->updateOutlineThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::compositeThresholdSliderChanged(int value)
{
  this->Internal->updateCompositeThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::subsamplingRateSliderChanged(int value)
{
  this->Internal->updateSubsamplingRateLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::squirtLevelRateSliderChanged(int value)
{
  this->Internal->updateSquirtLevelLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::restoreDefaultBackground()
{
  pqRenderViewModule* rvm =
    qobject_cast<pqRenderViewModule*>(this->Internal->ViewModule);
  if(rvm)
    {
    int* col = rvm->defaultBackgroundColor();
    this->Internal->backgroundColor->setChosenColor(
               QColor(col[0], col[1], col[2]));
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::resetLights()
{
  pqRenderViewModule* rvm =
    qobject_cast<pqRenderViewModule*>(this->Internal->ViewModule);
  if(rvm)
    {
    rvm->restoreDefaultLightSettings();
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
