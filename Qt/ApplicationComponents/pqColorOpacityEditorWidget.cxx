/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqColorOpacityEditorWidget.h"
#include "ui_pqColorOpacityEditorWidget.h"
#include "ui_pqSavePresetOptions.h"

#include "pqActiveObjects.h"
#include "pqChooseColorPresetReaction.h"
#include "pqColorTableModel.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqOpacityTableModel.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqResetScalarRangeReaction.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqTransferFunctionWidget.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkTable.h"
#include "vtkVector.h"
#include "vtkWeakPointer.h"
#include "vtk_jsoncpp.h"

#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>
#include <QtDebug>

#include <cassert>
#include <cmath>
#include <set>

namespace
{
//-----------------------------------------------------------------------------
// Decorator used to hide the widget when using IndexedLookup.
class pqColorOpacityEditorWidgetDecorator : public pqPropertyWidgetDecorator
{
  typedef pqPropertyWidgetDecorator Superclass;
  bool Hidden;

public:
  pqColorOpacityEditorWidgetDecorator(vtkPVXMLElement* xmlArg, pqPropertyWidget* parentArg)
    : Superclass(xmlArg, parentArg)
    , Hidden(false)
  {
  }
  ~pqColorOpacityEditorWidgetDecorator() override {}

  void setHidden(bool val)
  {
    if (val != this->Hidden)
    {
      this->Hidden = val;
      Q_EMIT this->visibilityChanged();
    }
  }
  bool canShowWidget(bool show_advanced) const override
  {
    Q_UNUSED(show_advanced);
    return !this->Hidden;
  }

private:
  Q_DISABLE_COPY(pqColorOpacityEditorWidgetDecorator)
};

} // end anonymous namespace

//-----------------------------------------------------------------------------
class pqColorOpacityEditorWidget::pqInternals
{
public:
  Ui::ColorOpacityEditorWidget Ui;
  pqColorTableModel ColorTableModel;
  pqOpacityTableModel OpacityTableModel;
  QPointer<pqColorOpacityEditorWidgetDecorator> Decorator;
  vtkWeakPointer<vtkSMPropertyGroup> PropertyGroup;
  vtkWeakPointer<vtkSMProxy> ScalarOpacityFunctionProxy;
  QScopedPointer<QAction> TempAction;
  QScopedPointer<pqChooseColorPresetReaction> ChoosePresetReaction;

  // We use this pqPropertyLinks instance to simply monitor smproperty changes.
  pqPropertyLinks LinksForMonitoringChanges;
  vtkNew<vtkEventQtSlotConnect> TransferFunctionConnector;
  vtkNew<vtkEventQtSlotConnect> RangeConnector;
  vtkNew<vtkEventQtSlotConnect> ConsumerConnector;

  pqTimer HistogramTimer;
  bool HistogramOutdated = true;

  pqInternals(pqColorOpacityEditorWidget* self, vtkSMPropertyGroup* group)
    : ColorTableModel(self)
    , OpacityTableModel(self)
    , PropertyGroup(group)
    , TempAction(new QAction(self))
    , ChoosePresetReaction(new pqChooseColorPresetReaction(this->TempAction.data(), false))
  {
    this->Ui.setupUi(self);
    this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());

    this->Decorator = new pqColorOpacityEditorWidgetDecorator(nullptr, self);

    this->Ui.ColorTable->setModel(&this->ColorTableModel);
    this->Ui.ColorTable->horizontalHeader()->setHighlightSections(false);
    this->Ui.ColorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->Ui.ColorTable->horizontalHeader()->setStretchLastSection(true);

    this->Ui.OpacityTable->setModel(&this->OpacityTableModel);
    this->Ui.OpacityTable->horizontalHeader()->setHighlightSections(false);
    this->Ui.OpacityTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->Ui.OpacityTable->horizontalHeader()->setStretchLastSection(true);

    QObject::connect(this->ChoosePresetReaction.data(), SIGNAL(presetApplied(const QString&)), self,
      SLOT(presetApplied()));

    this->HistogramTimer.setSingleShot(true);
    this->HistogramTimer.setInterval(1);
    QObject::connect(&this->HistogramTimer, SIGNAL(timeout()), self, SLOT(realShowDataHistogram()));
  }

  void render()
  {
    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (repr)
    {
      repr->renderViewEventually();
      return;
    }
    pqView* activeView = pqActiveObjects::instance().activeView();
    if (activeView)
    {
      activeView->render();
      return;
    }
    pqApplicationCore::instance()->render();
  }
};

//-----------------------------------------------------------------------------
pqColorOpacityEditorWidget::pqColorOpacityEditorWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this, smgroup))
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->proxy()->GetClientSideObject());
  if (stc)
  {
    ui.ColorEditor->initialize(stc, true, nullptr, false);
    QObject::connect(&this->Internals->ColorTableModel,
      SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
      SIGNAL(xrgbPointsChanged()));
    QObject::connect(&this->Internals->OpacityTableModel,
      SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
      SIGNAL(xvmsPointsChanged()));
  }
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqRepresentation*)),
    this, SLOT(representationOrViewChanged()));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(representationOrViewChanged()));

  // To avoid color editor widget movement when hidden
  QSizePolicy sp_retain = ui.OpacityEditor->sizePolicy();
  sp_retain.setRetainSizeWhenHidden(true);
  ui.OpacityEditor->setSizePolicy(sp_retain);

  QObject::connect(ui.OpacityEditor, SIGNAL(currentPointChanged(vtkIdType)), this,
    SLOT(opacityCurrentChanged(vtkIdType)));
  QObject::connect(ui.ColorEditor, SIGNAL(currentPointChanged(vtkIdType)), this,
    SLOT(colorCurrentChanged(vtkIdType)));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()), this, SIGNAL(xrgbPointsChanged()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()), this, SIGNAL(xvmsPointsChanged()));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()), this, SLOT(updateCurrentData()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()), this, SLOT(updateCurrentData()));

  QObject::connect(ui.ColorEditor, SIGNAL(rangeHandlesRangeChanged(double, double)), this,
    SLOT(onRangeHandlesRangeChanged(double, double)));
  QObject::connect(ui.OpacityEditor, SIGNAL(rangeHandlesRangeChanged(double, double)), this,
    SLOT(onRangeHandlesRangeChanged(double, double)));

  QObject::connect(
    ui.ColorEditor, SIGNAL(rangeHandlesDoubleClicked()), this, SLOT(resetRangeToCustom()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(rangeHandlesDoubleClicked()), this, SLOT(resetRangeToCustom()));

  QObject::connect(ui.ResetRangeToData, SIGNAL(clicked()), this, SLOT(resetRangeToData()));

  QObject::connect(ui.ResetRangeToCustom, SIGNAL(clicked()), this, SLOT(resetRangeToCustom()));

  QObject::connect(
    ui.ResetRangeToDataOverTime, SIGNAL(clicked()), this, SLOT(resetRangeToDataOverTime()));

  QObject::connect(
    ui.ResetRangeToVisibleData, SIGNAL(clicked()), this, SLOT(resetRangeToVisibleData()));

  QObject::connect(
    ui.InvertTransferFunctions, SIGNAL(clicked()), this, SLOT(invertTransferFunctions()));

  QObject::connect(ui.ChoosePreset, SIGNAL(clicked()), this, SLOT(choosePreset()));
  QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()), this, SLOT(saveAsPreset()));
  QObject::connect(
    ui.ComputeDataHistogram, SIGNAL(clicked()), this, SLOT(showDataHistogramClicked()));
  QObject::connect(ui.AdvancedButton, SIGNAL(clicked()), this, SLOT(updatePanel()));

  QObject::connect(
    ui.OpacityEditor, SIGNAL(chartRangeModified()), this, SLOT(setHistogramOutdated()));
  QObject::connect(ui.OpacityEditor, SIGNAL(chartRangeModified()), ui.OpacityEditor,
    SIGNAL(controlPointsModified()));
  QObject::connect(
    ui.ColorEditor, SIGNAL(chartRangeModified()), ui.ColorEditor, SIGNAL(controlPointsModified()));

  this->connect(
    ui.UseLogScaleOpacity, SIGNAL(clicked(bool)), SLOT(useLogScaleOpacityClicked(bool)));

  // if the user edits the "DataValue", we need to update the transfer function.
  QObject::connect(
    ui.CurrentDataValue, SIGNAL(textChangedAndEditingFinished()), this, SLOT(currentDataEdited()));

  vtkSMProperty* smproperty = smgroup->GetProperty("XRGBPoints");
  if (smproperty)
  {
    this->addPropertyLink(this, "xrgbPoints", SIGNAL(xrgbPointsChanged()), smproperty);
  }
  else
  {
    qCritical("Missing 'XRGBPoints' property. Widget may not function correctly.");
  }

  ui.OpacityEditor->hide();
  smproperty = smgroup->GetProperty("ScalarOpacityFunction");
  if (smproperty)
  {
    this->addPropertyLink(
      this, "scalarOpacityFunctionProxy", SIGNAL(scalarOpacityFunctionProxyChanged()), smproperty);
  }

  smproperty = smgroup->GetProperty("EnableOpacityMapping");
  if (smproperty)
  {
    this->addPropertyLink(ui.EnableOpacityMapping, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.EnableOpacityMapping->hide();
    ui.UseLogScaleOpacity->hide();
  }

  smproperty = smgroup->GetProperty("UseLogScale");
  if (smproperty)
  {
    this->addPropertyLink(this, "useLogScale", SIGNAL(useLogScaleChanged()), smproperty);
    QObject::connect(ui.UseLogScale, SIGNAL(clicked(bool)), this, SLOT(useLogScaleClicked(bool)));
  }
  else
  {
    ui.UseLogScale->hide();
  }

  smproperty = smgroup->GetProperty("UseOpacityControlPointsFreehandDrawing");
  if (smproperty)
  {
    this->addPropertyLink(this, "useOpacityControlPointsFreehandDrawing",
      SIGNAL(useOpacityControlPointsFreehandDrawingChanged()), smproperty);
    this->connect(ui.UseOpacityControlPointsFreehandDrawing, SIGNAL(clicked(bool)),
      SLOT(useOpacityControlPointsFreehandDrawingClicked(bool)));
  }
  else
  {
    ui.UseOpacityControlPointsFreehandDrawing->hide();
  }

  smproperty = smgroup->GetProperty("ShowDataHistogram");
  if (smproperty)
  {
    this->addPropertyLink(
      this, "showDataHistogram", SIGNAL(showDataHistogramChanged()), smproperty);
    QObject::connect(
      ui.ShowDataHistogram, SIGNAL(clicked(bool)), this, SLOT(showDataHistogramClicked(bool)));
  }
  else
  {
    ui.ShowDataHistogram->hide();
  }

  smproperty = smgroup->GetProperty("AutomaticDataHistogramComputation");
  if (smproperty)
  {
    this->addPropertyLink(this, "automaticDataHistogramComputation",
      SIGNAL(automaticDataHistogramComputationChanged()), smproperty);
    QObject::connect(ui.AutomaticDataHistogramComputation, SIGNAL(clicked(bool)), this,
      SLOT(automaticDataHistogramComputationClicked(bool)));
  }
  else
  {
    ui.AutomaticDataHistogramComputation->hide();
  }

  smproperty = smgroup->GetProperty("DataHistogramNumberOfBins");
  if (smproperty)
  {
    this->addPropertyLink(
      this, "dataHistogramNumberOfBins", SIGNAL(dataHistogramNumberOfBinsEdited()), smproperty);
    QObject::connect(ui.DataHistogramNumberOfBins, SIGNAL(valueEdited(int)), this,
      SLOT(dataHistogramNumberOfBinsEdited(int)));
  }
  else
  {
    ui.DataHistogramNumberOfBins->hide();
  }

  // Manage histogram computation if enabled
  // When creating the widget, we consider that the cost of recomputing the histogram table
  // can be paid systematically
  // We hide it to avoid seeing it before the timer ends and triggers the actual computation
  this->updateDataHistogramEnableState();
  this->Internals->Ui.OpacityEditor->setVisible(!ui.ShowDataHistogram->isChecked());
  this->showDataHistogramClicked(ui.ShowDataHistogram->isChecked());

  // if proxy has a property named IndexedLookup, we hide this entire widget
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
  {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->TransferFunctionConnector->Connect(smproxy->GetProperty("IndexedLookup"),
      vtkCommand::ModifiedEvent, this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be hidden when IndexedLookup is ON.
    this->addDecorator(this->Internals->Decorator);
  }

  if (smproxy->GetProperty("VectorMode"))
  {
    this->Internals->TransferFunctionConnector->Connect(smproxy->GetProperty("VectorMode"),
      vtkCommand::ModifiedEvent, this, SLOT(setHistogramOutdated()));
  }
  if (smproxy->GetProperty("VectorComponent"))
  {
    this->Internals->TransferFunctionConnector->Connect(smproxy->GetProperty("VectorComponent"),
      vtkCommand::ModifiedEvent, this, SLOT(setHistogramOutdated()));
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings)
  {
    this->Internals->Ui.AdvancedButton->setChecked(
      settings->value("showAdvancedPropertiesColorOpacityEditorWidget", false).toBool());
  }

  this->updateCurrentData();
  this->updatePanel();
}

//-----------------------------------------------------------------------------
pqColorOpacityEditorWidget::~pqColorOpacityEditorWidget()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings)
  {
    // save the state of the advanced button in the widget
    settings->setValue("showAdvancedPropertiesColorOpacityEditorWidget",
      this->Internals->Ui.AdvancedButton->isChecked());
  }

  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setScalarOpacityFunctionProxy(pqSMProxy sofProxy)
{
  pqInternals& internals = (*this->Internals);
  Ui::ColorOpacityEditorWidget& ui = internals.Ui;

  vtkSMProxy* newSofProxy = nullptr;
  vtkPiecewiseFunction* pwf =
    sofProxy ? vtkPiecewiseFunction::SafeDownCast(sofProxy->GetClientSideObject()) : nullptr;
  if (sofProxy && sofProxy->GetProperty("Points") && pwf)
  {
    newSofProxy = sofProxy;
  }
  if (internals.ScalarOpacityFunctionProxy == newSofProxy)
  {
    return;
  }
  if (internals.ScalarOpacityFunctionProxy)
  {
    // cleanup old property links.
    this->links().removePropertyLink(this, "xvmsPoints", SIGNAL(xvmsPointsChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("Points"));
    this->links().removePropertyLink(this, "useLogScaleOpacity",
      SIGNAL(useLogScaleOpacityChanged()), internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("UseLogScale"));
  }
  internals.ScalarOpacityFunctionProxy = newSofProxy;
  if (internals.ScalarOpacityFunctionProxy)
  {
    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());

    // When representation changes, we have to initialize the opacity widget when
    // "MultiComponentsMapping" is modified
    this->Internals->RangeConnector->Disconnect();
    vtkSMProperty* msProp = proxy->GetProperty("MapScalars");
    vtkSMProperty* mcmProp = proxy->GetProperty("MultiComponentsMapping");
    vtkSMProperty* uoaProperty = proxy->GetProperty("UseSeparateOpacityArray");
    if (msProp && (mcmProp || uoaProperty))
    {
      this->Internals->RangeConnector->Connect(msProp, vtkCommand::ModifiedEvent, this,
        SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

      if (mcmProp)
      {
        this->Internals->RangeConnector->Connect(mcmProp, vtkCommand::ModifiedEvent, this,
          SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);
      }

      if (uoaProperty)
      {
        this->Internals->RangeConnector->Connect(uoaProperty, vtkCommand::ModifiedEvent, this,
          SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);
      }
    }

    this->initializeOpacityEditor(pwf);

    // add new property links.
    this->links().addPropertyLink(this, "xvmsPoints", SIGNAL(xvmsPointsChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("Points"));
    this->links().addPropertyLink(this, "useLogScaleOpacity", SIGNAL(useLogScaleOpacityChanged()),
      internals.ScalarOpacityFunctionProxy,
      internals.ScalarOpacityFunctionProxy->GetProperty("UseLogScale"));
  }
  ui.OpacityEditor->setVisible(newSofProxy != nullptr);
}

//-----------------------------------------------------------------------------
pqSMProxy pqColorOpacityEditorWidget::scalarOpacityFunctionProxy() const
{
  return this->Internals->ScalarOpacityFunctionProxy.GetPointer();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::updateIndexedLookupState()
{
  if (this->proxy()->GetProperty("IndexedLookup"))
  {
    bool val = vtkSMPropertyHelper(this->proxy(), "IndexedLookup").GetAsInt() != 0;
    this->Internals->Decorator->setHidden(val);
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::multiComponentsMappingChanged(vtkObject* vtkNotUsed(sender),
  unsigned long vtkNotUsed(event), void* clientData, void* vtkNotUsed(callData))
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());

  if (proxy->GetVolumeIndependentRanges())
  {
    // force separate color map
    vtkSMProperty* separateProperty = proxy->GetProperty("UseSeparateColorMap");
    bool sepEnabled = vtkSMPropertyHelper(separateProperty).GetAsInt() != 0;
    if (!sepEnabled)
    {
      vtkSMPropertyHelper(separateProperty).Set(1);
      vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
      proxy->SetScalarColoring(helper.GetAsString(4), vtkDataObject::POINT);
      proxy->RescaleTransferFunctionToDataRange();
      return;
    }
  }

  this->initializeOpacityEditor(static_cast<vtkPiecewiseFunction*>(clientData));
  proxy->RescaleTransferFunctionToDataRange();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::initializeOpacityEditor(vtkPiecewiseFunction* pwf)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  vtkSMPVRepresentationProxy* proxy = static_cast<vtkSMPVRepresentationProxy*>(repr->getProxy());
  vtkScalarsToColors* stc = nullptr;
  vtkSMProperty* separateProperty = proxy->GetProperty("UseSeparateColorMap");
  bool sepEnabled = vtkSMPropertyHelper(separateProperty).GetAsInt() != 0;
  if (!proxy->GetVolumeIndependentRanges() || !sepEnabled)
  {
    stc = vtkScalarsToColors::SafeDownCast(this->proxy()->GetClientSideObject());
  }
  ui.OpacityEditor->initialize(stc, false, pwf, true);

  // The opacity editor has been initialized, set the data histogram table if needed
  this->showDataHistogramClicked(this->Internals->Ui.ShowDataHistogram->isChecked());
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::opacityCurrentChanged(vtkIdType index)
{
  if (index != -1)
  {
    Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
    ui.ColorEditor->setCurrentPoint(-1);
  }
  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::colorCurrentChanged(vtkIdType index)
{
  if (index != -1)
  {
    Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
    ui.OpacityEditor->setCurrentPoint(-1);
  }
  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::updatePanel()
{
  if (this->Internals)
  {
    bool advancedVisible = this->Internals->Ui.AdvancedButton->isChecked();
    this->Internals->Ui.ColorLabel->setVisible(advancedVisible);
    this->Internals->Ui.ColorTable->setVisible(advancedVisible);
    this->Internals->Ui.OpacityLabel->setVisible(advancedVisible);
    this->Internals->Ui.OpacityTable->setVisible(advancedVisible);
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::updateCurrentData()
{
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->proxy()->GetClientSideObject());
  vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf =
    pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : nullptr;

  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  if (ui.ColorEditor->currentPoint() >= 0 && stc)
  {
    double xrgbms[6];
    stc->GetNodeValue(ui.ColorEditor->currentPoint(), xrgbms);
    ui.CurrentDataValue->setText(pqCoreUtilities::number(xrgbms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(ui.ColorEditor->currentPoint() != 0 &&
      ui.ColorEditor->currentPoint() != (ui.ColorEditor->numberOfControlPoints() - 1));
  }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
  {
    double xvms[4];
    pwf->GetNodeValue(ui.OpacityEditor->currentPoint(), xvms);
    ui.CurrentDataValue->setText(pqCoreUtilities::number(xvms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(ui.OpacityEditor->currentPoint() != 0 &&
      ui.OpacityEditor->currentPoint() != (ui.OpacityEditor->numberOfControlPoints() - 1));
  }
  else
  {
    ui.CurrentDataValue->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorOpacityEditorWidget::xrgbPoints() const
{
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->proxy()->GetClientSideObject());
  QList<QVariant> values;
  for (int cc = 0; stc != nullptr && cc < stc->GetSize(); cc++)
  {
    double xrgbms[6];
    stc->GetNodeValue(cc, xrgbms);
    values.push_back(xrgbms[0]);
    values.push_back(xrgbms[1]);
    values.push_back(xrgbms[2]);
    values.push_back(xrgbms[3]);
  }

  return values;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorOpacityEditorWidget::xvmsPoints() const
{
  vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf =
    pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : nullptr;

  QList<QVariant> values;
  for (int cc = 0; pwf != nullptr && cc < pwf->GetSize(); cc++)
  {
    double xvms[4];
    pwf->GetNodeValue(cc, xvms);
    values.push_back(xvms[0]);
    values.push_back(xvms[1]);
    values.push_back(xvms[2]);
    values.push_back(xvms[3]);
  }
  return values;
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::useLogScale() const
{
  return this->Internals->Ui.UseLogScale->isChecked();
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::useLogScaleOpacity() const
{
  return this->Internals->Ui.UseLogScaleOpacity->isChecked();
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::useOpacityControlPointsFreehandDrawing() const
{
  return this->Internals->Ui.UseOpacityControlPointsFreehandDrawing->isChecked();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setUseLogScale(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.UseLogScale->setChecked(val);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setUseLogScaleOpacity(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.UseLogScaleOpacity->setChecked(val);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setUseOpacityControlPointsFreehandDrawing(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.UseOpacityControlPointsFreehandDrawing->setChecked(val);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useLogScaleClicked(bool log_space)
{
  if (log_space)
  {
    // Make sure both color and opacity are remapped if needed:
    this->prepareRangeForLogScaling();
    vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(this->proxy());
  }
  else
  {
    vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(this->proxy());
  }

  this->Internals->Ui.ColorEditor->SetLogScaleXAxis(log_space);

  Q_EMIT this->useLogScaleChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useOpacityControlPointsFreehandDrawingClicked(bool use)
{
  this->Internals->Ui.OpacityEditor->SetControlPointsFreehandDrawing(use);
  Q_EMIT this->useOpacityControlPointsFreehandDrawingChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useLogScaleOpacityClicked(bool log_space)
{
  vtkSMProxy* opacityProxy = this->Internals->ScalarOpacityFunctionProxy;
  if (log_space)
  {
    // Make sure both color and opacity are remapped if needed:
    this->prepareRangeForLogScaling();
    vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(opacityProxy);
  }
  else
  {
    vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(opacityProxy);
  }

  this->Internals->Ui.OpacityEditor->SetLogScaleXAxis(log_space);

  Q_EMIT this->useLogScaleOpacityChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setXvmsPoints(const QList<QVariant>& values)
{
  Q_UNUSED(values);
  // Since the vtkPiecewiseFunction connected to the widget is directly obtained
  // from the proxy, we don't need to do anything here. The widget will be
  // updated when the proxy updates.
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setXrgbPoints(const QList<QVariant>& values)
{
  Q_UNUSED(values);
  // Since the vtkColorTransferFunction connected to the widget is directly obtained
  // from the proxy, we don't need to do anything here. The widget will be
  // updated when the proxy updates.
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::currentDataEdited()
{
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->proxy()->GetClientSideObject());
  vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf =
    pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : nullptr;

  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  if (ui.ColorEditor->currentPoint() >= 0 && stc)
  {
    ui.ColorEditor->setCurrentPointPosition(ui.CurrentDataValue->text().toDouble());
  }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
  {
    ui.OpacityEditor->setCurrentPointPosition(ui.CurrentDataValue->text().toDouble());
  }

  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::representationOrViewChanged()
{
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  bool hasRepresentation = repr != nullptr;
  pqView* activeView = pqActiveObjects::instance().activeView();
  bool hasView = activeView != nullptr;

  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.ResetRangeToData->setEnabled(hasRepresentation);
  ui.ResetRangeToDataOverTime->setEnabled(hasRepresentation);
  ui.ResetRangeToVisibleData->setEnabled(hasRepresentation && hasView);

  vtkSMProxy* pwfProxy = this->scalarOpacityFunctionProxy();
  vtkPiecewiseFunction* pwf =
    pwfProxy ? vtkPiecewiseFunction::SafeDownCast(pwfProxy->GetClientSideObject()) : nullptr;

  // When representation changes, we have to initialize the opacity widget when
  // "MultiComponentsMapping" is modified
  this->Internals->RangeConnector->Disconnect();
  vtkSMProperty* msProp = repr->getProxy()->GetProperty("MapScalars");
  vtkSMProperty* mcmProp = repr->getProxy()->GetProperty("MultiComponentsMapping");
  vtkSMProperty* uoaProp = repr->getProxy()->GetProperty("UseSeparateOpacityArray");
  if (msProp && (mcmProp || uoaProp))
  {
    this->Internals->RangeConnector->Connect(msProp, vtkCommand::ModifiedEvent, this,
      SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);

    if (mcmProp)
    {
      this->Internals->RangeConnector->Connect(mcmProp, vtkCommand::ModifiedEvent, this,
        SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);
    }

    if (uoaProp)
    {
      this->Internals->RangeConnector->Connect(uoaProp, vtkCommand::ModifiedEvent, this,
        SLOT(multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*)), pwf);
    }
  }
  this->initializeOpacityEditor(pwf);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::prepareRangeForLogScaling()
{
  vtkSMProxy* colorProxy = this->proxy();
  double range[2];

  vtkSMTransferFunctionProxy::GetRange(colorProxy, range);

  if (vtkSMCoreUtilities::AdjustRangeForLog(range))
  {
    vtkSMProxy* opacityProxy = this->Internals->ScalarOpacityFunctionProxy;

    vtkGenericWarningMacro("Ranges not valid for log-space. Changed the range to ("
      << range[0] << ", " << range[1] << ").");

    vtkSMTransferFunctionProxy::RescaleTransferFunction(colorProxy, range);
    vtkSMTransferFunctionProxy::RescaleTransferFunction(opacityProxy, range);
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToData()
{
  // passing in nullptr ensure pqResetScalarRangeReaction simply uses active representation.
  if (pqResetScalarRangeReaction::resetScalarRangeToData(nullptr))
  {
    this->Internals->render();
    Q_EMIT this->changeFinished();
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToDataOverTime()
{
  // passing in nullptr ensure pqResetScalarRangeReaction simply uses active representation.
  if (pqResetScalarRangeReaction::resetScalarRangeToDataOverTime(nullptr))
  {
    this->Internals->render();
    Q_EMIT this->changeFinished();
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToVisibleData()
{
  pqPipelineRepresentation* repr =
    qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
  if (!repr)
  {
    qCritical() << "No active representation.";
    return;
  }

  vtkSMPVRepresentationProxy* repProxy = vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!repProxy)
  {
    return;
  }

  pqView* activeView = pqActiveObjects::instance().activeView();
  if (!activeView)
  {
    qCritical() << "No active view.";
    return;
  }

  vtkSMRenderViewProxy* rvproxy = vtkSMRenderViewProxy::SafeDownCast(activeView->getViewProxy());
  if (!rvproxy)
  {
    return;
  }

  BEGIN_UNDO_SET("Reset transfer function ranges using visible data");
  vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(repProxy, rvproxy);
  this->Internals->render();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToCustom()
{
  bool changed = false;
  pqPipelineRepresentation* repr =
    qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
  if (repr)
  {
    changed = pqResetScalarRangeReaction::resetScalarRangeToCustom(repr);
  }
  else
  {
    // Shouldn't happen, but fall back to the active lut if there is no active representation
    changed = pqResetScalarRangeReaction::resetScalarRangeToCustom(this->proxy());
  }

  if (changed)
  {
    this->Internals->render();
    Q_EMIT this->changeFinished();
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::invertTransferFunctions()
{
  BEGIN_UNDO_SET("Invert transfer function");
  vtkSMTransferFunctionProxy::InvertTransferFunction(this->proxy());

  Q_EMIT this->changeFinished();
  // We don't invert the opacity function, for now.
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::choosePreset(const char* presetName)
{
  this->Internals->ChoosePresetReaction->setTransferFunction(this->proxy());
  this->Internals->ChoosePresetReaction->choosePreset(presetName);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::presetApplied()
{
  Q_EMIT this->changeFinished();

  // Assume the color map and opacity have changed and refresh
  Q_EMIT this->xrgbPointsChanged();
  Q_EMIT this->xvmsPointsChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::saveAsPreset()
{
  QDialog dialog(this);
  Ui::SavePresetOptions ui;
  ui.setupUi(&dialog);
  ui.saveOpacities->setEnabled(this->scalarOpacityFunctionProxy() != nullptr);
  ui.saveOpacities->setChecked(ui.saveOpacities->isEnabled());
  ui.saveAnnotations->setVisible(false);

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  assert(ui.saveColors->isChecked());
  Json::Value preset = vtkSMTransferFunctionProxy::GetStateAsPreset(this->proxy());

  if (ui.saveOpacities->isChecked())
  {
    Json::Value opacities =
      vtkSMTransferFunctionProxy::GetStateAsPreset(this->scalarOpacityFunctionProxy());
    if (opacities.isMember("Points"))
    {
      preset["Points"] = opacities["Points"];
    }
  }

  std::string presetName;
  auto presets = vtkSMTransferFunctionPresets::GetInstance();
  presetName = presets->AddUniquePreset(preset, qPrintable(ui.presetName->text()));
  this->choosePreset(presetName.c_str());
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::showDataHistogram() const
{
  return this->Internals->Ui.ShowDataHistogram->isChecked();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setShowDataHistogram(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.ShowDataHistogram->setChecked(val);
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::automaticDataHistogramComputation() const
{
  return this->Internals->Ui.AutomaticDataHistogramComputation->isChecked();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setAutomaticDataHistogramComputation(bool val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.AutomaticDataHistogramComputation->setChecked(val);
}

//-----------------------------------------------------------------------------
int pqColorOpacityEditorWidget::dataHistogramNumberOfBins() const
{
  return this->Internals->Ui.DataHistogramNumberOfBins->value();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setDataHistogramNumberOfBins(int val)
{
  Ui::ColorOpacityEditorWidget& ui = this->Internals->Ui;
  ui.DataHistogramNumberOfBins->setValue(val);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::showDataHistogramClicked(bool showDataHistogram)
{
  this->updateDataHistogramEnableState();
  if (showDataHistogram)
  {
    // Defer the histogram computation for later to ensure all visible consumer
    // have their data available
    this->Internals->HistogramTimer.start();
  }
  else
  {
    this->Internals->Ui.OpacityEditor->setHistogramTable(nullptr);
  }
  Q_EMIT this->showDataHistogramChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::realShowDataHistogram()
{
  // the opacity editor may have been hidden before this call, make sure it is visible.
  this->Internals->Ui.OpacityEditor->show();

  vtkTable* histoTable = vtkSMTransferFunctionProxy::GetHistogramTableCache(this->proxy());
  if (!histoTable || this->Internals->HistogramOutdated)
  {
    // No cache or we are outdated, compute the histogram
    this->Internals->Ui.ComputeDataHistogram->clear();
    vtkSMTransferFunctionProxy* tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(this->proxy());
    histoTable =
      tfProxy->ComputeDataHistogramTable(this->Internals->Ui.DataHistogramNumberOfBins->value());
    this->Internals->Ui.OpacityEditor->setHistogramTable(histoTable);

    // Add all consumers, even non-visible, to the consumer connnector
    // so the histogram can be set outdated correctly
    this->Internals->ConsumerConnector->Disconnect();
    std::set<vtkSMProxy*> usedProxy;
    for (unsigned int cc = 0, max = tfProxy->GetNumberOfConsumers(); cc < max; ++cc)
    {
      vtkSMProxy* proxy = tfProxy->GetConsumerProxy(cc);
      proxy = proxy ? proxy->GetTrueParentProxy() : nullptr;
      vtkSMPVRepresentationProxy* consumer = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
      if (consumer && usedProxy.find(consumer) == usedProxy.end())
      {
        this->Internals->ConsumerConnector->Connect(consumer->GetProperty("Visibility"),
          vtkCommand::ModifiedEvent, this, SLOT(setHistogramOutdated()));
        usedProxy.insert(consumer);
      }
    }
  }
  this->Internals->Ui.OpacityEditor->setHistogramTable(histoTable);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::automaticDataHistogramComputationClicked(bool val)
{
  if (val)
  {
    this->showDataHistogramClicked(true);
  }
  this->updateDataHistogramEnableState();
  Q_EMIT this->automaticDataHistogramComputationChanged();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::dataHistogramNumberOfBinsEdited(int vtkNotUsed(val))
{
  this->setHistogramOutdated();
  Q_EMIT this->dataHistogramNumberOfBinsEdited();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setHistogramOutdated()
{
  this->Internals->HistogramOutdated = true;
  if (this->Internals->Ui.AutomaticDataHistogramComputation->isChecked())
  {
    this->showDataHistogramClicked(this->showDataHistogram());
  }
  else
  {
    this->Internals->Ui.ComputeDataHistogram->highlight();
  }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::updateDataHistogramEnableState()
{
  bool showDataHistogram = this->Internals->Ui.ShowDataHistogram->isChecked();
  this->Internals->Ui.AutomaticDataHistogramComputation->setEnabled(showDataHistogram);
  this->Internals->Ui.DataHistogramNumberOfBins->setEnabled(showDataHistogram);
  this->Internals->Ui.ComputeDataHistogram->setEnabled(
    showDataHistogram && !this->Internals->Ui.AutomaticDataHistogramComputation->isChecked());
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::onRangeHandlesRangeChanged(double rangeMin, double rangeMax)
{
  vtkSMProxy* colorProxy = this->proxy();
  vtkSMProxy* opacityProxy = this->Internals->ScalarOpacityFunctionProxy;
  double range[2] = { rangeMin, rangeMax };

  vtkSMTransferFunctionProxy::RescaleTransferFunction(colorProxy, range);
  vtkSMTransferFunctionProxy::RescaleTransferFunction(opacityProxy, range);
  this->Internals->render();
  Q_EMIT this->changeFinished();
}
