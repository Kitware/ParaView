// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorToolbar.h"
#include "ui_pqColorToolbar.h"

#include "pqActiveObjects.h"
#include "pqChooseColorPresetReaction.h"
#include "pqDisplayColorWidget.h"
#include "pqEditColorMapReaction.h"
#include "pqEditScalarBarReaction.h"
#include "pqLoadPaletteReaction.h"
#include "pqRescaleScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqSetName.h"
#include "pqUseSeparateColorMapReaction.h"
#include "pqWidgetUtilities.h"

#include <QToolButton>

//-----------------------------------------------------------------------------
void pqColorToolbar::constructor()
{
  Ui::pqColorToolbar ui;
  ui.setupUi(this);
  pqWidgetUtilities::formatChildTooltips(this);

  new pqLoadPaletteReaction(ui.actionLoadPalette);
  QToolButton* tb = qobject_cast<QToolButton*>(this->widgetForAction(ui.actionLoadPalette));
  if (tb)
  {
    tb->setPopupMode(QToolButton::InstantPopup);
  }
  new pqScalarBarVisibilityReaction(ui.actionScalarBarVisibility);
  new pqEditColorMapReaction(ui.actionEditColorMap);
  auto chooseColorPresetReaction = new pqChooseColorPresetReaction(ui.actionColorMapPresets);
  QObject::connect(chooseColorPresetReaction, &pqChooseColorPresetReaction::presetApplied,
    []()
    {
      auto& activeObjects(pqActiveObjects::instance());
      if (auto* rep = activeObjects.activeRepresentation())
      {
        rep->renderViewEventually();
      }
    });

  new pqEditScalarBarReaction(ui.actionScalarBarProperties);
  new pqRescaleScalarRangeReaction(ui.actionResetRange, true, pqRescaleScalarRangeReaction::DATA);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleCustomRange, true, pqRescaleScalarRangeReaction::CUSTOM);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleTemporalRange, true, pqRescaleScalarRangeReaction::TEMPORAL);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleVisibleRange, true, pqRescaleScalarRangeReaction::VISIBLE);

  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(this) << pqSetName("displayColor");
  this->addWidget(display_color);

  new pqUseSeparateColorMapReaction(ui.actionUseSeparateColorMap, display_color);

  QObject::connect(&pqActiveObjects::instance(),
    QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), display_color,
    QOverload<pqDataRepresentation*>::of(&pqDisplayColorWidget::setRepresentation));
}
