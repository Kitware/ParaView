/*=========================================================================

   Program: ParaView
   Module:  pqBoxPropertyWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqBoxPropertyWidget.h"
#include "ui_pqBoxPropertyWidget.h"

#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

#include <QDoubleValidator>

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::pqBoxPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
    "representations", "BoxWidgetRepresentation",
    smproxy, smgroup, parentObject)
{
  Ui::BoxPropertyWidget ui;
  ui.setupUi(this);
  new QDoubleValidator(ui.translateX);
  new QDoubleValidator(ui.translateY);
  new QDoubleValidator(ui.translateZ);
  new QDoubleValidator(ui.rotateX);
  new QDoubleValidator(ui.rotateY);
  new QDoubleValidator(ui.rotateZ);
  new QDoubleValidator(ui.scaleX);
  new QDoubleValidator(ui.scaleY);
  new QDoubleValidator(ui.scaleZ);

  if (vtkSMProperty* position = smgroup->GetProperty("Position"))
    {
    this->addPropertyLink(
      ui.translateX, "text2", SIGNAL(textChangedAndEditingFinished()), position, 0);
    this->addPropertyLink(
      ui.translateY, "text2", SIGNAL(textChangedAndEditingFinished()), position, 1);
    this->addPropertyLink(
      ui.translateZ, "text2", SIGNAL(textChangedAndEditingFinished()), position, 2);
    ui.labelTranslate->setText(position->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for 'Position'.");
    }

  if (vtkSMProperty* rotation = smgroup->GetProperty("Rotation"))
    {
    this->addPropertyLink(
      ui.rotateX, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 0);
    this->addPropertyLink(
      ui.rotateY, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 1);
    this->addPropertyLink(
      ui.rotateZ, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 2);
    ui.labelRotate->setText(rotation->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for 'Rotation'.");
    }

  if (vtkSMProperty* scale = smgroup->GetProperty("Scale"))
    {
    this->addPropertyLink(
      ui.scaleX, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 0);
    this->addPropertyLink(
      ui.scaleY, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 1);
    this->addPropertyLink(
      ui.scaleZ, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 2);
    ui.labelScale->setText(scale->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for 'Scale'.");
    }

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  vtkSMProxy* wdgProxy = this->widgetProxy();
  this->WidgetLinks.addPropertyLink(ui.enableTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("TranslationEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableScaling, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("ScalingEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableRotation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("RotationEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableMoveFaces, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("MoveFacesEnabled"));
  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());
}

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::~pqBoxPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
    {
    return;
    }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();

  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();

  // This is incorrect. We should never be changing properties on the proxy like
  // this. The way we are letting users set the box without explicitly setting
  // the bounds is wrong. We'll have revisit that. For now, I am letting this
  // be. Please don't follow this pattern in your code.
  vtkSMPropertyHelper(this->proxy(), "Bounds", /*quiet*/true).Set(bds, 6);
  this->proxy()->UpdateVTKObjects();
}
