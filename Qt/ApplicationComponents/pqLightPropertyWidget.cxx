/*=========================================================================

   Program: ParaView
   Module:  pqLightPropertyWidget.cxx

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
#include "pqLightPropertyWidget.h"
#include "ui_pqLightPropertyWidget.h"

#include "pqColorChooserButton.h"
#include "pqComboBoxDomain.h"
#include "pqSignalAdaptors.h"
#include "vtkPVLight.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

class pqLightPropertyWidget::pqInternals : public Ui::LightPropertyWidget
{
public:
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
};

//-----------------------------------------------------------------------------
pqLightPropertyWidget::pqLightPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "LightWidgetRepresentation", smproxy, smgroup, parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  Ui::LightPropertyWidget& ui = *this->Internals;

  if (vtkSMProperty* positional = smgroup->GetProperty("Positional"))
  {
    vtkSMEnumerationDomain* enumDomain = positional->FindDomain<vtkSMEnumerationDomain>();
    if (enumDomain)
    {
      new pqComboBoxDomain(ui.positionalComboBox, positional, enumDomain);
      pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(ui.positionalComboBox);
      this->addPropertyLink(
        adaptor, "currentData", SIGNAL(currentTextChanged(QString)), positional);
    }
    else
    {
      qCritical("Missing required enumeration domain for property 'Positional'");
    }
  }
  else
  {
    qCritical("Missing required property for function 'Positional'");
  }

  if (vtkSMProperty* worldPosition = smgroup->GetProperty("WorldPosition"))
  {
    this->addPropertyLink(
      ui.worldPositionX, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 0);
    this->addPropertyLink(
      ui.worldPositionY, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 1);
    this->addPropertyLink(
      ui.worldPositionZ, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 2);
  }
  else
  {
    qCritical("Missing required property for function 'WorldPosition'");
  }

  if (vtkSMProperty* focalPoint = smgroup->GetProperty("FocalPoint"))
  {
    this->addPropertyLink(
      ui.focalPointX, "text2", SIGNAL(textChangedAndEditingFinished()), focalPoint, 0);
    this->addPropertyLink(
      ui.focalPointY, "text2", SIGNAL(textChangedAndEditingFinished()), focalPoint, 1);
    this->addPropertyLink(
      ui.focalPointZ, "text2", SIGNAL(textChangedAndEditingFinished()), focalPoint, 2);
  }
  else
  {
    qCritical("Missing required property for function 'FocalPoint'");
  }

  if (vtkSMDoubleVectorProperty* coneAngle =
        vtkSMDoubleVectorProperty::SafeDownCast(smgroup->GetProperty("ConeAngle")))
  {
    vtkSMDoubleRangeDomain* range = coneAngle->FindDomain<vtkSMDoubleRangeDomain>();
    if (coneAngle->GetNumberOfElements() == 1 && range->GetMinimumExists(0) &&
      range->GetMaximumExists(0))
    {
      ui.coneAngle->setMinimum(range->GetMinimum(0));
      ui.coneAngle->setMaximum(range->GetMaximum(0));
      if (range->GetResolutionExists())
      {
        ui.coneAngle->setResolution(range->GetResolution());
      }

      this->addPropertyLink(ui.coneAngle, "value", SIGNAL(valueEdited(double)), coneAngle, 0);
    }
    else
    {
      qCritical("Missing required double range domain with min and max for property 'ConeAngle'");
    }
  }
  else
  {
    qCritical("Missing required property for function 'ConeAngle'");
  }

  if (vtkSMDoubleVectorProperty* lightColor =
        vtkSMDoubleVectorProperty::SafeDownCast(smgroup->GetProperty("LightColor")))
  {
    if (lightColor->GetNumberOfElements() == 3)
    {
      ui.colorButton->setText(lightColor->GetXMLLabel());
      ui.colorButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      ui.colorButton->setShowAlphaChannel(false);
      this->addPropertyLink(
        ui.colorButton, "chosenColorRgbF", SIGNAL(chosenColorChanged(const QColor&)), lightColor);
    }
    else
    {
      qCritical("Incorect number of element property for function 'LightColor'");
    }
  }
  else
  {
    qCritical("Missing required property for function 'LightColor'");
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  // Connect to other properties to emulate decorators
  this->Internals->VTKConnector->Connect(smproxy->GetProperty("LightSwitch"),
    vtkCommand::ModifiedEvent, this, SLOT(updateVisibilityState()));
  this->Internals->VTKConnector->Connect(smproxy->GetProperty("LightType"),
    vtkCommand::ModifiedEvent, this, SLOT(updateVisibilityState()));
  this->Internals->VTKConnector->Connect(smproxy->GetProperty("Positional"),
    vtkCommand::ModifiedEvent, this, SLOT(updateVisibilityState()));
  this->updateVisibilityState();
}

//-----------------------------------------------------------------------------
pqLightPropertyWidget::~pqLightPropertyWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqLightPropertyWidget::updateVisibilityState()
{
  vtkSMProxy* proxy = this->proxy();
  bool showPositions = false;
  bool showAngle = false;
  bool showLightWidget = false;

  // Check LightSwitch, LightType and Positional to position the different boolean flags
  vtkSMProperty* lightSwitch = proxy->GetProperty("LightSwitch");
  if (lightSwitch)
  {
    showLightWidget = vtkSMPropertyHelper(lightSwitch).GetAsInt() == 1;
  }
  else
  {
    qWarning("Missing required property \"LightSwitch\" for the visibility state");
  }

  vtkSMProperty* lightType = proxy->GetProperty("LightType");
  if (lightType)
  {
    int lightTypeVal = vtkSMPropertyHelper(lightType).GetAsInt();
    showPositions =
      lightTypeVal == VTK_LIGHT_TYPE_CAMERA_LIGHT || lightTypeVal == VTK_LIGHT_TYPE_SCENE_LIGHT;
    showAngle = lightTypeVal != VTK_LIGHT_TYPE_AMBIENT_LIGHT;
    showLightWidget &= lightTypeVal == VTK_LIGHT_TYPE_SCENE_LIGHT;
  }
  else
  {
    qWarning("Missing required property \"LightType\" for the visibility state");
  }

  vtkSMProperty* positional = proxy->GetProperty("Positional");
  if (positional)
  {
    showAngle &= vtkSMPropertyHelper(positional).GetAsInt() == 1;
  }
  else
  {
    showAngle = false;
    qWarning("Missing required property \"Positional\" for the visibility state");
  }

  // show/hide positions related widget
  Ui::LightPropertyWidget& ui = *this->Internals;
  ui.lightPositionLabel->setVisible(showPositions);
  ui.worldPositionX->setVisible(showPositions);
  ui.worldPositionY->setVisible(showPositions);
  ui.worldPositionZ->setVisible(showPositions);
  ui.focalPointLabel->setVisible(showPositions);
  ui.focalPointX->setVisible(showPositions);
  ui.focalPointY->setVisible(showPositions);
  ui.focalPointZ->setVisible(showPositions);

  // show/hide cone angle related widgets
  vtkSMPropertyHelper(this->widgetProxy(), "Positional").Set(showAngle);
  this->widgetProxy()->UpdateVTKObjects();
  ui.coneAngleLabel->setVisible(showAngle);
  ui.coneAngle->setVisible(showAngle);

  // show/hide the light widget
  this->setWidgetVisible(ui.show3DWidget->isChecked() && showLightWidget);
  ui.show3DWidget->setVisible(showLightWidget);
}
