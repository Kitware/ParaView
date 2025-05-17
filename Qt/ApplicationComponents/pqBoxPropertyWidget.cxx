// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoxPropertyWidget.h"
#include "ui_pqBoxPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqUndoStack.h"

#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
class pqBoxPropertyWidget::pqUi : public Ui::BoxPropertyWidget
{
};

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::pqBoxPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject, bool hideReferenceBounds)
  : Superclass("representations", "BoxWidgetRepresentation", smproxy, smgroup, parentObject)
  , Ui(new pqBoxPropertyWidget::pqUi())
  , BoxIsRelativeToInput(false)
{
  this->Ui->setupUi(this);

  vtkSMProxy* wdgProxy = this->widgetProxy();

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  this->WidgetLinks.addPropertyLink(this->Ui->enableTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("TranslationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableScaling, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("ScalingEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableRotation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("RotationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Ui->enableMoveFaces, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("MoveFacesEnabled"));

  if (vtkSMProperty* position = smgroup->GetProperty("Position"))
  {
    this->addPropertyLink(
      this->Ui->translateX, "text2", SIGNAL(textChangedAndEditingFinished()), position, 0);
    this->addPropertyLink(
      this->Ui->translateY, "text2", SIGNAL(textChangedAndEditingFinished()), position, 1);
    this->addPropertyLink(
      this->Ui->translateZ, "text2", SIGNAL(textChangedAndEditingFinished()), position, 2);
    this->Ui->labelTranslate->setText(
      QCoreApplication::translate("ServerManagerXML", position->GetXMLLabel()));
    QString tooltip = this->getTooltip(position);
    this->Ui->translateX->setToolTip(tooltip);
    this->Ui->translateY->setToolTip(tooltip);
    this->Ui->translateZ->setToolTip(tooltip);
    this->Ui->labelTranslate->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelTranslate->hide();
    this->Ui->translateX->hide();
    this->Ui->translateY->hide();
    this->Ui->translateZ->hide();

    // see WidgetLinks above.
    this->Ui->enableTranslation->setChecked(false);
    this->Ui->enableTranslation->hide();
  }

  if (vtkSMProperty* rotation = smgroup->GetProperty("Rotation"))
  {
    this->addPropertyLink(
      this->Ui->rotateX, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 0);
    this->addPropertyLink(
      this->Ui->rotateY, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 1);
    this->addPropertyLink(
      this->Ui->rotateZ, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 2);
    this->Ui->labelRotate->setText(
      QCoreApplication::translate("ServerManagerXML", rotation->GetXMLLabel()));
    QString tooltip = this->getTooltip(rotation);
    this->Ui->rotateX->setToolTip(tooltip);
    this->Ui->rotateY->setToolTip(tooltip);
    this->Ui->rotateZ->setToolTip(tooltip);
    this->Ui->labelRotate->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelRotate->hide();
    this->Ui->rotateX->hide();
    this->Ui->rotateY->hide();
    this->Ui->rotateZ->hide();

    // see WidgetLinks above.
    this->Ui->enableRotation->setChecked(false);
    this->Ui->enableRotation->hide();
  }

  if (vtkSMProperty* scale = smgroup->GetProperty("Scale"))
  {
    this->addPropertyLink(
      this->Ui->scaleX, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 0);
    this->addPropertyLink(
      this->Ui->scaleY, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 1);
    this->addPropertyLink(
      this->Ui->scaleZ, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 2);
    this->Ui->labelScale->setText(
      QCoreApplication::translate("ServerManagerXML", scale->GetXMLLabel()));
    QString tooltip = this->getTooltip(scale);
    this->Ui->scaleX->setToolTip(tooltip);
    this->Ui->scaleY->setToolTip(tooltip);
    this->Ui->scaleZ->setToolTip(tooltip);
    this->Ui->labelScale->setToolTip(tooltip);
  }
  else
  {
    this->Ui->labelScale->hide();
    this->Ui->scaleX->hide();
    this->Ui->scaleY->hide();
    this->Ui->scaleZ->hide();

    // see WidgetLinks above.
    this->Ui->enableScaling->setChecked(false);
    this->Ui->enableScaling->hide();
    this->Ui->enableMoveFaces->setChecked(false);
    this->Ui->enableMoveFaces->hide();
  }

  auto useRefBounds = smgroup->GetProperty("UseReferenceBounds");
  auto refBounds = smgroup->GetProperty("ReferenceBounds");
  if (useRefBounds && refBounds)
  {
    this->addPropertyLink(
      this->Ui->useReferenceBounds, "checked", SIGNAL(toggled(bool)), useRefBounds);
    this->addPropertyLink(
      this->Ui->xmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 0);
    this->addPropertyLink(
      this->Ui->xmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 1);
    this->addPropertyLink(
      this->Ui->ymin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 2);
    this->addPropertyLink(
      this->Ui->ymax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 3);
    this->addPropertyLink(
      this->Ui->zmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 4);
    this->addPropertyLink(
      this->Ui->zmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 5);
  }
  else
  {
    this->Ui->referenceBoundsLabel->hide();
    this->Ui->referenceBoundsHLine->hide();
    this->Ui->useReferenceBounds->hide();
    this->Ui->xmin->hide();
    this->Ui->xmax->hide();
    this->Ui->ymin->hide();
    this->Ui->ymax->hide();
    this->Ui->zmin->hide();
    this->Ui->zmax->hide();

    // if `ReferenceBounds` or `UseReferenceBounds` is not present,
    // the box is either using the input bounds as reference (e.g. Transform) or
    // isn't i.e relative to unit box.  To determine that, let's if "Input" is part
    // of the property group.
    this->BoxIsRelativeToInput = (smgroup->GetProperty("Input") != nullptr);
    vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").Set(0);
    wdgProxy->UpdateVTKObjects();
  }

  if (hideReferenceBounds)
  {
    this->Ui->referenceBoundsLabel->hide();
    this->Ui->referenceBoundsHLine->hide();
    this->Ui->useReferenceBounds->hide();
    this->Ui->xmin->hide();
    this->Ui->xmax->hide();
    this->Ui->ymin->hide();
    this->Ui->ymax->hide();
    this->Ui->zmin->hide();
    this->Ui->zmax->hide();
  }

  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(this->Ui->show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  this->Ui->show3DWidget->connect(
    this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(this->Ui->show3DWidget->isChecked());

  QObject::connect(this->Ui->resetBounds, &QAbstractButton::clicked,
    [this, wdgProxy](bool)
    {
      auto bbox = this->dataBounds(this->Ui->visibleBoundsOnly->isChecked());
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
      if (this->PlaceWidgetConnection)
      {
        QObject::disconnect(this->PlaceWidgetConnection);
      }
      this->PlaceWidgetConnection = QObject::connect(&pqActiveObjects::instance(),
        &pqActiveObjects::dataUpdated, this, &pqBoxPropertyWidget::placeWidget);
      this->render();
    });
}

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::~pqBoxPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::placeWidget()
{
  auto wdgProxy = this->widgetProxy();
  if (this->BoxIsRelativeToInput || vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").GetAsInt())
  {
    auto bbox = this->dataBounds(this->Ui->visibleBoundsOnly->isChecked());
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
