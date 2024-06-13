// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorMapEditor.h"
#include "ui_pqColorMapEditor.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqBlockProxyWidget.h"
#include "pqDataRepresentation.h"
#include "pqEditScalarBarReaction.h"
#include "pqProxyWidget.h"
#include "pqSMAdaptor.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqSearchBox.h"
#include "pqSettings.h"
#include "pqUndoStack.h"
#include "pqUse2DTransferFunctionReaction.h"
#include "pqUseSeparateColorMapReaction.h"
#include "pqUseSeparateOpacityArrayReaction.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"

#include <QAction>
#include <QKeyEvent>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

class pqColorMapEditor::pqInternals
{
public:
  Ui::ColorMapEditor Ui;
  QPointer<pqProxyWidget> ProxyWidget;
  QPointer<pqDataRepresentation> Representation;

  QPointer<pqScalarBarVisibilityReaction> ScalarBarVisibilityReaction;
  QPointer<pqEditScalarBarReaction> EditScalarBarReaction;
  QPointer<pqUseSeparateColorMapReaction> UseSeparateColorMapReaction;
  QPointer<pqUseSeparateOpacityArrayReaction> UseSeparateOpacityArrayReaction;
  QPointer<pqUse2DTransferFunctionReaction> UseTransfer2DReaction;

  QPointer<QAction> ScalarBarVisibilityAction;
  QPointer<QAction> EditScalarBarAction;
  QPointer<QAction> UseSeparateColorArrayAction;
  QPointer<QAction> UseSeparateOpacityArrayAction;
  QPointer<QAction> UseTransfer2DAction;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;
  std::vector<vtkSMProxy*> CTFs;
  vtkSmartPointer<vtkSMProxy> CopyFirstCTF;

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
  QObject::connect(this->Internals->Ui.SearchBox, &pqSearchBox::advancedSearchActivated, this,
    &pqColorMapEditor::updatePanel);
  QObject::connect(
    this->Internals->Ui.SearchBox, &pqSearchBox::textChanged, this, &pqColorMapEditor::updatePanel);
  QObject::connect(this->Internals->Ui.RestoreDefaults, &QPushButton::clicked, this,
    &pqColorMapEditor::restoreDefaults);
  QObject::connect(this->Internals->Ui.SaveAsDefaults, &QPushButton::clicked, this,
    &pqColorMapEditor::saveAsDefault);
  QObject::connect(this->Internals->Ui.SaveAsArrayDefaults, &QPushButton::clicked, this,
    &pqColorMapEditor::saveAsArrayDefault);
  QObject::connect(
    this->Internals->Ui.AutoUpdate, &QPushButton::clicked, this, &pqColorMapEditor::setAutoUpdate);
  QObject::connect(
    this->Internals->Ui.Update, &QPushButton::clicked, this, &pqColorMapEditor::renderViews);

  // Let reactions do the heavy lifting for managing the enabled state of the tool buttons.
  this->Internals->ScalarBarVisibilityAction = new QAction(this);

  QObject::connect(this->Internals->ScalarBarVisibilityAction, &QAction::toggled,
    this->Internals->Ui.ShowScalarBar, &QToolButton::setChecked);
  QObject::connect(this->Internals->Ui.ShowScalarBar, &QToolButton::clicked,
    this->Internals->ScalarBarVisibilityAction, &QAction::trigger);
  this->Internals->ScalarBarVisibilityReaction = new pqScalarBarVisibilityReaction(
    this->Internals->ScalarBarVisibilityAction, /*track_active_objects=*/false);

  this->Internals->EditScalarBarAction = new QAction(this);
  QObject::connect(this->Internals->Ui.EditScalarBar, &QToolButton::clicked,
    this->Internals->EditScalarBarAction, &QAction::trigger);
  this->Internals->EditScalarBarReaction = new pqEditScalarBarReaction(
    this->Internals->EditScalarBarAction, /*track_active_objects=*/false);
  this->Internals->EditScalarBarReaction->setScalarBarVisibilityReaction(
    this->Internals->ScalarBarVisibilityReaction);

  this->Internals->UseSeparateColorArrayAction = new QAction(this);
  QObject::connect(this->Internals->Ui.UseSeparateColorArray, &QToolButton::clicked,
    this->Internals->UseSeparateColorArrayAction, &QAction::trigger);
  QObject::connect(this->Internals->UseSeparateColorArrayAction, &QAction::changed, this,
    &pqColorMapEditor::updateColorArraySelectorWidgets);
  this->updateColorArraySelectorWidgets();
  this->Internals->UseSeparateColorMapReaction =
    new pqUseSeparateColorMapReaction(this->Internals->UseSeparateColorArrayAction,
      this->Internals->Ui.DisplayColorWidget, /*track_active_objects=*/false);

  this->Internals->UseSeparateOpacityArrayAction = new QAction(this);
  QObject::connect(this->Internals->Ui.UseSeparateOpacityArray, &QToolButton::clicked,
    this->Internals->UseSeparateOpacityArrayAction, &QAction::trigger);
  QObject::connect(this->Internals->UseSeparateOpacityArrayAction, &QAction::changed, this,
    &pqColorMapEditor::updateOpacityArraySelectorWidgets);
  this->updateOpacityArraySelectorWidgets();
  this->Internals->UseSeparateOpacityArrayReaction = new pqUseSeparateOpacityArrayReaction(
    this->Internals->UseSeparateOpacityArrayAction, /*track_active_objects=*/false);

  this->Internals->UseTransfer2DAction = new QAction(this);
  QObject::connect(this->Internals->Ui.Use2DTransferFunction, &QToolButton::clicked,
    this->Internals->UseTransfer2DAction, &QAction::trigger);
  QObject::connect(this->Internals->UseTransfer2DAction, &QAction::changed, this,
    &pqColorMapEditor::updateColor2ArraySelectorWidgets);
  this->updateColor2ArraySelectorWidgets();
  this->Internals->UseTransfer2DReaction =
    new pqUse2DTransferFunctionReaction(this->Internals->UseTransfer2DAction,
      /*track_active_objects=*/false);

  // update the enable state for the buttons based on the actions.
  QObject::connect(this->Internals->ScalarBarVisibilityAction, &QAction::changed, this,
    &pqColorMapEditor::updateScalarBarButtons);
  QObject::connect(this->Internals->EditScalarBarAction, &QAction::changed, this,
    &pqColorMapEditor::updateScalarBarButtons);
  this->updateScalarBarButtons();

  auto selectedPropertiesComboBox = this->Internals->Ui.SelectedPropertiesComboBox;
  selectedPropertiesComboBox->addItem(tr("Representation"));
  selectedPropertiesComboBox->addItem(tr("Block(s)"));
  selectedPropertiesComboBox->setCurrentIndex(0);
  QObject::connect(selectedPropertiesComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    this, &pqColorMapEditor::setSelectedPropertiesType);

  QObject::connect(&pqActiveObjects::instance(),
    QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
    QOverload<>::of(&pqColorMapEditor::updateActive));

  if (pqSettings* settings = pqApplicationCore::instance()->settings())
  {
    this->Internals->Ui.AutoUpdate->setChecked(
      settings->value("autoUpdateColorMapEditor2", true).toBool());
  }
  this->updateActive();
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setSelectedPropertiesType(int selectedPropertiesType)
{
  const int oldSelectedPropertiesType =
    this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType();
  if (oldSelectedPropertiesType != selectedPropertiesType)
  {
    this->Internals->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
    const bool prev = this->blockSignals(true);
    this->Internals->Ui.SelectedPropertiesComboBox->setCurrentIndex(selectedPropertiesType);
    this->blockSignals(prev);
    this->updateActive(/*forceUpdate=*/true);
  }
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
void pqColorMapEditor::updateActive(bool forceUpdate)
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  this->setRepresentation(repr, forceUpdate);

  auto colorMapEditorHelper = this->Internals->ColorMapEditorHelper.Get();
  // Set the current LUT proxy to edit.
  if (repr && colorMapEditorHelper->GetAnySelectedUsingScalarColoring(repr->getProxy()))
  {
    auto luts = colorMapEditorHelper->GetSelectedLookupTables(repr->getProxy());
    for (vtkSMProxy* lutProxy : luts)
    {
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
    }
    this->setColorTransferFunctions(luts);
  }
  else
  {
    this->setColorTransferFunction(nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setRepresentation(pqDataRepresentation* repr, bool forceUpdate)
{
  // this method sets up hooks to ensure that when the repr's properties are
  // modified, the editor shows the correct LUT.
  if (this->Internals->Representation == repr && !forceUpdate)
  {
    return;
  }

  if (this->Internals->Representation)
  {
    // disconnect signals.
    if (this->Internals->ObserverId)
    {
      this->Internals->Representation->getProxy()->RemoveObserver(this->Internals->ObserverId);
    }
  }

  this->Internals->ObserverId = 0;
  this->Internals->Representation = repr;
  if (repr && repr->getProxy())
  {
    this->Internals->ObserverId = repr->getProxy()->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, QOverload<>::of(&pqColorMapEditor::updateActive));
  }
  auto selectedPropertiesType = this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType();
  this->Internals->Ui.DisplayColorWidget->setRepresentation(repr, selectedPropertiesType);
  this->Internals->Ui.DisplayOpacityWidget->setRepresentation(repr); // no block support
  this->Internals->Ui.DisplayColor2Widget->setRepresentation(repr);  // no block support
  this->Internals->ScalarBarVisibilityReaction->setRepresentation(repr, selectedPropertiesType);
  this->Internals->EditScalarBarReaction->setRepresentation(repr, selectedPropertiesType);
  this->Internals->UseSeparateColorMapReaction->setRepresentation(repr, selectedPropertiesType);
  this->Internals->UseSeparateOpacityArrayReaction->setRepresentation(repr); // no block support
  this->Internals->UseTransfer2DReaction->setRepresentation(repr);           // no block support
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::setColorTransferFunctions(std::vector<vtkSMProxy*> ctfs)
{
  Ui::ColorMapEditor& ui = this->Internals->Ui;
  vtkSMProxy* firstCTF = ctfs.empty() ? nullptr : ctfs[0];

  if (this->Internals->ProxyWidget == nullptr && firstCTF == nullptr)
  {
    return;
  }
  if (this->Internals->ProxyWidget && firstCTF &&
    this->Internals->ProxyWidget->proxy() == firstCTF && this->Internals->CTFs == ctfs)
  {
    return;
  }

  if ((firstCTF == nullptr && this->Internals->ProxyWidget) ||
    (this->Internals->ProxyWidget && firstCTF &&
      (this->Internals->ProxyWidget->proxy() != firstCTF || this->Internals->CTFs != ctfs)))
  {
    ui.PropertiesFrame->layout()->removeWidget(this->Internals->ProxyWidget);
    delete this->Internals->ProxyWidget;
  }

  ui.RestoreDefaults->setEnabled(firstCTF != nullptr);
  ui.SaveAsDefaults->setEnabled(firstCTF != nullptr);
  ui.SaveAsArrayDefaults->setEnabled(firstCTF != nullptr);
  if (!firstCTF)
  {
    return;
  }

  pqProxyWidget* widget;
  if (this->Internals->ColorMapEditorHelper->GetSelectedPropertiesType() ==
    vtkSMColorMapEditorHelper::Representation)
  {
    widget = new pqProxyWidget(firstCTF, this);
  }
  else
  {
    const std::vector<std::string> selectedBlockSelectors =
      this->Internals->ColorMapEditorHelper->GetSelectedBlockSelectors(
        this->Internals->Representation->getProxy());
    widget = new pqBlockProxyWidget(firstCTF, selectedBlockSelectors[0].c_str(), this);
  }
  widget->setObjectName("Properties");
  widget->setApplyChangesImmediately(true);
  widget->filterWidgets();

  ui.PropertiesFrame->layout()->addWidget(widget);

  this->Internals->ProxyWidget = widget;
  this->Internals->CTFs = ctfs;
  this->Internals->CopyFirstCTF =
    vtk::TakeSmartPointer(this->Internals->Representation->proxyManager()->NewProxy(
      firstCTF->GetXMLGroup(), firstCTF->GetXMLName()));
  this->Internals->CopyFirstCTF->Copy(firstCTF);
  this->updatePanel();

  QObject::connect(widget, &pqProxyWidget::changeFinished, this, &pqColorMapEditor::updateIfNeeded);
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

  const bool enabled = internals.UseSeparateColorArrayAction->isEnabled();
  ui.UseSeparateColorArray->setEnabled(enabled);

  const bool checked = internals.UseSeparateColorArrayAction->isChecked();
  ui.UseSeparateColorArray->setChecked(checked);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateColor2ArraySelectorWidgets()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;

  const bool enabled = internals.UseTransfer2DAction->isEnabled();
  ui.Use2DTransferFunction->setEnabled(enabled);

  const bool checked = internals.UseTransfer2DAction->isChecked();
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

  // when 2D transfer function is enabled, disable the option to use block properties
  const int currentIndex = ui.SelectedPropertiesComboBox->currentIndex();
  this->Internals->Ui.SelectedPropertiesComboBox->setCurrentIndex(enabled ? 0 : currentIndex);
  this->Internals->Ui.SelectedPropertiesComboBox->setEnabled(!enabled);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::updateOpacityArraySelectorWidgets()
{
  pqInternals& internals = *this->Internals;
  Ui::ColorMapEditor& ui = internals.Ui;

  const bool enabled = internals.UseSeparateOpacityArrayAction->isEnabled();
  ui.UseSeparateOpacityArray->setEnabled(enabled);

  const bool checked = internals.UseSeparateOpacityArrayAction->isChecked();
  ui.UseSeparateOpacityArray->setChecked(checked);

  ui.OpacityLabel->setEnabled(checked && enabled);
  ui.DisplayOpacityWidget->setEnabled(checked && enabled);

  // when separate opacity function is enabled, disable the option to use block properties
  const int currentIndex = ui.SelectedPropertiesComboBox->currentIndex();
  this->Internals->Ui.SelectedPropertiesComboBox->setCurrentIndex(enabled ? 0 : currentIndex);
  this->Internals->Ui.SelectedPropertiesComboBox->setEnabled(!enabled);
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::renderViews()
{
  const auto source = this->Internals->Representation->getInput();
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

  vtkSMProxy* repr = this->Internals->Representation->getProxy();
  if (!repr)
  {
    return;
  }

  const std::vector<vtkSMProxy*> luts =
    this->Internals->ColorMapEditorHelper->GetSelectedLookupTables(repr);
  for (vtkSMProxy* lutProxy : luts)
  {
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
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::saveAsArrayDefault()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  vtkSMProxy* repr = this->Internals->Representation->getProxy();
  if (!repr)
  {
    return;
  }

  auto colorMapEditorHelper = this->Internals->ColorMapEditorHelper.Get();
  // get the array names
  std::vector<std::pair<int, std::string>> arrayNames =
    colorMapEditorHelper->GetSelectedColorArrays(repr);
  // Remove special characters from the array names
  std::transform(arrayNames.begin(), arrayNames.end(), arrayNames.begin(),
    [](const std::pair<int, std::string>& pair) {
      return std::make_pair(pair.first, vtkSMCoreUtilities::SanitizeName(pair.second));
    });

  const std::vector<vtkSMProxy*> luts = colorMapEditorHelper->GetSelectedLookupTables(repr);

  for (size_t i = 0; i < arrayNames.size(); ++i)
  {
    const std::string& arrayName = arrayNames[i].second;
    vtkSMProxy* lutProxy = luts[i];
    if (lutProxy)
    {
      std::ostringstream prefix;
      prefix << ".array_" << lutProxy->GetXMLGroup() << "." << arrayName;

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
}

//-----------------------------------------------------------------------------
void pqColorMapEditor::restoreDefaults()
{
  vtkSMProxy* repr = this->Internals->Representation->getProxy();

  auto colorMapEditorHelper = this->Internals->ColorMapEditorHelper.Get();
  // get the array names
  std::vector<std::pair<int, std::string>> arrayNames =
    colorMapEditorHelper->GetSelectedColorArrays(repr);
  // Remove special characters from the array names
  std::transform(arrayNames.begin(), arrayNames.end(), arrayNames.begin(),
    [](const std::pair<int, std::string>& pair) {
      return std::make_pair(pair.first, vtkSMCoreUtilities::SanitizeName(pair.second));
    });

  const std::vector<vtkSMProxy*> luts = colorMapEditorHelper->GetSelectedLookupTables(repr);

  const QString undoText = tr("Reset ") +
    QCoreApplication::translate("ServerManagerXML",
      colorMapEditorHelper->GetSelectedColorArrayProperty(repr)->GetXMLLabel()) +
    tr(" to defaults");
  BEGIN_UNDO_SET(undoText);
  for (size_t i = 0; i < arrayNames.size(); ++i)
  {
    const std::string& arrayName = arrayNames[i].second;
    vtkSMProxy* lutProxy = luts[i];
    if (lutProxy)
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
  // if an update occurred, update all other color transfer functions
  if (this->Internals->ProxyWidget && this->Internals->ProxyWidget->proxy())
  {
    auto firstCTF = this->Internals->ProxyWidget->proxy();
    auto changedProperties =
      firstCTF->GetPropertiesWithDifferentValues(this->Internals->CopyFirstCTF);
    const std::vector<std::string> selectedBlockSelectors =
      this->Internals->ColorMapEditorHelper->GetSelectedBlockSelectors(
        this->Internals->Representation->getProxy());
    for (size_t i = 0; i < selectedBlockSelectors.size(); ++i)
    {
      vtkSMProxy* otherCTF = this->Internals->CTFs[i];
      const std::string& selector = selectedBlockSelectors[i];
      if (otherCTF && otherCTF != firstCTF)
      {
        SM_SCOPED_TRACE(PropertiesModified)
          .arg("proxy", otherCTF)
          .arg("selector", selector.c_str());
        for (const auto& prop : changedProperties)
        {
          otherCTF->GetProperty(prop.c_str())->Copy(firstCTF->GetProperty(prop.c_str()));
        }
      }
    }
    // copy the updated properties to the copy
    this->Internals->CopyFirstCTF->Copy(firstCTF);
  }
  if (this->Internals->Ui.AutoUpdate->isChecked())
  {
    this->renderViews();
  }
}
