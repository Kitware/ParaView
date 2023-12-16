// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorMapEditor.h"
#include "ui_pqColorMapEditor.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqEditScalarBarReaction.h"
#include "pqPipelineSource.h"
#include "pqProxyWidget.h"
#include "pqProxyWidgetDialog.h"
#include "pqSMAdaptor.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqSearchBox.h"
#include "pqSettings.h"
#include "pqUndoStack.h"
#include "pqUse2DTransferFunctionReaction.h"
#include "pqUseSeparateColorMapReaction.h"
#include "pqUseSeparateOpacityArrayReaction.h"

#include "vtkCommand.h"
#include "vtkPVArrayInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSettings.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkWeakPointer.h"

#include <sstream>

#include <QDebug>
#include <QKeyEvent>
#include <QPointer>
#include <QStyle>
#include <QVBoxLayout>

class pqColorMapEditor::pqInternals
{
public:
  Ui::ColorMapEditor Ui;
  QPointer<pqProxyWidget> ProxyWidget;
  QPointer<pqDataRepresentation> ActiveRepresentation;
  QPointer<QAction> ScalarBarVisibilityAction;
  QPointer<QAction> EditScalarBarAction;
  QPointer<QAction> UseSeparateColorArrayAction;
  QPointer<QAction> UseSeparateOpacityArrayAction;
  QPointer<QAction> UseTransfer2DAction;

  unsigned long ObserverId;

  pqInternals(pqColorMapEditor* self)
    : ObserverId(0)
  {
    this->Ui.setupUi(self);

    QVBoxLayout* vbox = new QVBoxLayout(this->Ui.PropertiesFrame);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
  }

  ~pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqColorMapEditor::pqColorMapEditor(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqColorMapEditor::pqInternals(this))
{
  QObject::connect(this->Internals->Ui.SearchBox, SIGNAL(advancedSearchActivated(bool)), this,
    SLOT(updatePanel()));
  QObject::connect(
    this->Internals->Ui.SearchBox, SIGNAL(textChanged(QString)), this, SLOT(updatePanel()));
  QObject::connect(
    this->Internals->Ui.RestoreDefaults, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
  QObject::connect(
    this->Internals->Ui.SaveAsDefaults, SIGNAL(clicked()), this, SLOT(saveAsDefault()));
  QObject::connect(
    this->Internals->Ui.SaveAsArrayDefaults, SIGNAL(clicked()), this, SLOT(saveAsArrayDefault()));
  QObject::connect(
    this->Internals->Ui.AutoUpdate, SIGNAL(clicked(bool)), this, SLOT(setAutoUpdate(bool)));
  QObject::connect(this->Internals->Ui.Update, SIGNAL(clicked()), this, SLOT(renderViews()));

  // Let reactions do the heavy lifting for managing the enabled state of the tool buttons.
  this->Internals->ScalarBarVisibilityAction = new QAction(this);
  this->Internals->Ui.ShowScalarBar->connect(
    this->Internals->ScalarBarVisibilityAction, SIGNAL(toggled(bool)), SLOT(setChecked(bool)));
  this->Internals->ScalarBarVisibilityAction->connect(
    this->Internals->Ui.ShowScalarBar, SIGNAL(clicked()), SLOT(trigger()));
  new pqScalarBarVisibilityReaction(this->Internals->ScalarBarVisibilityAction);

  this->Internals->EditScalarBarAction = new QAction(this);
  this->Internals->EditScalarBarAction->connect(
    this->Internals->Ui.EditScalarBar, SIGNAL(clicked()), SLOT(trigger()));
  new pqEditScalarBarReaction(this->Internals->EditScalarBarAction);

  this->Internals->UseSeparateColorArrayAction = new QAction(this);
  this->Internals->UseSeparateColorArrayAction->connect(
    this->Internals->Ui.UseSeparateColorArray, SIGNAL(clicked()), SLOT(trigger()));
  QObject::connect(this->Internals->UseSeparateColorArrayAction, &QAction::changed, this,
    &pqColorMapEditor::updateColorArraySelectorWidgets);
  this->updateColorArraySelectorWidgets();
  new pqUseSeparateColorMapReaction(
    this->Internals->UseSeparateColorArrayAction, this->Internals->Ui.DisplayColorWidget);

  this->Internals->UseSeparateOpacityArrayAction = new QAction(this);
  this->Internals->UseSeparateOpacityArrayAction->connect(
    this->Internals->Ui.UseSeparateOpacityArray, SIGNAL(clicked()), SLOT(trigger()));
  QObject::connect(this->Internals->UseSeparateOpacityArrayAction, &QAction::changed, this,
    &pqColorMapEditor::updateOpacityArraySelectorWidgets);
  this->updateOpacityArraySelectorWidgets();
  new pqUseSeparateOpacityArrayReaction(this->Internals->UseSeparateOpacityArrayAction);

  this->Internals->UseTransfer2DAction = new QAction(this);
  this->Internals->UseTransfer2DAction->connect(
    this->Internals->Ui.Use2DTransferFunction, SIGNAL(clicked()), SLOT(trigger()));
  QObject::connect(this->Internals->UseTransfer2DAction, &QAction::changed, this,
    &pqColorMapEditor::updateColor2ArraySelectorWidgets);
  this->updateColor2ArraySelectorWidgets();
  new pqUse2DTransferFunctionReaction(this->Internals->UseTransfer2DAction);

  // update the enable state for the buttons based on the actions.
  this->connect(
    this->Internals->ScalarBarVisibilityAction, SIGNAL(changed()), SLOT(updateScalarBarButtons()));
  this->connect(
    this->Internals->EditScalarBarAction, SIGNAL(changed()), SLOT(updateScalarBarButtons()));
  this->updateScalarBarButtons();

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(representationChanged(pqDataRepresentation*)), this,
    SLOT(updateActive()));

  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings)
  {
    this->Internals->Ui.AutoUpdate->setChecked(
      settings->value("autoUpdateColorMapEditor2", true).toBool());
  }
  this->updateActive();
}

//-----------------------------------------------------------------------------
pqColorMapEditor::~pqColorMapEditor()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings)
  {
    // save the state of advanced button in the user config.
    settings->setValue("autoUpdateColorMapEditor2", this->Internals->Ui.AutoUpdate->isChecked());
  }

  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updatePanel()
{
  if (this->Internals->ProxyWidget)
  {
    this->Internals->ProxyWidget->filterWidgets(
      this->Internals->Ui.SearchBox->isAdvancedSearchActive(),
      this->Internals->Ui.SearchBox->text());
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateActive()
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();

  this->setDataRepresentation(repr);

  // Set the current LUT proxy to edit.
  if (repr && vtkSMColorMapEditorHelper::GetUsingScalarColoring(repr->getProxy()))
  {
    vtkSMProxy* lutProxy = vtkSMPropertyHelper(repr->getProxy(), "LookupTable", true).GetAsProxy();
    if (lutProxy)
    {
      // setup property links with the representation.
      if (vtkSMProperty* useTF2DProperty = repr->getProxy()->GetProperty("UseTransfer2D"))
      {
        useTF2DProperty->AddLinkedProperty(lutProxy->GetProperty("Using2DTransferFunction"));
      }
      else
      {
        vtkSMPropertyHelper(lutProxy, "Using2DTransferFunction").Set(0);
      }
      if (vtkSMProperty* useSepOpacityProperty =
            repr->getProxy()->GetProperty("UseSeparateOpacityArray"))
      {
        useSepOpacityProperty->AddLinkedProperty(
          lutProxy->GetProperty("UsingSeparateOpacityArray"));
      }
      else
      {
        vtkSMPropertyHelper(lutProxy, "UsingSeparateOpacityArray").Set(0);
      }
    }
    else if (lutProxy != nullptr)
    {
      vtkSMPropertyHelper(lutProxy, "Using2DTransferFunction").Set(0);
      vtkSMPropertyHelper(lutProxy, "UsingSeparateOpacityArray").Set(0);
    }
    this->setColorTransferFunction(lutProxy);
  }
  else
  {
    this->setColorTransferFunction(nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setDataRepresentation(pqDataRepresentation* repr)
{
  // this method sets up hooks to ensure that when the repr's properties are
  // modified, the editor shows the correct LUT.
  if (this->Internals->ActiveRepresentation == repr)
  {
    return;
  }

  if (this->Internals->ActiveRepresentation)
  {
    // disconnect signals.
    if (this->Internals->ObserverId)
    {
      this->Internals->ActiveRepresentation->getProxy()->RemoveObserver(
        this->Internals->ObserverId);
    }
  }

  this->Internals->ObserverId = 0;
  this->Internals->ActiveRepresentation = repr;
  if (repr && repr->getProxy())
  {
    this->Internals->ObserverId = repr->getProxy()->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, &pqColorMapEditor::updateActive);
  }
  this->Internals->Ui.DisplayColorWidget->setRepresentation(repr);
  this->Internals->Ui.DisplayOpacityWidget->setRepresentation(repr);
  this->Internals->Ui.DisplayColor2Widget->setRepresentation(repr);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setColorTransferFunction(vtkSMProxy* ctf)
{
  Ui::ColorMapEditor& ui = this->Internals->Ui;

  if (this->Internals->ProxyWidget == nullptr && ctf == nullptr)
  {
    return;
  }
  if (this->Internals->ProxyWidget && ctf && this->Internals->ProxyWidget->proxy() == ctf)
  {
    return;
  }

  if ((ctf == nullptr && this->Internals->ProxyWidget) ||
    (this->Internals->ProxyWidget && ctf && this->Internals->ProxyWidget->proxy() != ctf))
  {
    ui.PropertiesFrame->layout()->removeWidget(this->Internals->ProxyWidget);
    delete this->Internals->ProxyWidget;
  }

  ui.RestoreDefaults->setEnabled(ctf != nullptr);
  ui.SaveAsDefaults->setEnabled(ctf != nullptr);
  ui.SaveAsArrayDefaults->setEnabled(ctf != nullptr);
  if (!ctf)
  {
    return;
  }

  pqProxyWidget* widget = new pqProxyWidget(ctf, this);
  widget->setObjectName("Properties");
  widget->setApplyChangesImmediately(true);
  widget->filterWidgets();

  ui.PropertiesFrame->layout()->addWidget(widget);

  this->Internals->ProxyWidget = widget;
  this->updatePanel();

  QObject::connect(widget, SIGNAL(changeFinished()), this, SLOT(updateIfNeeded()));
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateScalarBarButtons()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;
  ui.ShowScalarBar->setEnabled(internals.ScalarBarVisibilityAction->isEnabled());
  ui.EditScalarBar->setEnabled(internals.EditScalarBarAction->isEnabled());
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateColorArraySelectorWidgets()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;

  bool enabled = internals.UseSeparateColorArrayAction->isEnabled();
  ui.UseSeparateColorArray->setEnabled(enabled);

  bool checked = internals.UseSeparateColorArrayAction->isChecked();
  ui.UseSeparateColorArray->setChecked(checked);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateColor2ArraySelectorWidgets()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;

  bool enabled = internals.UseTransfer2DAction->isEnabled();
  ui.Use2DTransferFunction->setEnabled(enabled);

  bool checked = internals.UseTransfer2DAction->isChecked();
  ui.Use2DTransferFunction->setChecked(checked);

  ui.Color2Label->setEnabled(checked && enabled);
  ui.DisplayColor2Widget->setEnabled(checked && enabled);

  if (checked && enabled)
  {
    ui.ColorLabel->setText(tr("Color X"));
  }
  else
  {
    ui.ColorLabel->setText(tr("Color"));
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateOpacityArraySelectorWidgets()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;

  bool enabled = internals.UseSeparateOpacityArrayAction->isEnabled();
  ui.UseSeparateOpacityArray->setEnabled(enabled);

  bool checked = internals.UseSeparateOpacityArrayAction->isChecked();
  ui.UseSeparateOpacityArray->setChecked(checked);

  ui.OpacityLabel->setEnabled(checked && enabled);
  ui.DisplayOpacityWidget->setEnabled(checked && enabled);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::renderViews()
{
  const auto source = this->Internals->ActiveRepresentation->getInput();
  // render all views which display the representation of this pqPipelineSource
  for (auto& view : source->getViews())
  {
    const auto representation = source->getRepresentation(view);
    if (representation && representation->isVisible())
    {
      representation->renderViewEventually();
    }
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::saveAsDefault()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  vtkSMProxy* proxy = this->Internals->ActiveRepresentation->getProxy();
  if (!proxy)
  {
    return;
  }

  vtkSMProxy* lutProxy = pqSMAdaptor::getProxyProperty(proxy->GetProperty("LookupTable"));
  if (lutProxy)
  {
    settings->SetProxySettings(lutProxy);
  }
  else
  {
    qCritical() << "No LookupTable property found.";
  }

  vtkSMProxy* scalarOpacityFunctionProxy = lutProxy
    ? pqSMAdaptor::getProxyProperty(lutProxy->GetProperty("ScalarOpacityFunction"))
    : nullptr;
  if (scalarOpacityFunctionProxy)
  {
    settings->SetProxySettings(scalarOpacityFunctionProxy);
  }
  else
  {
    qCritical("No ScalarOpacityFunction property found");
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::saveAsArrayDefault()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  vtkSMProxy* proxy = this->Internals->ActiveRepresentation->getProxy();
  if (!proxy)
  {
    return;
  }

  vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");

  vtkSMProxy* lutProxy = pqSMAdaptor::getProxyProperty(proxy->GetProperty("LookupTable"));
  if (lutProxy)
  {
    // Remove special characters from the array name
    std::string sanitizedArrayName =
      vtkSMCoreUtilities::SanitizeName(colorArrayHelper.GetInputArrayNameToProcess());

    std::ostringstream prefix;
    prefix << ".array_" << lutProxy->GetXMLGroup() << "." << sanitizedArrayName;

    settings->SetProxySettings(prefix.str().c_str(), lutProxy);
  }
  else
  {
    qCritical() << "No LookupTable property found.";
  }

  vtkSMProxy* scalarOpacityFunctionProxy = lutProxy
    ? pqSMAdaptor::getProxyProperty(lutProxy->GetProperty("ScalarOpacityFunction"))
    : nullptr;
  if (scalarOpacityFunctionProxy)
  {
    settings->SetProxySettings(scalarOpacityFunctionProxy);
  }
  else
  {
    qCritical("No ScalarOpacityFunction property found");
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::restoreDefaults()
{
  vtkSMProxy* proxy = this->Internals->ActiveRepresentation->getProxy();

  vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");
  std::string arrayName =
    vtkSMCoreUtilities::SanitizeName(colorArrayHelper.GetInputArrayNameToProcess());

  BEGIN_UNDO_SET(tr("Reset color map to defaults"));
  if (vtkSMProxy* lutProxy = vtkSMPropertyHelper(proxy, "LookupTable").GetAsProxy())
  {
    // Load array-specific preset, if specified.
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    std::string stdPresetsKey = ".standard_presets.";
    stdPresetsKey += arrayName;
    if (settings->HasSetting(stdPresetsKey.c_str()))
    {
      vtkSMTransferFunctionProxy::ApplyPreset(lutProxy,
        settings->GetSettingAsString(stdPresetsKey.c_str(), 0, "").c_str(),
        /*rescale=*/false);

      // Should probably support setting a standard preset for opacity function at some point. */
    }
    else
    {
      vtkSMTransferFunctionProxy::ResetPropertiesToDefaults(lutProxy, arrayName.c_str(), true);
      if (vtkSMProxy* sofProxy =
            vtkSMPropertyHelper(lutProxy, "ScalarOpacityFunction").GetAsProxy())
      {
        vtkSMTransferFunctionProxy::ResetPropertiesToDefaults(sofProxy, arrayName.c_str(), true);
      }
    }
  }
  END_UNDO_SET();
  this->renderViews();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setAutoUpdate(bool val)
{
  this->Internals->Ui.AutoUpdate->setChecked(val);
  this->updateIfNeeded();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateIfNeeded()
{
  if (this->Internals->Ui.AutoUpdate->isChecked())
  {
    this->renderViews();
  }
}
