// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorEditorPropertyWidget.h"
#include "ui_pqColorEditorPropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqChooseColorPresetReaction.h"
#include "pqDataRepresentation.h"
#include "pqEditColorMapReaction.h"
#include "pqEditScalarBarReaction.h"
#include "pqPropertiesPanel.h"
#include "pqRescaleScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqServerManagerModel.h"
#include "pqUseSeparateColorMapReaction.h"
#include "vtkSMPropertyHelper.h"

class pqColorEditorPropertyWidget::pqInternals
{
public:
  Ui::ColorEditorPropertyWidget Ui;
  QPointer<QAction> ScalarBarVisibilityAction;
  QPointer<QAction> UseSeparateColorMapAction;
  QPointer<QAction> EditScalarBarAction;
};

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::pqColorEditorPropertyWidget(vtkSMProxy* smProxy, QWidget* parentObject)
  : Superclass(smProxy, parentObject)
  , Internals(new pqColorEditorPropertyWidget::pqInternals())
{
  this->setShowLabel(true);

  Ui::ColorEditorPropertyWidget& Ui = this->Internals->Ui;
  Ui.setupUi(this);
  Ui.gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // Setup various widget properties.
  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* pqproxy = smm->findItem<pqProxy*>(smProxy);
  pqDataRepresentation* representation = qobject_cast<pqDataRepresentation*>(pqproxy);
  Ui.DisplayColorWidget->setRepresentation(representation);

  // show scalar bar button
  QAction* scalarBarAction = new QAction(this);
  this->Internals->ScalarBarVisibilityAction = scalarBarAction;
  scalarBarAction->connect(Ui.ShowScalarBar, SIGNAL(clicked()), SLOT(trigger()));
  Ui.ShowScalarBar->connect(scalarBarAction, SIGNAL(toggled(bool)), SLOT(setChecked(bool)));
  pqScalarBarVisibilityReaction* sbvr =
    new pqScalarBarVisibilityReaction(scalarBarAction, /*track_active_objects*/ false);
  sbvr->setRepresentation(representation);
  this->connect(scalarBarAction, SIGNAL(changed()), SLOT(updateEnableState()));

  // edit scalar bar.
  QAction* editScalarBarAction = new QAction(this);
  this->Internals->EditScalarBarAction = editScalarBarAction;
  editScalarBarAction->connect(Ui.EditScalarBar, SIGNAL(clicked()), SLOT(trigger()));
  pqEditScalarBarReaction* esbr = new pqEditScalarBarReaction(editScalarBarAction, false);
  esbr->setRepresentation(representation);
  this->connect(editScalarBarAction, SIGNAL(changed()), SLOT(updateEnableState()));

  // edit color map button
  QAction* editColorMapAction = new QAction(this);
  QObject::connect(Ui.EditColorMap, SIGNAL(clicked()), editColorMapAction, SLOT(trigger()));
  pqEditColorMapReaction* ecmr = new pqEditColorMapReaction(editColorMapAction, false);
  ecmr->setRepresentation(representation);

  // separate color map button
  QAction* useSeparateColorMapAction = new QAction(this);
  this->Internals->UseSeparateColorMapAction = useSeparateColorMapAction;
  useSeparateColorMapAction->connect(Ui.UseSeparateColorMap, SIGNAL(clicked()), SLOT(trigger()));
  Ui.UseSeparateColorMap->connect(
    useSeparateColorMapAction, SIGNAL(toggled(bool)), SLOT(setChecked(bool)));
  pqUseSeparateColorMapReaction* uscmr =
    new pqUseSeparateColorMapReaction(useSeparateColorMapAction, Ui.DisplayColorWidget, false);
  uscmr->setRepresentation(representation);
  this->connect(useSeparateColorMapAction, SIGNAL(changed()), SLOT(updateEnableState()));

  // reset range button
  QAction* resetRangeAction = new QAction(this);
  QObject::connect(Ui.Rescale, SIGNAL(clicked()), resetRangeAction, SLOT(trigger()));
  pqRescaleScalarRangeReaction* rsrr = new pqRescaleScalarRangeReaction(resetRangeAction, false);
  rsrr->setRepresentation(representation);

  // reset custom range button
  QAction* resetCustomRangeAction = new QAction(this);
  resetCustomRangeAction->connect(Ui.RescaleCustom, SIGNAL(clicked()), SLOT(trigger()));
  rsrr = new pqRescaleScalarRangeReaction(
    resetCustomRangeAction, false, pqRescaleScalarRangeReaction::CUSTOM);
  rsrr->setRepresentation(representation);

  // reset custom range button
  QAction* resetTemporalRangeAction = new QAction(this);
  resetTemporalRangeAction->connect(Ui.RescaleTemporal, SIGNAL(clicked()), SLOT(trigger()));
  rsrr = new pqRescaleScalarRangeReaction(
    resetTemporalRangeAction, false, pqRescaleScalarRangeReaction::TEMPORAL);
  rsrr->setRepresentation(representation);

  // choose preset button.
  QAction* choosePresetAction = new QAction(this);
  choosePresetAction->connect(Ui.ChoosePreset, SIGNAL(clicked()), SLOT(trigger()));
  pqChooseColorPresetReaction* ccpr = new pqChooseColorPresetReaction(choosePresetAction, false);
  ccpr->setRepresentation(representation);
  representation->connect(
    ccpr, SIGNAL(presetApplied(const QString&)), SLOT(renderViewEventually()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::~pqColorEditorPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorEditorPropertyWidget::updateEnableState()
{
  Ui::ColorEditorPropertyWidget& ui = this->Internals->Ui;
  const QAction* sbva = this->Internals->ScalarBarVisibilityAction;
  ui.ShowScalarBar->setEnabled(sbva->isEnabled());
  ui.UseSeparateColorMap->setEnabled(sbva->isEnabled());
  ui.Rescale->setEnabled(sbva->isEnabled());
  ui.RescaleCustom->setEnabled(sbva->isEnabled());
  ui.RescaleTemporal->setEnabled(sbva->isEnabled());
  ui.ChoosePreset->setEnabled(sbva->isEnabled());

  const QAction* esba = this->Internals->EditScalarBarAction;
  ui.EditScalarBar->setEnabled(esba->isEnabled());
}
