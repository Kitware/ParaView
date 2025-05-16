// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHandlePropertyWidget.h"
#include "ui_pqHandlePropertyWidget.h"

#include "pqOutputPort.h"
#include "pqPVApplicationCore.h"
#include "pqPointPickingHelper.h"
#include "pqSelectionManager.h"
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
    ui.pointLabel->setText(
      QCoreApplication::translate("ServerManagerXML", worldPosition->GetXMLLabel()));
    ui.pickLabel->setText(ui.pickLabel->text().arg(
      QCoreApplication::translate("ServerManagerXML", worldPosition->GetXMLLabel())));
  }
  else
  {
    qCritical("Missing required property for function 'WorldPosition'");
    ui.pickLabel->setText(ui.pickLabel->text().arg(tr("Point")));
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

  this->UseSelectionCenterButton = ui.useSelectionCenter;
  this->connect(pqPVApplicationCore::instance()->selectionManager(),
    SIGNAL(selectionChanged(pqOutputPort*)), SLOT(selectionChanged()));
  this->selectionChanged();
  QObject::connect(ui.useSelectionCenter, &QPushButton::clicked,
    [this]()
    {
      auto selMgr = pqPVApplicationCore::instance()->selectionManager();
      auto bbox = selMgr->selectedDataBounds();
      if (bbox.IsValid())
      {
        double center[3];
        bbox.GetCenter(center);
        this->setWorldPosition(center[0], center[1], center[2]);
      }
    });

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelper, SIGNAL(pick(double, double, double)),
    SLOT(setWorldPosition(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setWorldPosition(double, double, double)));
}

//-----------------------------------------------------------------------------
pqHandlePropertyWidget::~pqHandlePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqHandlePropertyWidget::selectionChanged()
{
  if (this->UseSelectionCenterButton)
  {
    auto selMgr = pqPVApplicationCore::instance()->selectionManager();
    this->UseSelectionCenterButton->setEnabled(selMgr->hasActiveSelection());
  }
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
  Q_EMIT this->changeAvailable();
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
