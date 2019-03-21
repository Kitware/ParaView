/*=========================================================================

   Program: ParaView
   Module:  pqHandlePropertyWidget.cxx

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
#include "pqHandlePropertyWidget.h"
#include "ui_pqHandlePropertyWidget.h"

#include "pqPointPickingHelper.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

//-----------------------------------------------------------------------------
pqHandlePropertyWidget::pqHandlePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "HandleWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::HandlePropertyWidget ui;
  ui.setupUi(this);

  if (vtkSMProperty* worldPosition = smgroup->GetProperty("WorldPosition"))
  {
    this->addPropertyLink(
      ui.worldPositionX, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 0);
    this->addPropertyLink(
      ui.worldPositionY, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 1);
    this->addPropertyLink(
      ui.worldPositionZ, "text2", SIGNAL(textChangedAndEditingFinished()), worldPosition, 2);
    ui.pointLabel->setText(worldPosition->GetXMLLabel());
    ui.pickLabel->setText(
      ui.pickLabel->text().replace("'Point'", QString("'%1'").arg(worldPosition->GetXMLLabel())));
  }
  else
  {
    qCritical("Missing required property for function 'WorldPosition'");
  }

  if (smgroup->GetProperty("Input"))
  {
    this->connect(ui.useCenterBounds, SIGNAL(clicked()), SLOT(centerOnBounds()));
  }
  else
  {
    // We have not data to center the point on.
    ui.useCenterBounds->hide();
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(pickHelper, SIGNAL(pick(double, double, double)),
    SLOT(setWorldPosition(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper2->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(pickHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setWorldPosition(double, double, double)));
}

//-----------------------------------------------------------------------------
pqHandlePropertyWidget::~pqHandlePropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqHandlePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqHandlePropertyWidget::setWorldPosition(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double o[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "WorldPosition").Set(o, 3);
  wdgProxy->UpdateVTKObjects();
  emit this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqHandlePropertyWidget::centerOnBounds()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (bbox.IsValid())
  {
    double origin[3];
    bbox.GetCenter(origin);
    this->setWorldPosition(origin[0], origin[1], origin[2]);
  }
}
