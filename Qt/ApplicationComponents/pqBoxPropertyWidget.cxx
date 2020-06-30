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

#include <QKeyEvent>
#include <QShortcut>

#define TRANSLATE_INCR (0.01)

enum DirectionOfTranslation
{
  TRANSLATE_X = 0,
  TRANSLATE_Y,
  TRANSLATE_Z,
};

struct pqBoxPropertyWidget::pqInternals
{
  bool isInteractive = false;
  bool isMovable = false;
  DirectionOfTranslation translationAxis;
  double currentXPos = 0;
  Ui::BoxPropertyWidget ui;
};

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::pqBoxPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject, bool hideReferenceBounds)
  : Superclass("representations", "BoxWidgetRepresentation", smproxy, smgroup, parentObject)
  , BoxIsRelativeToInput(false)
  , Internals(new pqBoxPropertyWidget::pqInternals())
{
  this->Internals->ui.setupUi(this);

  vtkSMProxy* wdgProxy = this->widgetProxy();

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  this->WidgetLinks.addPropertyLink(this->Internals->ui.enableTranslation, "checked",
    SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("TranslationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Internals->ui.enableScaling, "checked",
    SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("ScalingEnabled"));
  this->WidgetLinks.addPropertyLink(this->Internals->ui.enableRotation, "checked",
    SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("RotationEnabled"));
  this->WidgetLinks.addPropertyLink(this->Internals->ui.enableMoveFaces, "checked",
    SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("MoveFacesEnabled"));

  if (vtkSMProperty* position = smgroup->GetProperty("Position"))
  {
    this->addPropertyLink(this->Internals->ui.translateX, "text2",
      SIGNAL(textChangedAndEditingFinished()), position, 0);
    this->addPropertyLink(this->Internals->ui.translateY, "text2",
      SIGNAL(textChangedAndEditingFinished()), position, 1);
    this->addPropertyLink(this->Internals->ui.translateZ, "text2",
      SIGNAL(textChangedAndEditingFinished()), position, 2);
    this->Internals->ui.labelTranslate->setText(position->GetXMLLabel());
    QString tooltip = this->getTooltip(position);
    this->Internals->ui.translateX->setToolTip(tooltip);
    this->Internals->ui.translateY->setToolTip(tooltip);
    this->Internals->ui.translateZ->setToolTip(tooltip);
    this->Internals->ui.labelTranslate->setToolTip(tooltip);
  }
  else
  {
    this->Internals->ui.labelTranslate->hide();
    this->Internals->ui.translateX->hide();
    this->Internals->ui.translateY->hide();
    this->Internals->ui.translateZ->hide();

    // see WidgetLinks above.
    this->Internals->ui.enableTranslation->setChecked(false);
    this->Internals->ui.enableTranslation->hide();
  }

  if (vtkSMProperty* rotation = smgroup->GetProperty("Rotation"))
  {
    this->addPropertyLink(
      this->Internals->ui.rotateX, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 0);
    this->addPropertyLink(
      this->Internals->ui.rotateY, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 1);
    this->addPropertyLink(
      this->Internals->ui.rotateZ, "text2", SIGNAL(textChangedAndEditingFinished()), rotation, 2);
    this->Internals->ui.labelRotate->setText(rotation->GetXMLLabel());
    QString tooltip = this->getTooltip(rotation);
    this->Internals->ui.rotateX->setToolTip(tooltip);
    this->Internals->ui.rotateY->setToolTip(tooltip);
    this->Internals->ui.rotateZ->setToolTip(tooltip);
    this->Internals->ui.labelRotate->setToolTip(tooltip);
  }
  else
  {
    this->Internals->ui.labelRotate->hide();
    this->Internals->ui.rotateX->hide();
    this->Internals->ui.rotateY->hide();
    this->Internals->ui.rotateZ->hide();

    // see WidgetLinks above.
    this->Internals->ui.enableRotation->setChecked(false);
    this->Internals->ui.enableRotation->hide();
  }

  if (vtkSMProperty* scale = smgroup->GetProperty("Scale"))
  {
    this->addPropertyLink(
      this->Internals->ui.scaleX, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 0);
    this->addPropertyLink(
      this->Internals->ui.scaleY, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 1);
    this->addPropertyLink(
      this->Internals->ui.scaleZ, "text2", SIGNAL(textChangedAndEditingFinished()), scale, 2);
    this->Internals->ui.labelScale->setText(scale->GetXMLLabel());
    QString tooltip = this->getTooltip(scale);
    this->Internals->ui.scaleX->setToolTip(tooltip);
    this->Internals->ui.scaleY->setToolTip(tooltip);
    this->Internals->ui.scaleZ->setToolTip(tooltip);
    this->Internals->ui.labelScale->setToolTip(tooltip);
  }
  else
  {
    this->Internals->ui.labelScale->hide();
    this->Internals->ui.scaleX->hide();
    this->Internals->ui.scaleY->hide();
    this->Internals->ui.scaleZ->hide();

    // see WidgetLinks above.
    this->Internals->ui.enableScaling->setChecked(false);
    this->Internals->ui.enableScaling->hide();
    this->Internals->ui.enableMoveFaces->setChecked(false);
    this->Internals->ui.enableMoveFaces->hide();
  }

  auto useRefBounds = smgroup->GetProperty("UseReferenceBounds");
  auto refBounds = smgroup->GetProperty("ReferenceBounds");
  if (useRefBounds && refBounds)
  {
    this->addPropertyLink(
      this->Internals->ui.useReferenceBounds, "checked", SIGNAL(toggled(bool)), useRefBounds);
    this->addPropertyLink(
      this->Internals->ui.xmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 0);
    this->addPropertyLink(
      this->Internals->ui.xmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 1);
    this->addPropertyLink(
      this->Internals->ui.ymin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 2);
    this->addPropertyLink(
      this->Internals->ui.ymax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 3);
    this->addPropertyLink(
      this->Internals->ui.zmin, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 4);
    this->addPropertyLink(
      this->Internals->ui.zmax, "text2", SIGNAL(textChangedAndEditingFinished()), refBounds, 5);
  }
  else
  {
    this->Internals->ui.referenceBoundsLabel->hide();
    this->Internals->ui.referenceBoundsHLine->hide();
    this->Internals->ui.useReferenceBounds->hide();
    this->Internals->ui.xmin->hide();
    this->Internals->ui.xmax->hide();
    this->Internals->ui.ymin->hide();
    this->Internals->ui.ymax->hide();
    this->Internals->ui.zmin->hide();
    this->Internals->ui.zmax->hide();

    // if `ReferenceBounds` or `UseReferenceBounds` is not present, which is the
    // case for `Transform`, the box is providing params relative to the input
    // bounds.
    vtkSMPropertyHelper(wdgProxy, "UseReferenceBounds").Set(0);
    wdgProxy->UpdateVTKObjects();

    this->BoxIsRelativeToInput = true;
  }

  if (hideReferenceBounds)
  {
    this->Internals->ui.referenceBoundsLabel->hide();
    this->Internals->ui.referenceBoundsHLine->hide();
    this->Internals->ui.useReferenceBounds->hide();
    this->Internals->ui.xmin->hide();
    this->Internals->ui.xmax->hide();
    this->Internals->ui.ymin->hide();
    this->Internals->ui.ymax->hide();
    this->Internals->ui.zmin->hide();
    this->Internals->ui.zmax->hide();
  }

  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(
    this->Internals->ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  this->Internals->ui.show3DWidget->connect(
    this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(this->Internals->ui.show3DWidget->isChecked());

  QObject::connect(
    this->Internals->ui.resetBounds, &QAbstractButton::clicked, [this, wdgProxy](bool) {
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

  this->connect(this->Internals->ui.enableInteractive, SIGNAL(toggled(bool)), this,
    SLOT(onEnableInteractive(bool)));
}

//-----------------------------------------------------------------------------
pqBoxPropertyWidget::~pqBoxPropertyWidget()
{
  QWidget::releaseKeyboard();
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

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::translate(double incr)
{
  QLineEdit* line = nullptr;

  switch (static_cast<DirectionOfTranslation>(this->Internals->translationAxis))
  {
    case TRANSLATE_X:
      line = this->Internals->ui.translateX;
      break;
    case TRANSLATE_Y:
      line = this->Internals->ui.translateY;
      break;
    case TRANSLATE_Z:
      line = this->Internals->ui.translateZ;
      break;
    default:
      return;
  }

  const double value = line->text().toDouble();
  const double newValue = value + (incr * TRANSLATE_INCR);

  line->setText(std::to_string(newValue).c_str());

  Q_EMIT this->changeAvailable();
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::keyPressEvent(QKeyEvent* event)
{
  bool relevantKeyPressed = false;

  if (event->modifiers() == Qt::ShiftModifier)
  {
    switch (event->key())
    {
      case Qt::Key_X:
        relevantKeyPressed = true;
        this->Internals->translationAxis = TRANSLATE_X;
        break;

      case Qt::Key_Y:
        relevantKeyPressed = true;
        this->Internals->translationAxis = TRANSLATE_Y;
        break;

      case Qt::Key_Z:
        relevantKeyPressed = true;
        this->Internals->translationAxis = TRANSLATE_Z;
        break;
    }
  }

  if (relevantKeyPressed)
  {
    QWidget::setMouseTracking(true);
    QWidget::grabMouse();
    this->Internals->isMovable = true;
  }

  Superclass::keyPressEvent(event);
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::keyReleaseEvent(QKeyEvent* event)
{
  switch (event->key())
  {
    case Qt::Key_X:
    case Qt::Key_Y:
    case Qt::Key_Z:
      QWidget::releaseMouse();
      QWidget::setMouseTracking(false);
      this->Internals->currentXPos = 0;
      this->Internals->isMovable = false;
  }

  Superclass::keyReleaseEvent(event);
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (this->Internals->isMovable && this->Internals->isInteractive)
  {
    auto xPos = event->globalX();

    if (this->Internals->currentXPos != 0)
    {
      translate(xPos - this->Internals->currentXPos);
    }

    this->Internals->currentXPos = xPos;
  }

  Superclass::mouseMoveEvent(event);
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::focusOutEvent(QFocusEvent* event)
{
  QWidget::releaseMouse();
  QWidget::setMouseTracking(false);
  Superclass::focusOutEvent(event);
}

//-----------------------------------------------------------------------------
void pqBoxPropertyWidget::onEnableInteractive(bool isEnabled)
{
  this->Internals->isInteractive = isEnabled;
}
