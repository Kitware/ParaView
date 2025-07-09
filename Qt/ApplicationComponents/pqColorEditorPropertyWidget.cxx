// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorEditorPropertyWidget.h"
#include "ui_pqColorEditorPropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqChooseColorPresetReaction.h"
#include "pqDataRepresentation.h"
#include "pqEditColorMapReaction.h"
#include "pqEditScalarBarReaction.h"
#include "pqMultiBlockPropertiesStateWidget.h"
#include "pqPropertiesPanel.h"
#include "pqRescaleScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqServerManagerModel.h"
#include "pqUseSeparateColorMapReaction.h"

#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProxy.h"

#include <QAction>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QWidget>

#include <algorithm>
#include <vector>

class pqColorEditorPropertyWidget::pqInternals : public QObject
{
public:
  Ui::ColorEditorPropertyWidget Ui;

  QPointer<pqScalarBarVisibilityReaction> ScalarBarVisibilityReaction;
  QPointer<pqUseSeparateColorMapReaction> UseSeparateColorMapReaction;
  QPointer<pqChooseColorPresetReaction> ChooseColorPresetReaction;

  QPointer<QAction> ScalarBarVisibilityAction;
  QPointer<QAction> EditScalarBarAction;

  QPointer<pqDataRepresentation> Representation;
  QPointer<pqMultiBlockPropertiesStateWidget> StateWidget;

  std::vector<vtkSMProxy*> OldLuts;
};

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::pqColorEditorPropertyWidget(
  vtkSMProxy* smProxy, QWidget* parentObject, int selectedPropertiesType)
  : Superclass(smProxy, parentObject)
  , Internals(new pqColorEditorPropertyWidget::pqInternals())
{
  this->setShowLabel(true);
  auto& internals = *this->Internals;

  Ui::ColorEditorPropertyWidget& Ui = internals.Ui;
  Ui.setupUi(this);
  Ui.gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargins());
  Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // Get the representation.
  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* pqproxy = smm->findItem<pqProxy*>(smProxy);
  internals.Representation = qobject_cast<pqDataRepresentation*>(pqproxy);

  // Setup various widget properties.
  Ui.DisplayColorWidget->setRepresentation(internals.Representation, selectedPropertiesType);

  // show scalar bar button
  QAction* scalarBarAction = new QAction(this);
  this->Internals->ScalarBarVisibilityAction = scalarBarAction;
  QObject::connect(Ui.ShowScalarBar, &QPushButton::clicked, scalarBarAction, &QAction::trigger);
  QObject::connect(scalarBarAction, &QAction::toggled, Ui.ShowScalarBar, &QPushButton::setChecked);
  this->Internals->ScalarBarVisibilityReaction =
    new pqScalarBarVisibilityReaction(scalarBarAction, /*track_active_objects=*/false);
  this->Internals->ScalarBarVisibilityReaction->setRepresentation(
    internals.Representation, selectedPropertiesType);
  QObject::connect(
    scalarBarAction, &QAction::changed, this, &pqColorEditorPropertyWidget::updateEnableState);

  // edit scalar bar.
  QAction* editScalarBarAction = new QAction(this);
  this->Internals->EditScalarBarAction = editScalarBarAction;
  QObject::connect(Ui.EditScalarBar, &QPushButton::clicked, editScalarBarAction, &QAction::trigger);
  pqEditScalarBarReaction* esbr =
    new pqEditScalarBarReaction(editScalarBarAction, /*track_active_objects=*/false);
  esbr->setScalarBarVisibilityReaction(this->Internals->ScalarBarVisibilityReaction);
  esbr->setRepresentation(internals.Representation, selectedPropertiesType);
  QObject::connect(
    editScalarBarAction, &QAction::changed, this, &pqColorEditorPropertyWidget::updateEnableState);

  // edit color map button
  QAction* editColorMapAction = new QAction(this);
  editColorMapAction->setCheckable(true);
  editColorMapAction->setChecked(Ui.EditColorMap->isChecked());
  QObject::connect(Ui.EditColorMap, &QPushButton::toggled, editColorMapAction, &QAction::trigger);
  QObject::connect(editColorMapAction, &QAction::toggled, Ui.EditColorMap,
    [=](bool checked)
    {
      // Synchronize the check state of the action with the pushbutton
      // block signal to avoid infinite loop
      bool blocked = Ui.EditColorMap->blockSignals(true);
      Ui.EditColorMap->setChecked(checked);
      Ui.EditColorMap->blockSignals(blocked);
    });
  pqEditColorMapReaction* ecmr =
    new pqEditColorMapReaction(editColorMapAction, /*track_active_objects=*/false);
  ecmr->setRepresentation(internals.Representation, selectedPropertiesType);

  // separate color map button
  QAction* useSeparateColorMapAction = new QAction(this);
  QObject::connect(
    Ui.UseSeparateColorMap, &QPushButton::clicked, useSeparateColorMapAction, &QAction::trigger);
  QObject::connect(
    useSeparateColorMapAction, &QAction::toggled, Ui.UseSeparateColorMap, &QPushButton::setChecked);
  this->Internals->UseSeparateColorMapReaction = new pqUseSeparateColorMapReaction(
    useSeparateColorMapAction, Ui.DisplayColorWidget, /*track_active_objects=*/false);
  this->Internals->UseSeparateColorMapReaction->setRepresentation(
    internals.Representation, selectedPropertiesType);
  QObject::connect(useSeparateColorMapAction, &QAction::changed, this,
    &pqColorEditorPropertyWidget::updateEnableState);

  // reset range button
  QAction* resetRangeAction = new QAction(this);
  QObject::connect(Ui.Rescale, &QPushButton::clicked, resetRangeAction, &QAction::trigger);
  pqRescaleScalarRangeReaction* rsrr = new pqRescaleScalarRangeReaction(
    resetRangeAction, /*track_active_objects=*/false, pqRescaleScalarRangeReaction::DATA);
  rsrr->setRepresentation(internals.Representation, selectedPropertiesType);

  // reset custom range button
  QAction* resetCustomRangeAction = new QAction(this);
  QObject::connect(
    Ui.RescaleCustom, &QPushButton::clicked, resetCustomRangeAction, &QAction::trigger);
  rsrr = new pqRescaleScalarRangeReaction(
    resetCustomRangeAction, /*track_active_objects=*/false, pqRescaleScalarRangeReaction::CUSTOM);
  rsrr->setRepresentation(internals.Representation, selectedPropertiesType);

  // reset custom range button
  QAction* resetTemporalRangeAction = new QAction(this);
  QObject::connect(
    Ui.RescaleTemporal, &QPushButton::clicked, resetTemporalRangeAction, &QAction::trigger);
  rsrr = new pqRescaleScalarRangeReaction(resetTemporalRangeAction, /*track_active_objects=*/false,
    pqRescaleScalarRangeReaction::TEMPORAL);
  rsrr->setRepresentation(internals.Representation, selectedPropertiesType);

  // choose preset button.
  QAction* choosePresetAction = new QAction(this);
  QObject::connect(Ui.ChoosePreset, &QPushButton::clicked, choosePresetAction, &QAction::trigger);
  this->Internals->ChooseColorPresetReaction =
    new pqChooseColorPresetReaction(choosePresetAction, /*track_active_objects=*/false);
  this->Internals->ChooseColorPresetReaction->setRepresentation(
    internals.Representation, selectedPropertiesType);
  QObject::connect(this->Internals->ChooseColorPresetReaction,
    &pqChooseColorPresetReaction::presetApplied, internals.Representation,
    &pqDataRepresentation::renderViewEventually);

  if (selectedPropertiesType == vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
  {
    const int iconSize = std::max(this->style()->pixelMetric(QStyle::PM_SmallIconSize), 20);
    this->Internals->StateWidget = new pqMultiBlockPropertiesStateWidget(smProxy,
      { "BlockColors", "BlockColorArrayNames", "BlockUseSeparateColorMaps" }, iconSize, QString(),
      this);
    const int numColumns = Ui.gridLayout->columnCount();
    Ui.gridLayout->addWidget(this->Internals->StateWidget, 1, numColumns, 1, 1, Qt::AlignVCenter);
    QObject::connect(this->Internals->StateWidget, &pqMultiBlockPropertiesStateWidget::stateChanged,
      this, &pqColorEditorPropertyWidget::updateBlockBasedEnableState);
    QObject::connect(this->Internals->StateWidget,
      &pqMultiBlockPropertiesStateWidget::selectedBlockSelectorsChanged, this,
      &pqColorEditorPropertyWidget::updateBlockBasedEnableState);
    QObject::connect(internals.StateWidget, &pqMultiBlockPropertiesStateWidget::startStateReset,
      this,
      [&]()
      {
        this->Internals->OldLuts =
          this->Internals->Representation->getLookupTableProxies(vtkSMColorMapEditorHelper::Blocks);
      });
    QObject::connect(internals.StateWidget, &pqMultiBlockPropertiesStateWidget::endStateReset, this,
      [&]()
      {
        pqDisplayColorWidget::hideScalarBarsIfNotNeeded(
          this->Internals->Representation->getViewProxy(), this->Internals->OldLuts);
        this->Internals->Representation->renderViewEventually();
      });
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::~pqColorEditorPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqColorEditorPropertyWidget::updateBlockBasedEnableState()
{
  this->Internals->Ui.DisplayColorWidget->queryCurrentSelectedArray();
  this->Internals->UseSeparateColorMapReaction->querySelectedUseSeparateColorMap();
  this->Internals->ScalarBarVisibilityReaction->updateEnableState();
  this->Internals->ChooseColorPresetReaction->setTransferFunctions(
    this->Internals->Representation->getLookupTableProxies(vtkSMColorMapEditorHelper::Blocks));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqColorEditorPropertyWidget::updateEnableState()
{
  auto& internals = *this->Internals;
  bool enabled = true;
  if (!internals.StateWidget.isNull())
  {
    // enable any widget only if there is at least a block selected
    enabled = internals.StateWidget->getState() !=
      pqMultiBlockPropertiesStateWidget::BlockPropertyState::Disabled;
  }
  Ui::ColorEditorPropertyWidget& ui = internals.Ui;
  ui.DisplayColorWidget->setEnabled(enabled);
  ui.EditColorMap->setEnabled(enabled);
  const bool isScalarBarVisibilityActionEnabled = internals.ScalarBarVisibilityAction->isEnabled();
  ui.UseSeparateColorMap->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  ui.Rescale->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  ui.RescaleCustom->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  ui.RescaleTemporal->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  ui.ChoosePreset->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  ui.ShowScalarBar->setEnabled(enabled && isScalarBarVisibilityActionEnabled);
  const bool isEditScalarBarActionEnabled = internals.EditScalarBarAction->isEnabled();
  ui.EditScalarBar->setEnabled(enabled && isEditScalarBarActionEnabled);
}
