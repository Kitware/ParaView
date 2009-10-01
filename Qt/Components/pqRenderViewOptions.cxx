/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewOptions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

/// \file pqRenderViewOptions.cxx
/// \date 7/20/2007

#include "pqRenderViewOptions.h"
#include "ui_pqRenderViewOptions.h"

#include <QPointer>

#include "vtkSMProxy.h"

#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqRenderView.h"
#include "pqSignalAdaptors.h"
#include "pqStandardColorLinkAdaptor.h"

class pqRenderViewOptions::pqInternal : public Ui::pqRenderViewOptions
{
public:
  QPointer<pqRenderView> RenderView;
  pqPropertyManager Links;
  pqPropertyManager LightsLinks;
  pqSignalAdaptorColor *ColorAdaptor;
};


//----------------------------------------------------------------------------
pqRenderViewOptions::pqRenderViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(this->Internal->backgroundColor, 
    "chosenColor", SIGNAL(chosenColorChanged(const QColor&)), false);


  // enable the apply button when things are changed
  QObject::connect(&this->Internal->Links, SIGNAL(modified()),
          this, SIGNAL(changesAvailable()));
  QObject::connect(&this->Internal->LightsLinks, SIGNAL(modified()),
          this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->restoreDefault,
    SIGNAL(clicked(bool)), this, SLOT(restoreDefaultBackground()));
  
  QObject::connect(this->Internal->ResetLight,
    SIGNAL(clicked(bool)), this, SLOT(resetLights()));
  
  QObject::connect(this->Internal->OrientationAxes,
          SIGNAL(toggled(bool)),
          this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->OrientationAxesInteraction,
          SIGNAL(toggled(bool)),
          this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->OrientationAxesOutlineColor,
          SIGNAL(chosenColorChanged(const QColor&)),
          this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->OrientationAxesLabelColor,
          SIGNAL(chosenColorChanged(const QColor&)),
          this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->CenterAxesVisibility,
          SIGNAL(toggled(bool)),
          this, SIGNAL(changesAvailable()));
}

pqRenderViewOptions::~pqRenderViewOptions()
{
  delete this->Internal;
}

void pqRenderViewOptions::setPage(const QString &page)
{
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    if(this->Internal->stackedWidget->widget(i)->objectName() == page)
      {
      this->Internal->stackedWidget->setCurrentIndex(i);
      break;
      }
    }
}

QStringList pqRenderViewOptions::getPageList()
{
  QStringList pages;
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    pages << this->Internal->stackedWidget->widget(i)->objectName();
    }
  return pages;
}
  
void pqRenderViewOptions::setView(pqView* view)
{
  // disconnect widgets from current render view
  this->disconnectGUI();
  this->Internal->RenderView = qobject_cast<pqRenderView*>(view);
  if(this->Internal->RenderView)
    {
    // connect widgets to current render view
    this->connectGUI();
    }
}

void pqRenderViewOptions::applyChanges()
{
  if(!this->Internal->RenderView)
    {
    return;
    }

  this->Internal->Links.accept();
  this->Internal->LightsLinks.accept();
  
  this->Internal->RenderView->setOrientationAxesVisibility(this->Internal->OrientationAxes->isChecked());

  this->Internal->RenderView->setOrientationAxesInteractivity(
    this->Internal->OrientationAxesInteraction->checkState() == Qt::Checked);
  this->Internal->RenderView->setOrientationAxesOutlineColor(
    this->Internal->OrientationAxesOutlineColor->chosenColor());
  this->Internal->RenderView->setOrientationAxesLabelColor(
    this->Internal->OrientationAxesLabelColor->chosenColor());

  this->Internal->RenderView->setCenterAxesVisibility(
    this->Internal->CenterAxesVisibility->checkState() == Qt::Checked);
  
  this->Internal->RenderView->saveSettings();

  // update the view after changes
  this->Internal->RenderView->render();
}

void pqRenderViewOptions::resetChanges()
{
  this->Internal->Links.reject();
  this->Internal->LightsLinks.reject();
  this->resetAnnotation();
}

void pqRenderViewOptions::connectGUI()
{
  this->blockSignals(true);

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();

  // link stuff on the general tab
  this->Internal->Links.registerLink(this->Internal->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("Background"));

  new pqStandardColorLinkAdaptor(this->Internal->backgroundColor,
    proxy, "Background");
  
  this->Internal->Links.registerLink(this->Internal->parallelProjection, "checked",
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("CameraParallelProjection"));
  
  
  // link default light params
  this->Internal->LightsLinks.registerLink(this->Internal->DefaultLightSwitch, "checked", 
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("LightSwitch"));
  pqSignalAdaptorSliderRange* sliderAdaptor;
  sliderAdaptor = new pqSignalAdaptorSliderRange(this->Internal->LightIntensity);
  this->Internal->LightsLinks.registerLink(sliderAdaptor, "value",
    SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("LightIntensity"));
  this->Internal->LightsLinks.registerLink(this->Internal->LightIntensity_Edit, "text",
    SIGNAL(textChanged(const QString&)),
    proxy, proxy->GetProperty("LightIntensity"));
  pqSignalAdaptorColor* lightColorAdaptor;
  lightColorAdaptor = new pqSignalAdaptorColor(this->Internal->SetLightColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);
  this->Internal->LightsLinks.registerLink(lightColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("LightDiffuseColor"));


  // link light kit params
  pqNamedWidgets::link(this->Internal->UseLight, proxy, &this->Internal->LightsLinks);

  // Need to explicitly link all the wacky-named properties
  this->Internal->LightsLinks.registerLink(this->Internal->FillLightK_F_Ratio, "value",
    SIGNAL(valueChanged(double)), proxy, proxy->GetProperty("FillLightK:F Ratio"));
  this->Internal->LightsLinks.registerLink(this->Internal->BackLightK_B_Ratio, "value",
    SIGNAL(valueChanged(double)), proxy, proxy->GetProperty("BackLightK:B Ratio"));
  this->Internal->LightsLinks.registerLink(this->Internal->HeadLightK_H_Ratio, "value",
    SIGNAL(valueChanged(double)), proxy, proxy->GetProperty("HeadLightK:H Ratio"));

  this->Internal->LightsLinks.registerLink(this->Internal->UseLight, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseLight"));

  this->resetAnnotation();
  
  this->blockSignals(false);

}

//-----------------------------------------------------------------------------
void pqRenderViewOptions::resetAnnotation()
{
  this->Internal->OrientationAxes->setChecked(this->Internal->RenderView->getOrientationAxesVisibility());
  this->Internal->OrientationAxesInteraction->setCheckState(
    this->Internal->RenderView->getOrientationAxesInteractivity()? Qt::Checked : Qt::Unchecked);
  this->Internal->OrientationAxesOutlineColor->setChosenColor(
    this->Internal->RenderView->getOrientationAxesOutlineColor());
  this->Internal->OrientationAxesLabelColor->setChosenColor(
    this->Internal->RenderView->getOrientationAxesLabelColor());

  this->Internal->CenterAxesVisibility->setCheckState(
    this->Internal->RenderView->getCenterAxesVisibility()? Qt::Checked : Qt::Unchecked);
}

//-----------------------------------------------------------------------------
void pqRenderViewOptions::disconnectGUI()
{
  this->Internal->Links.removeAllLinks();
  this->Internal->LightsLinks.removeAllLinks();
}

//-----------------------------------------------------------------------------
void pqRenderViewOptions::restoreDefaultBackground()
{
  if (this->Internal->RenderView)
    {
    const int* col = this->Internal->RenderView->defaultBackgroundColor();
    this->Internal->backgroundColor->setChosenColor(
               QColor(col[0], col[1], col[2]));
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewOptions::resetLights()
{
  if(this->Internal->RenderView)
    {
    // TODO: this doesn't let the user cancel to get previous lights back
    this->Internal->RenderView->restoreDefaultLightSettings();
    this->Internal->LightsLinks.reject();
    emit this->changesAvailable();
    }
}

