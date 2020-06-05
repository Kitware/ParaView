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

#include "pqUndoStack.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::pqBoxPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject, bool hideReferenceBounds)
  : Superclass("representations", "BoxWidgetRepresentation", smproxy, smgroup, parentObject)
  , BoxIsRelativeToInput(false)
{
  Ui::BoxPropertyWidget ui;
  ui.setupUi(this);

  vtkSMProxy* wdgProxy = this->widgetProxy();

  ui.translateX->setToolTip("hi");
  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  this->WidgetLinks.addPropertyLink(ui.enableTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("TranslationEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableScaling, "checked", SIGNAL(toggled(bool)), wdgProxy,
    wdgProxy->GetProperty("ScalingEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableRotation, "checked", SIGNAL(toggled(bool)), wdgProxy,
    wdgProxy->GetProperty("RotationEnabled"));
  this->WidgetLinks.addPropertyLink(ui.enableMoveFaces, "checked", SIGNAL(toggled(bool)), wdgProxy,
    wdgProxy->GetProperty("MoveFacesEnabled"));

  if (vtkSMProperty* position = smgroup->GetProperty("Position"))
  {
    this->addPropertyLink(
      ui.translateX, "text2", SIGNAL(textChangedAndEditingFinished()), position, 0);
    this->addPropertyLink(
      ui.translateY, "text2", SIGNAL(textChangedAndEditingFinished()), position, 1);
    this->addPropertyLink(
      ui.translateZ, "text2", SIGNAL(textChangedAndEditingFinished()), position, 2);
    ui.labelTranslate->setText(position->GetXMLLabel());
    QString tooltip = this->getTooltip(position);
    ui.translateX->setToolTip(tooltip);
    ui.translateY->setToolTip(tooltip);
    ui.translateZ->setToolTip(tooltip);
    ui.labelTranslate->setToolTip(tooltip);
  }
  else
  {
    ui.labelTranslate->hide();
    ui.translateX->hide();
    ui.translateY->hide();
    ui.translateZ->hide();

    // see WidgetLinks above.
    ui.enableTranslation->setChecked(false);
    ui.enableTranslation->hide();
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
    QString tooltip = this->getTooltip(rotation);
    ui.rotateX->setToolTip(tooltip);
    ui.rotateY->setToolTip(tooltip);
    ui.rotateZ->setToolTip(tooltip);
    ui.labelRotate->setToolTip(tooltip);
  }
  else
  {
    ui.labelRotate->hide();
    ui.rotateX->hide();
    ui.rotateY->hide();
    ui.rotateZ->hide();

    // see WidgetLinks above.
    ui.enableRotation->setChecked(false);
    ui.enableRotation->hide();
  }

  if (vtkSMProperty* scale = smgroup->GetProperty("Scale"))
  {
    this->addPropertyLink(ui.scaleX, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 0);
    this->addPropertyLink(ui.scaleY, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 1);
    this->addPropertyLink(ui.scaleZ, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 2);
    ui.labelScale->setText(scale->GetXMLLabel());
    QString tooltip = this->getTooltip(scale);
    ui.scaleX->setToolTip(tooltip);
    ui.scaleY->setToolTip(tooltip);
    ui.scaleZ->setToolTip(tooltip);
    ui.labelScale->setToolTip(tooltip);
  }
  else
  {
    ui.labelScale->hide();
    ui.scaleX->hide();
    ui.scaleY->hide();
    ui.scaleZ->hide();

    // see WidgetLinks above.
    ui.enableScaling->setChecked(false);
    ui.enableScaling->hide();
    ui.enableMoveFaces->setChecked(false);
    ui.enableMoveFaces->hide();
  }

  auto useRefBounds = smgroup->GetProperty("UseReferenceBounds");
  auto refBounds = smgroup->GetProperty("ReferenceBounds");
  if (useRefBounds && refBounds)
  {
    this->addPropertyLink(ui.useReferenceBounds, "checked", SIGNAL(toggled(bool)), useRefBounds);
    this->addPropertyLink(ui.xmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 0);
    this->addPropertyLink(ui.xmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 1);
    this->addPropertyLink(ui.ymin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 2);
    this->addPropertyLink(ui.ymax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 3);
    this->addPropertyLink(ui.zmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 4);
    this->addPropertyLink(ui.zmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 5);
  }
  else
  {
    ui.referenceBoundsLabel->hide();
    ui.referenceBoundsHLine->hide();
    ui.useReferenceBounds->hide();
    ui.xmin->hide();
    ui.xmax->hide();
    ui.ymin->hide();
    ui.ymax->hide();
    ui.zmin->hide();
    ui.zmax->hide();

    // if `ReferenceBounds` or `UseReferenceBounds` is not present, which is the
    // case for `Transform`, the box is providing params relative to the input
    // bounds.
    vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").Set(0);
    wdgProxy->UpdateVTKObjects();

    this->BoxIsRelativeToInput = true;
  }

  if (hideReferenceBounds)
  {
    ui.referenceBoundsLabel->hide();
    ui.referenceBoundsHLine->hide();
    ui.useReferenceBounds->hide();
    ui.xmin->hide();
    ui.xmax->hide();
    ui.ymin->hide();
    ui.ymax->hide();
    ui.zmin->hide();
    ui.zmax->hide();
  }

  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  QObject::connect(ui.resetBounds, &QAbstractButton::clicked, [this, wdgProxy](bool) {
    auto bbox = this->dataBounds();
    if (!bbox.IsValid())
    {
      return;
    }
    if (this->BoxIsRelativeToInput ||
      vtkSMUncheckedPropertyHelper(wdgProxy, "UseReferenceBounds").GetAsInt() == 1)
    {
      double bds[6];
      bbox.GetBounds(bds);
      vtkSMPropertyHelper(wdgProxy, "ReferenceBounds").Set(bds, 6);

      const double scale[3] = { 1, 1, 1 };
      vtkSMPropertyHelper(wdgProxy, "Scale").Set(scale, 3);

      const double pos[3] = { 0, 0, 0 };
      vtkSMPropertyHelper(wdgProxy, "Position").Set(pos, 3);

      const double orient[3] = { 0, 0, 0 };
      vtkSMPropertyHelper(wdgProxy, "Rotation").Set(orient, 3);
    }
    else
    {
      double bds[6] = { 0, 1, 0, 1, 0, 1 };
      vtkSMPropertyHelper(wdgProxy, "ReferenceBounds").Set(bds, 6);

      double lengths[3];
      bbox.GetLengths(lengths);
      vtkSMPropertyHelper(wdgProxy, "Scale").Set(lengths, 3);

      const double orient[3] = { 0, 0, 0 };
      vtkSMPropertyHelper(wdgProxy, "Rotation").Set(orient, 3);

      vtkSMPropertyHelper(wdgProxy, "Position").Set(bbox.GetMinPoint(), 3);
    }
    wdgProxy->UpdateVTKObjects();
    Q_EMIT this->changeAvailable();
    this->placeWidget();
    this->render();
  });
}

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::~pqBoxPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::placeWidget()
{
  auto wdgProxy = this->widgetProxy();
  if (this->BoxIsRelativeToInput || vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").GetAsInt())
  {
    auto bbox = this->dataBounds();
    if (bbox.IsValid())
    {
      double bds[6];
      bbox.GetBounds(bds);

      vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
      wdgProxy->UpdateVTKObjects();
    }
  }
  else
  {
    double bds[6] = { 0, 1, 0, 1, 0, 1 };
    vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
    wdgProxy->UpdateVTKObjects();
  }
}
