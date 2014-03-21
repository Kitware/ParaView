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

#include "pqActiveObjects.h"
#include "pqColorPresetManager.h"
#include "pqColorTableModel.h"
#include "pqDataRepresentation.h"
#include "pqOpacityTableModel.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqRescaleRange.h"
#include "pqSettings.h"
#include "pqTransferFunctionWidget.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkVector.h"
#include "vtkWeakPointer.h"

#include <QDoubleValidator>
#include <QMessageBox>
#include <QPointer>
#include <QtDebug>
#include <QTimer>
#include <QVBoxLayout>

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
    : Superclass(xmlArg, parentArg), Hidden(false)
    {
    }
  virtual ~pqColorOpacityEditorWidgetDecorator()
    {
    }

  void setHidden(bool val)
    {
    if (val != this->Hidden) { this->Hidden = val; emit this->visibilityChanged(); }
    }
  virtual bool canShowWidget(bool show_advanced) const
    {
    Q_UNUSED(show_advanced);
    return !this->Hidden;
    }

private:
  Q_DISABLE_COPY(pqColorOpacityEditorWidgetDecorator);
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

  // We use this pqPropertyLinks instance to simply monitor smproperty changes.
  pqPropertyLinks LinksForMonitoringChanges;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;

  pqInternals(pqColorOpacityEditorWidget* self, vtkSMPropertyGroup* group)
    : ColorTableModel(self), OpacityTableModel(self), PropertyGroup(group)
    {
    this->Ui.setupUi(self);
    this->Ui.CurrentDataValue->setValidator(new QDoubleValidator(self));
    this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    //this->Ui.mainLayout->setSpacing(
    //  pqPropertiesPanel::suggestedVerticalSpacing());

    this->Decorator = new pqColorOpacityEditorWidgetDecorator(NULL, self);

    this->Ui.ColorTable->setModel(&this->ColorTableModel);
    this->Ui.ColorTable->horizontalHeader()->setHighlightSections(false);
#if QT_VERSION >= 0x050000
    this->Ui.ColorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    this->Ui.ColorTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
    this->Ui.ColorTable->horizontalHeader()->setStretchLastSection(true);

    this->Ui.OpacityTable->setModel(&this->OpacityTableModel);
    this->Ui.OpacityTable->horizontalHeader()->setHighlightSections(false);
#if QT_VERSION >= 0x050000
    this->Ui.OpacityTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    this->Ui.OpacityTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
    this->Ui.OpacityTable->horizontalHeader()->setStretchLastSection(true);
    }
};

//-----------------------------------------------------------------------------
pqColorOpacityEditorWidget::pqColorOpacityEditorWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject),
  Internals(new pqInternals(this, smgroup))
{
  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());

  vtkPiecewiseFunction* pwf = stc? stc->GetScalarOpacityFunction() : NULL;
  if (pwf)
    {
    ui.OpacityEditor->initialize(stc, false, pwf, true);
    QObject::connect(
      &this->Internals->OpacityTableModel,
      SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      this, SIGNAL(xvmsPointsChanged()));
    }
  else
    {
    ui.OpacityEditor->hide();
    }
  if (stc)
    {
    ui.ColorEditor->initialize(stc, true, NULL, false);
    QObject::connect(
      &this->Internals->ColorTableModel,
      SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      this, SIGNAL(xrgbPointsChanged()));
    }

  QObject::connect(
    ui.OpacityEditor, SIGNAL(currentPointChanged(vtkIdType)),
    this, SLOT(opacityCurrentChanged(vtkIdType)));
  QObject::connect(
    ui.ColorEditor, SIGNAL(currentPointChanged(vtkIdType)),
    this, SLOT(colorCurrentChanged(vtkIdType)));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()),
    this, SIGNAL(xrgbPointsChanged()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()),
    this, SIGNAL(xvmsPointsChanged()));

  QObject::connect(
    ui.ColorEditor, SIGNAL(controlPointsModified()),
    this, SLOT(updateCurrentData()));
  QObject::connect(
    ui.OpacityEditor, SIGNAL(controlPointsModified()),
    this, SLOT(updateCurrentData()));

  QObject::connect(
    ui.ResetRangeToData, SIGNAL(clicked()),
    this, SLOT(resetRangeToData()));

  QObject::connect(
    ui.ResetRangeToCustom, SIGNAL(clicked()),
    this, SLOT(resetRangeToCustom()));

  QObject::connect(
    ui.ResetRangeToDataOverTime, SIGNAL(clicked()),
    this, SLOT(resetRangeToDataOverTime()));

  QObject::connect(
    ui.InvertTransferFunctions, SIGNAL(clicked()),
    this, SLOT(invertTransferFunctions()));

  QObject::connect(ui.ChoosePreset, SIGNAL(clicked()),
    this, SLOT(choosePreset()));
  QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()),
    this, SLOT(saveAsPreset()));
  QObject::connect(ui.AdvancedButton, SIGNAL(clicked()),
    this, SLOT(updatePanel()));

  // if the user edits the "DataValue", we need to update the transfer function.
  QObject::connect(ui.CurrentDataValue, SIGNAL(textChangedAndEditingFinished()),
    this, SLOT(currentDataEdited()));

  vtkSMProperty* smproperty = smgroup->GetProperty("XRGBPoints");
  if (smproperty)
    {
    this->addPropertyLink(
      this, "xrgbPoints", SIGNAL(xrgbPointsChanged()), smproperty);
    }
  else
    {
    qCritical("Missing 'XRGBPoints' property. Widget may not function correctly.");
    }

  smproperty = smgroup->GetProperty("ScalarOpacityFunction");
  if (smproperty)
    {
    // TODO: T
    vtkSMProxy* pwfProxy = vtkSMPropertyHelper(smproperty).GetAsProxy();
    if (pwfProxy && pwfProxy->GetProperty("Points"))
      {
      this->addPropertyLink(
        this, "xvmsPoints", SIGNAL(xvmsPointsChanged()),
        pwfProxy, pwfProxy->GetProperty("Points"));
      }
    else
      {
      ui.OpacityEditor->hide();
      }
    }
  else
    {
    ui.OpacityEditor->hide();
    }

  smproperty = smgroup->GetProperty("EnableOpacityMapping");
  if (smproperty)
    {
    this->addPropertyLink(
      ui.EnableOpacityMapping, "checked", SIGNAL(toggled(bool)), smproperty);
    }
  else
    {
    ui.EnableOpacityMapping->hide();
    }

  smproperty = smgroup->GetProperty("UseLogScale");
  if (smproperty)
    {
    this->addPropertyLink(
      this, "useLogScale", SIGNAL(useLogScaleChanged()), smproperty);
    QObject::connect(ui.UseLogScale, SIGNAL(clicked(bool)),
      this, SLOT(useLogScaleClicked(bool)));
    // QObject::connect(ui.UseLogScale, SIGNAL(toggled(bool)),
    //  this, SIGNAL(useLogScaleChanged()));
    }
  else
    {
    ui.UseLogScale->hide();
    }

  smproperty = smgroup->GetProperty("LockScalarRange");
  if (smproperty)
    {
    this->addPropertyLink(
      this, "lockScalarRange", SIGNAL(lockScalarRangeChanged()), smproperty);
    QObject::connect(ui.AutoRescaleRange, SIGNAL(toggled(bool)),
      this, SIGNAL(lockScalarRangeChanged()));
    }
  else
    {
    ui.AutoRescaleRange->hide();
    }

  // if proxy has a property named IndexedLookup, we hide this entire widget
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
    {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->VTKConnector->Connect(
      smproxy->GetProperty("IndexedLookup"), vtkCommand::ModifiedEvent,
      this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be hidden when IndexedLookup is ON.
    this->addDecorator(this->Internals->Decorator);
    }

  pqSettings *settings = pqApplicationCore::instance()->settings();
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
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    // save the state of the advanced button in the widget
    settings->setValue("showAdvancedPropertiesColorOpacityEditorWidget",
                       this->Internals->Ui.AdvancedButton->isChecked());
    }

  delete this->Internals;
  this->Internals = NULL;
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
void pqColorOpacityEditorWidget::opacityCurrentChanged(vtkIdType index)
{
  if (index != -1)
    {
    Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
    ui.ColorEditor->setCurrentPoint(-1);
    }
  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::colorCurrentChanged(vtkIdType index)
{
  if (index != -1)
    {
    Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
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
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());
  vtkPiecewiseFunction* pwf = stc? stc->GetScalarOpacityFunction() : NULL;

  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  if (ui.ColorEditor->currentPoint() >= 0 && stc)
    {
    double xrgbms[6];
    stc->GetNodeValue(ui.ColorEditor->currentPoint(), xrgbms);
    ui.CurrentDataValue->setText(QString::number(xrgbms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(
      ui.ColorEditor->currentPoint() != 0 &&
      ui.ColorEditor->currentPoint() !=
      (ui.ColorEditor->numberOfControlPoints()-1));
    }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
    {
    double xvms[4];
    pwf->GetNodeValue(ui.OpacityEditor->currentPoint(), xvms);
    ui.CurrentDataValue->setText(QString::number(xvms[0]));

    // Don't enable widget for first/last control point. For those, users must
    // rescale the transfer function manually
    ui.CurrentDataValue->setEnabled(
      ui.OpacityEditor->currentPoint() != 0 &&
      ui.OpacityEditor->currentPoint() !=
      (ui.OpacityEditor->numberOfControlPoints()-1));
    }
  else
    {
    ui.CurrentDataValue->setEnabled(false);
    }

  this->Internals->ColorTableModel.refresh();
  this->Internals->OpacityTableModel.refresh();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorOpacityEditorWidget::xrgbPoints() const
{
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());
  QList<QVariant> values;
  for (int cc=0; stc != NULL && cc < stc->GetSize(); cc++)
    {
    double xrgbms[6];
    stc->GetNodeValue(cc, xrgbms);
    vtkVector<double, 4> value;
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
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());
  vtkPiecewiseFunction* pwf = stc? stc->GetScalarOpacityFunction() : NULL;

  QList<QVariant> values;
  for (int cc=0; pwf != NULL && cc < pwf->GetSize(); cc++)
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
void pqColorOpacityEditorWidget::setUseLogScale(bool val)
{
  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  ui.UseLogScale->setChecked(val);
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::useLogScaleClicked(bool log_space)
{
  if (log_space)
    {
    vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(this->proxy());
    }
  else
    {
    vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(this->proxy());
    }

  // FIXME: ensure scalar range is valid.

  emit this->useLogScaleChanged();
}

//-----------------------------------------------------------------------------
bool pqColorOpacityEditorWidget::lockScalarRange() const
{
  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  return !ui.AutoRescaleRange->isChecked();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::setLockScalarRange(bool val)
{
  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  ui.AutoRescaleRange->setChecked(!val);
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
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());
  vtkPiecewiseFunction* pwf = stc? stc->GetScalarOpacityFunction() : NULL;

  Ui::ColorOpacityEditorWidget &ui = this->Internals->Ui;
  if (ui.ColorEditor->currentPoint() >= 0 && stc)
    {
    ui.ColorEditor->setCurrentPointPosition(
      ui.CurrentDataValue->text().toDouble());
    }
  else if (ui.OpacityEditor->currentPoint() >= 0 && pwf)
    {
    ui.OpacityEditor->setCurrentPointPosition(
      ui.CurrentDataValue->text().toDouble());
    }

  this->updateCurrentData();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToData()
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  if (!repr)
    {
    qDebug("No active representation.");
    return;
    }
  BEGIN_UNDO_SET("Reset transfer function ranges using data range");
  vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(repr->getProxy());
  repr->renderViewEventually();
  emit this->changeFinished();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToDataOverTime()
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  if (!repr)
    {
    qDebug("No active representation.");
    return;
    }

  if (QMessageBox::warning(this,
      "Potentially slow operation",
      "This can potentially take a long time to complete. \n"
      "Are you sure you want to continue?",
      QMessageBox::Yes |QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
    BEGIN_UNDO_SET("Reset transfer function ranges using temporal data range");
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(repr->getProxy());

    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    this->setLockScalarRange(true);
    repr->renderViewEventually();
    emit this->changeFinished();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToCustom()
{
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      this->proxy()->GetClientSideObject());
  double range[2];
  stc->GetRange(range);

  pqRescaleRange dialog(this);
  dialog.setRange(range[0], range[1]);
  if (dialog.exec() == QDialog::Accepted)
    {
    this->resetRangeToCustom(dialog.getMinimum(), dialog.getMaximum());
    }
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::resetRangeToCustom(double min, double max)
{
  BEGIN_UNDO_SET("Reset transfer function ranges");
  vtkSMTransferFunctionProxy::RescaleTransferFunction(this->proxy(), min, max);
  vtkSMTransferFunctionProxy::RescaleTransferFunction(
    vtkSMPropertyHelper(this->proxy(), "ScalarOpacityFunction").GetAsProxy(),
    min, max);
  // disable auto-rescale of transfer function since the user has set on
  // explicitly (BUG #14371).
  this->setLockScalarRange(true);
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  repr->renderViewEventually();
  emit this->changeFinished();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::invertTransferFunctions()
{
  BEGIN_UNDO_SET("Invert transfer function");
  vtkSMTransferFunctionProxy::InvertTransferFunction(this->proxy());

  emit this->changeFinished();
  // We don't invert the opacity function, for now.
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::choosePreset(const pqColorMapModel* add_new/*=NULL*/)
{
  pqColorPresetManager preset(this,
    pqColorPresetManager::SHOW_NON_INDEXED_COLORS_ONLY);
  preset.setUsingCloseButton(true);
  preset.loadBuiltinColorPresets();
  preset.restoreSettings();
  if (add_new)
    {
    preset.addColorMap(*add_new, "New Color Preset");
    }

  QObject::connect(&preset, SIGNAL(currentChanged(const pqColorMapModel*)),
    this, SLOT(applyPreset(const pqColorMapModel*)));
  preset.exec();
  preset.saveSettings();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::applyPreset(const pqColorMapModel* preset)
{
  vtkNew<vtkPVXMLElement> xml;
  xml->SetName("ColorMap");
  if (pqColorPresetManager::saveColorMapToXML(preset, xml.GetPointer()))
    {
    BEGIN_UNDO_SET("Apply color preset");
    vtkSMTransferFunctionProxy::ApplyColorMap(this->proxy(), xml.GetPointer());
    emit this->changeFinished();
    END_UNDO_SET();
    }

  // Assume the color map and opacity have changed and refresh
  this->Internals->ColorTableModel.refresh();
  this->Internals->OpacityTableModel.refresh();
}

//-----------------------------------------------------------------------------
void pqColorOpacityEditorWidget::saveAsPreset()
{
  vtkNew<vtkPVXMLElement> xml;
  if (vtkSMTransferFunctionProxy::SaveColorMap(this->proxy(), xml.GetPointer()))
    {
    pqColorMapModel colorMap =
      pqColorPresetManager::createColorMapFromXML(xml.GetPointer());
    this->choosePreset(&colorMap);
    }
}
