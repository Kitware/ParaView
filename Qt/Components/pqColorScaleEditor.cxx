/*=========================================================================

   Program: ParaView
   Module:    pqColorScaleEditor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqColorScaleEditor.cxx
/// \date 2/14/2007

#include "pqColorScaleEditor.h"
#include "ui_pqColorScaleDialog.h"

#include "pqApplicationCore.h"
#include "pqChartValue.h"
#include "pqColorMapModel.h"
#include "pqColorPresetManager.h"
#include "pqColorPresetModel.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqLookupTableManager.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewBase.h"
#include "pqRescaleRange.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarOpacityFunction.h"
#include "pqScalarsToColors.h"
#ifdef FIXME
#include "pqScatterPlotRepresentation.h"
#endif
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqStandardColorLinkAdaptor.h"
#include "vtkColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkType.h"

#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QIntValidator>
#include <QItemSelectionModel>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QSpacerItem>
#include <QString>
#include <QtDebug>
#include <QVariant>




class pqColorScaleEditorForm : public Ui::pqColorScaleDialog
{
public:
  pqColorScaleEditorForm();
  ~pqColorScaleEditorForm() { }

  pqPropertyLinks Links; // used to link properties on the legend
  pqPropertyLinks ReprLinks; // used to link properties on the representation.
  pqSignalAdaptorColor *TitleColorAdaptor;
  pqSignalAdaptorColor *LabelColorAdaptor;
  pqStandardColorLinkAdaptor* TitleColorLink;
  pqStandardColorLinkAdaptor* LabelColorLink;
  pqSignalAdaptorComboBox *TitleFontAdaptor;
  pqSignalAdaptorComboBox *LabelFontAdaptor;
  vtkEventQtSlotConnect *Listener;
  pqColorPresetManager *Presets;
  QPointer<pqDataRepresentation> CurrentDisplay;
  int CurrentIndex;
  bool InSetColors;
  bool IgnoreEditor;
  bool IsDormant;
  bool MakingLegend;
};


//----------------------------------------------------------------------------
pqColorScaleEditorForm::pqColorScaleEditorForm()
  : Ui::pqColorScaleDialog(), Links(), CurrentDisplay(0)
{
  this->TitleColorAdaptor = 0;
  this->LabelColorAdaptor = 0;
  this->TitleColorLink = 0;
  this->LabelColorLink = 0;
  this->TitleFontAdaptor = 0;
  this->LabelFontAdaptor = 0;
  this->Listener = 0;
  this->Presets = 0;
  this->CurrentIndex = -1;
  this->InSetColors = false;
  this->IgnoreEditor = false;
  this->IsDormant = true;
  this->MakingLegend = false;
}

//----------------------------------------------------------------------------
pqColorScaleEditor::pqColorScaleEditor(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqColorScaleEditorForm();
  this->Viewer = vtkTransferFunctionViewer::New();
  this->Display = 0;
  this->ColorMap = 0;
  this->OpacityFunction = 0;
  this->Legend = 0;

  // Set up the ui.
  this->Form->setupUi(this);
  this->Form->Listener = vtkEventQtSlotConnect::New();
  this->Form->Presets = new pqColorPresetManager(this);
  this->Form->Presets->restoreSettings();
  this->Form->ColorScale->setToolTip("Note: Use Tab or Shift+Tab to navigate among points.");
  
  vtkRenderWindow *win = this->Form->ColorScale->GetRenderWindow();
  vtkRenderWindowInteractor *iren = this->Form->ColorScale->GetInteractor();
  this->Viewer->SetInteractor(iren);
  this->Viewer->SetRenderWindow(win);
  this->Viewer->SetTransferFunctionEditorType(vtkTransferFunctionViewer::SIMPLE_1D);
  this->Viewer->SetModificationTypeToColorAndOpacity();
  this->Viewer->SetWholeScalarRange(0.0, 1.0);
  this->Viewer->SetVisibleScalarRange(0.0, 1.0);
  this->Viewer->SetLockEndPoints(1);
  this->Viewer->SetShowColorFunctionInBackground(1);
  this->Viewer->SetShowColorFunctionOnLines(0);
  QColor col = this->palette().color(QPalette::Window);
  this->Viewer->SetBackgroundColor(col.redF(), col.greenF(), col.blueF());
  this->Viewer->SetLinesColor(0.0, 0.0, 0.0);

  this->Form->Listener->Connect(this->Viewer, vtkCommand::PickEvent,
      this, SLOT(changeCurrentColor()));
  this->Form->Listener->Connect(this->Viewer,
      vtkCommand::WidgetValueChangedEvent,
      this, SLOT(handleEditorPointMoved()));
  this->Form->Listener->Connect(this->Viewer, vtkCommand::EndInteractionEvent,
      this, SLOT(handleEditorPointMoveFinished()));
  this->Form->Listener->Connect(this->Viewer, vtkCommand::PlacePointEvent,
      this, SLOT(handleEditorAddOrDelete()));
  this->Form->Listener->Connect(this->Viewer, vtkCommand::WidgetModifiedEvent,
      this, SLOT(handleEditorCurrentChanged()));

  // Initialize the state of some of the controls.
  this->enableRescaleControls(this->Form->UseAutoRescale->isChecked());
  this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());

  this->enableLegendControls(this->Form->ShowColorLegend->isChecked());

  // Add the color space options to the combo box.
  this->Form->ColorSpace->addItem("RGB");
  this->Form->ColorSpace->addItem("HSV");
  this->Form->ColorSpace->addItem("Wrapped HSV");
  this->Form->ColorSpace->addItem("CIELAB");
  this->Form->ColorSpace->addItem("Diverging");

  // Add the color scale presets menu.
  this->loadBuiltinColorPresets();

  // Make sure the line edits only allow number inputs.
  this->Form->ScalarValue->setValidator(new QDoubleValidator(this));
  this->Form->Opacity->setValidator(new QDoubleValidator(this));
  this->Form->ScalarOpacityUnitDistance->setValidator(
    new QDoubleValidator(this));

  QIntValidator *intValidator = new QIntValidator(this);
  this->Form->TableSizeText->setValidator(intValidator);

  // Connect the color scale widgets.
  this->connect(this->Form->ScalarValue, SIGNAL(editingFinished()),
      this, SLOT(setValueFromText()));
  this->connect(this->Form->Opacity, SIGNAL(editingFinished()),
      this, SLOT(setOpacityFromText()));

  this->connect(this->Form->ColorSpace, SIGNAL(currentIndexChanged(int)),
      this, SLOT(setColorSpace(int)));

  this->connect(this->Form->NanColor,SIGNAL(chosenColorChanged(const QColor &)),
                this, SLOT(setNanColor(const QColor &)));

  this->connect(this->Form->SaveButton, SIGNAL(clicked()),
      this, SLOT(savePreset()));
  this->connect(this->Form->PresetButton, SIGNAL(clicked()),
      this, SLOT(loadPreset()));

  this->connect(this->Form->UseLogScale, SIGNAL(toggled(bool)),
      this, SLOT(setLogScale(bool)));

  this->connect(this->Form->UseAutoRescale, SIGNAL(toggled(bool)),
      this, SLOT(setAutoRescale(bool)));
  this->connect(this->Form->RescaleButton, SIGNAL(clicked()),
      this, SLOT(rescaleToNewRange()));
  this->connect(this->Form->RescaleToDataButton, SIGNAL(clicked()),
      this, SLOT(rescaleToDataRange()));
  this->connect(this->Form->RescaleToDataOverTimeButton, SIGNAL(clicked()),
      this, SLOT(rescaleToDataRangeOverTime()));

  this->connect(this->Form->UseDiscreteColors, SIGNAL(toggled(bool)),
      this, SLOT(setUseDiscreteColors(bool)));
  this->connect(this->Form->TableSize, SIGNAL(valueChanged(int)),
      this, SLOT(setSizeFromSlider(int)));
  this->connect(this->Form->TableSizeText, SIGNAL(editingFinished()),
      this, SLOT(setSizeFromText()));

  // Connect the color legend widgets.
  this->connect(this->Form->ShowColorLegend, SIGNAL(toggled(bool)),
      this, SLOT(setLegendVisibility(bool)));

  this->connect(this->Form->TitleName, SIGNAL(textChanged(const QString &)),
      this, SLOT(setLegendName(const QString &)));
  this->connect(this->Form->TitleComponent, SIGNAL(textChanged(const QString &)),
      this, SLOT(setLegendComponent(const QString &)));
  this->Form->TitleColorAdaptor = new pqSignalAdaptorColor(
      this->Form->TitleColorButton, "chosenColor", 
      SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Form->TitleFontAdaptor = new pqSignalAdaptorComboBox(
      this->Form->TitleFont);

  this->Form->LabelColorAdaptor = new pqSignalAdaptorColor(
      this->Form->LabelColorButton, "chosenColor", 
      SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Form->LabelFontAdaptor = new pqSignalAdaptorComboBox(
      this->Form->LabelFont);

  // Hook the close button up to the accept action.
  this->connect(this->Form->CloseButton, SIGNAL(clicked()),
      this, SLOT(accept()));

  //Hook up the MakeDefaultButton
  this->connect(this->Form->MakeDefaultButton, SIGNAL(clicked()),
      this, SLOT(makeDefault()));
}

pqColorScaleEditor::~pqColorScaleEditor()
{
  // Save the color map presets.
  this->Form->Presets->saveSettings();

  delete this->Form->LabelColorAdaptor;
  delete this->Form->TitleColorAdaptor;
  delete this->Form->LabelFontAdaptor;
  delete this->Form->TitleFontAdaptor;
  this->Form->Listener->Delete();
  delete this->Form;
  this->Viewer->Delete();
}

void pqColorScaleEditor::setRepresentation(pqDataRepresentation *display)
{
  this->Form->CurrentDisplay = display;
  if(this->Form->IsDormant)
    {
    return;
    }

  if(this->Display == display)
    {
    return;
    }

  this->setLegend(0);
  this->Form->ShowColorLegend->setEnabled(false);
  if(this->Display)
    {
    this->disconnect(this->Display, 0, this, 0);
    this->disconnect(&this->Form->Links, 0, this->Display, 0);
    this->disconnect(&this->Form->ReprLinks, 0, this->Display, 0);
    this->Form->ReprLinks.removeAllPropertyLinks();
    if(this->ColorMap)
      {
      this->disconnect(this->ColorMap, 0, this, 0);
      this->Form->Listener->Disconnect(
          this->ColorMap->getProxy()->GetProperty("RGBPoints"));
      }

    if(this->OpacityFunction)
      {
      this->Form->Listener->Disconnect(
          this->OpacityFunction->getProxy()->GetProperty("Points"));
      }
    }

  this->Display = display;
  this->ColorMap = 0;
  this->OpacityFunction = 0;
  if(this->Display)
    {
    this->connect(this->Display, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupDisplay()));
    this->connect(&this->Form->Links, SIGNAL(qtWidgetChanged()),
        this->Display, SLOT(renderViewEventually()));
    this->connect(&this->Form->ReprLinks, SIGNAL(qtWidgetChanged()),
        this->Display, SLOT(renderViewEventually()));

    // Get the color map object for the display's lookup table.
    this->ColorMap = this->Display->getLookupTable();
    if(this->ColorMap)
      {
      this->connect(this->ColorMap, SIGNAL(destroyed(QObject *)),
          this, SLOT(cleanupDisplay()));
      this->connect(this->ColorMap, SIGNAL(scalarBarsChanged()),
          this, SLOT(checkForLegend()));
      this->Form->Listener->Connect(
          this->ColorMap->getProxy()->GetProperty("RGBPoints"),
          vtkCommand::ModifiedEvent, this, SLOT(handlePointsChanged()));
      }
    }

  // Disable the gui elements if the color map is null.
  this->Form->ColorTabs->setEnabled(this->ColorMap != 0);
  this->initColorScale();
  if(this->ColorMap)
    {
    pqRenderViewBase *renderModule = qobject_cast<pqRenderViewBase *>(
        this->Display->getView());
    this->Form->ShowColorLegend->setEnabled(renderModule != 0);
    this->setLegend(this->ColorMap->getScalarBar(renderModule));
    }
}

void pqColorScaleEditor::showEvent(QShowEvent *e)
{
  // Set up the display and view if the dialog has been dormant.
  if(this->Form->IsDormant)
    {
    this->Form->IsDormant = false;
    this->setRepresentation(this->Form->CurrentDisplay);
    }

  QDialog::showEvent(e);
}

void pqColorScaleEditor::hideEvent(QHideEvent *e)
{
  QDialog::hideEvent(e);

  // Save the current display and view and go dormant.
  pqDataRepresentation *display = this->Form->CurrentDisplay;
  this->setRepresentation(0);
  this->Form->IsDormant = true;
  this->Form->CurrentDisplay = display;
}

void pqColorScaleEditor::handleEditorPointMoved()
{
  if(!this->Form->IgnoreEditor)
    {
    this->updatePointValues();
    }
}

void pqColorScaleEditor::handleEditorPointMoveFinished()
{
  if(!this->Form->IgnoreEditor)
    {
    this->updatePointValues();
    this->setColors();
    }
}

void pqColorScaleEditor::handleEditorAddOrDelete()
{
  if(!this->Form->IgnoreEditor)
    {
    this->setColors();

    // Update the current point.
    this->Form->CurrentIndex = this->Viewer->GetCurrentElementId();

    // Update the gui controls.
    this->enablePointControls();
    this->updatePointValues();
    }
}

void pqColorScaleEditor::setColors()
{
  if(!this->ColorMap)
    {
    return;
    }

  QList<QVariant> rgbPoints;
  QList<QVariant> opacityPoints;
  this->Form->InSetColors = true;

  double rgb[3];
  double scalar = 0.0;
  int total = this->Viewer->GetColorFunction()->GetSize();
  for(int i = 0; i < total; i++)
    {
    if(this->Viewer->GetElementRGBColor(i, rgb))
      {
      scalar = this->Viewer->GetElementScalar(i);
      rgbPoints << scalar << rgb[0] << rgb[1] << rgb[2];
      if(this->OpacityFunction)
        {
        double opacity = this->Viewer->GetElementOpacity(i);
        opacityPoints << scalar << opacity;
        }
      }
    }

  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
      lookupTable->GetProperty("RGBPoints"), rgbPoints);
  if(this->OpacityFunction)
    {
    vtkSMProxy *points = this->OpacityFunction->getProxy();
    pqSMAdaptor::setMultipleElementProperty(points->GetProperty("Points"),
        opacityPoints);
    points->UpdateVTKObjects();
    }

  this->Form->InSetColors = false;
  lookupTable->UpdateVTKObjects();
  this->Display->renderViewEventually();
}

void pqColorScaleEditor::changeCurrentColor()
{
  // Get the current index color from the viewer.
  double rgb[3];
  unsigned int index = this->Viewer->GetCurrentElementId();
  if(this->Viewer->GetElementRGBColor(index, rgb))
    {
    // Let the user choose a color.
    QColor color = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
    color = QColorDialog::getColor(color, this);
    if(color.isValid())
      {
      this->Viewer->SetElementRGBColor(index, color.redF(), color.greenF(),
          color.blueF());
      this->setColors();
      }
    }
}

void pqColorScaleEditor::handlePointsChanged()
{
  // If the point change was not generated by setColors, update the
  // points in the editor.
  if(!this->Form->InSetColors)
    {
    // Save the current point index to use after the change.
    int index = this->Form->CurrentIndex;
    this->Form->CurrentIndex = -1;

    // Load the new points.
    this->Form->IgnoreEditor = true;
    this->loadColorPoints();

    this->Viewer->Render();

    // Set the current point on the editor.
    if(index != -1)
      {
      this->Viewer->SetCurrentElementId(index);
      this->Form->CurrentIndex = this->Viewer->GetCurrentElementId();
      }

    // Update the displayed values.
    this->Form->IgnoreEditor = false;
    this->enablePointControls();
    this->updatePointValues();
    }
}

void pqColorScaleEditor::handleEditorCurrentChanged()
{
  // Get the current index from the viewer.
  if(!this->Form->IgnoreEditor)
    {
    unsigned int id = this->Viewer->GetCurrentElementId();
    this->setCurrentPoint((int)id);
    }
}

void pqColorScaleEditor::setCurrentPoint(int index)
{
  if(index != this->Form->CurrentIndex)
    {
    // Change the current index and update the gui elements.
    this->Form->CurrentIndex = index;
    this->enablePointControls();

    // Get the value and opacity for the current point.
    this->updatePointValues();
    }
}

void pqColorScaleEditor::setValueFromText()
{
  if(this->Form->CurrentIndex == -1)
    {
    return;
    }

  // Get the value from the line edit.
  bool ok = true;
  double value = this->Form->ScalarValue->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous value.
    this->updatePointValues();
    return;
    }



  // Make sure the value is greater than the previous point and less
  // than the next point.
  bool endpoint = this->Form->CurrentIndex == 0;
  if(this->Form->CurrentIndex > 0)
    {
    double prev = this->Viewer->GetElementScalar(this->Form->CurrentIndex - 1);
    if(value <= prev)
      {
      // value not acceptable.
      this->updatePointValues();
      return;
      }
    }

  vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
  endpoint = endpoint || this->Form->CurrentIndex == colors->GetSize() - 1;
  if(this->Form->CurrentIndex < colors->GetSize() - 1)
    {
    double next = this->Viewer->GetElementScalar(this->Form->CurrentIndex + 1);
    if(value >= next)
      {
      // value not acceptable.
      this->updatePointValues();
      return;
      }
    }

  

  // Set the new value on the point in the editor.
  this->Form->IgnoreEditor = true;
  this->Viewer->SetElementScalar(this->Form->CurrentIndex, value);
  this->Form->IgnoreEditor = false;

  // Update the colors on the proxy.
  this->setColors();

  // Update the range if the modified point was an endpoint.
  if(endpoint)
    {
    QPair<double, double> range = this->ColorMap->getScalarRange();
    this->updateScalarRange(range.first, range.second);
    }

  this->Viewer->Render();
}

void pqColorScaleEditor::setOpacityFromText()
{
  if(this->Form->CurrentIndex == -1 || !this->OpacityFunction)
    {
    return;
    }

  // Get the opacity from the line edit.
  bool ok = true;
  double opacity = this->Form->Opacity->text().toDouble(&ok);
  if(!ok)
    {
    // Reset to the previous opacity.
    this->updatePointValues();
    return;
    }

  // Make sure the opacity is valid (0.0 - 1.0).
  if(opacity < 0.0)
    {
    opacity = 0.0;
    }
  else if(opacity > 1.0)
    {
    opacity = 1.0;
    }

  // Set the new opacity on the point in the editor.
  this->Form->IgnoreEditor = true;
  this->Viewer->SetElementOpacity(this->Form->CurrentIndex, opacity);
  this->Viewer->Render();
  this->Form->IgnoreEditor = false;

  // Update the colors on the proxy.
  this->setColors();
}

void pqColorScaleEditor::setColorSpace(int index)
{
  this->Viewer->SetColorSpace(index);
  this->Viewer->Render();

  if(this->ColorMap)
    {
    // Set the property on the lookup table.
    int wrap = index == 2 ? 1 : 0;
    if(index >= 2)
      {
      index--;
      }

    this->Form->InSetColors = true;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("ColorSpace"), index);
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("HSVWrap"), wrap);
    this->Form->InSetColors = false;
    lookupTable->UpdateVTKObjects();
    this->Display->renderViewEventually();
    }
}

void pqColorScaleEditor::setNanColor(const QColor &color)
{
  if (this->ColorMap)
    {
    this->Form->InSetColors = true;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    QList<QVariant> values;
    values << color.redF() << color.greenF() << color.blueF();
    pqSMAdaptor::setMultipleElementProperty(
                                  lookupTable->GetProperty("NanColor"), values);
    this->Form->InSetColors = false;
    lookupTable->UpdateVTKObjects();
    this->Display->renderViewEventually();
    }
}

void pqColorScaleEditor::savePreset()
{
  // Get the color preset model from the manager.
  pqColorPresetModel *model = this->Form->Presets->getModel();

  // Save the current color scale settings as a preset.
  double rgb[3];
  double scalar = 0.0;
  pqColorMapModel colorMap;
  colorMap.setColorSpaceFromInt(this->Form->ColorSpace->currentIndex());
  int total = this->Viewer->GetColorFunction()->GetSize();
  for(int i = 0; i < total; i++)
    {
    if(this->Viewer->GetElementRGBColor(i, rgb))
      {
      scalar = this->Viewer->GetElementScalar(i);
      if(this->OpacityFunction)
        {
        colorMap.addPoint(pqChartValue(scalar),
            QColor::fromRgbF(rgb[0], rgb[1], rgb[2]),
            pqChartValue(this->Viewer->GetElementOpacity(i)));
        }
      else
        {
        colorMap.addPoint(pqChartValue(scalar),
            QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
        }
      }
    }
  colorMap.setNanColor(this->Form->NanColor->chosenColor());

  model->addColorMap(colorMap, "New Color Preset");

  // Select the newly added item (the last in the list).
  QItemSelectionModel *selection = this->Form->Presets->getSelectionModel();
  selection->setCurrentIndex(model->index(model->rowCount() - 1, 0),
      QItemSelectionModel::ClearAndSelect);

  // Set up the dialog and open it.
  this->Form->Presets->setUsingCloseButton(true);
  this->Form->Presets->exec();
}

void pqColorScaleEditor::loadPreset()
{
  this->Form->Presets->setUsingCloseButton(false);
  if(this->Form->Presets->exec() == QDialog::Accepted)
    {
    // Get the color map from the selection.
    QItemSelectionModel *selection = this->Form->Presets->getSelectionModel();
    QModelIndex index = selection->currentIndex();
    const pqColorMapModel *colorMap =
        this->Form->Presets->getModel()->getColorMap(index.row());
    if(colorMap)
      {
      this->Form->IgnoreEditor = true;
      this->Form->CurrentIndex = -1;
      int colorSpace = colorMap->getColorSpaceAsInt();

      QColor color;
      pqChartValue value, opacity;
      pqColorMapModel temp(*colorMap);
      if(this->Form->UseAutoRescale->isChecked() ||
          colorMap->isRangeNormalized())
        {
        QPair<double, double> range = this->ColorMap->getScalarRange();
        temp.setValueRange(range.first, range.second);
        }

      vtkPiecewiseFunction *opacities = 0;
      vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
      colors->RemoveAllPoints();
      if(this->OpacityFunction)
        {
        opacities = this->Viewer->GetOpacityFunction();
        opacities->RemoveAllPoints();
        }

      // Update the displayed range.
      temp.getValueRange(value, opacity);
      this->updateScalarRange(value.getDoubleValue(), opacity.getDoubleValue());

      for(int i = 0; i < colorMap->getNumberOfPoints(); i++)
        {
        temp.getPointColor(i, color);
        temp.getPointValue(i, value);
        colors->AddRGBPoint(value.getDoubleValue(), color.redF(),
            color.greenF(), color.blueF());
        if(this->OpacityFunction)
          {
          temp.getPointOpacity(i, opacity);
          opacities->AddPoint(value.getDoubleValue(),
              opacity.getDoubleValue());
          }
        }

      // Update the color space.
      this->Viewer->SetColorSpace(colorSpace);
      this->Viewer->Render();

      // Update the color space chooser.
      this->Form->ColorSpace->blockSignals(true);
      this->Form->ColorSpace->setCurrentIndex(colorSpace);
      this->Form->ColorSpace->blockSignals(false);
      if(this->ColorMap)
        {
        // Set the property on the lookup table.
        int wrap = colorSpace == 2 ? 1 : 0;
        if(colorSpace >= 2)
          {
          colorSpace--;
          }

        this->Form->InSetColors = true;
        vtkSMProxy *lookupTable = this->ColorMap->getProxy();
        pqSMAdaptor::setElementProperty(
            lookupTable->GetProperty("ColorSpace"), colorSpace);
        pqSMAdaptor::setElementProperty(
            lookupTable->GetProperty("HSVWrap"), wrap);
        this->Form->InSetColors = false;
        }

      // Update the NaN color.
      QColor nanColor;
      colorMap->getNanColor(nanColor);
      this->Form->NanColor->blockSignals(true);
      this->Form->NanColor->setChosenColor(nanColor);
      this->Form->NanColor->blockSignals(false);

      if (this->ColorMap)
        {
        // Set the property on the lookup table.
        this->Form->InSetColors = true;
        vtkSMProxy *lookupTable = this->ColorMap->getProxy();
        QList<QVariant> values;
        values << nanColor.redF() << nanColor.greenF() << nanColor.blueF();
        pqSMAdaptor::setMultipleElementProperty(
                                  lookupTable->GetProperty("NanColor"), values);
        this->Form->InSetColors = false;
        }

      // Update the actual color map.
      this->Form->IgnoreEditor = false;
      this->setColors();

      // Set up the current point index.
      this->Viewer->SetCurrentElementId(0);
      this->Form->CurrentIndex = this->Viewer->GetCurrentElementId();

      this->enablePointControls();
      this->updatePointValues();
      }
    }
}

void pqColorScaleEditor::setLogScale(bool on)
{
  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  pqSMAdaptor::setElementProperty(
      lookupTable->GetProperty("UseLogScale"), on ? 1 : 0);

  // Set the log scale flag on the editor.
  this->Viewer->GetColorFunction()->SetScale(
                                           on ? VTK_CTF_LOG10 : VTK_CTF_LINEAR);
  this->Viewer->Render();

  lookupTable->UpdateVTKObjects();
  this->Display->renderViewEventually();
}

void pqColorScaleEditor::setAutoRescale(bool on)
{
  this->enableRescaleControls(!on);
  this->ColorMap->setScalarRangeLock(!on);
  this->enablePointControls();
  if(on)
    {
    // Reset the range to the current.
    this->rescaleToDataRange();
    }
}

void pqColorScaleEditor::rescaleToNewRange()
{
  // Launch the rescale range dialog to get the new range.
  pqRescaleRange rescaleDialog(this);
  QPair<double, double> range = this->ColorMap->getScalarRange();
  rescaleDialog.setRange(range.first, range.second);
  if(rescaleDialog.exec() == QDialog::Accepted)
    {
    this->setScalarRange(rescaleDialog.getMinimum(),
        rescaleDialog.getMaximum());
    }
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToDataRangeOverTime()
{
  if (QMessageBox::warning(
      pqCoreUtilities::mainWidget(),
      "Potentially slow operation",
      "This can potentially take a long time to complete. \n"
      "Are you sure you want to continue?",
      QMessageBox::Yes |QMessageBox::No, QMessageBox::No) ==
    QMessageBox::Yes)
    {
    pqPipelineRepresentation *pipeline =
      qobject_cast<pqPipelineRepresentation *>(this->Display);
    if(pipeline)
      {
      pipeline->resetLookupTableScalarRangeOverTime();
      pipeline->renderViewEventually();
      }
    }
  // TODO: Handle all the other representation types!
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::rescaleToDataRange()
{
  pqPipelineRepresentation *pipeline =
      qobject_cast<pqPipelineRepresentation *>(this->Display);
  //pqBarChartRepresentation *histogram =
  //    qobject_cast<pqBarChartRepresentation *>(this->Display);
#ifdef FIXME
  pqScatterPlotRepresentation* scatterPlot =
    qobject_cast<pqScatterPlotRepresentation *>(this->Display);
#endif
  if(pipeline)
    {
    pipeline->resetLookupTableScalarRange();
    pipeline->renderViewEventually();
    }
  //else if(histogram)
  //  {
  //  histogram->resetLookupTableScalarRange();
  //  histogram->renderViewEventually();
  //  }
#ifdef FIXME
  else if(scatterPlot)
    {
    scatterPlot->resetLookupTableScalarRange();
    scatterPlot->renderViewEventually();
    }
#endif
}

//-----------------------------------------------------------------------------
void pqColorScaleEditor::setUseDiscreteColors(bool on)
{
  // Update the color scale widget and gui controls.
  this->enableResolutionControls(on);

  // TODO?
  this->Viewer->Render();

  if(this->ColorMap)
    {
    // Set the property on the lookup table.
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("Discretize"), (on ? 1 : 0));
    lookupTable->UpdateVTKObjects();
    this->Display->renderViewEventually();
    }
}

void pqColorScaleEditor::setSizeFromText()
{
  // Get the size from the text. Set the size for the slider and the
  // color scale.
  QString text = this->Form->TableSizeText->text();
  int tableSize = text.toInt();
  this->Form->TableSize->setValue(tableSize);
  this->setTableSize(tableSize);
}

void pqColorScaleEditor::setSizeFromSlider(int tableSize)
{
  QString sizeString;
  sizeString.setNum(tableSize);
  this->Form->TableSizeText->setText(sizeString);
  this->setTableSize(tableSize);
}

void pqColorScaleEditor::setTableSize(int tableSize)
{
  // TODO?
  this->Viewer->Render();

  if(this->ColorMap)
    {
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("NumberOfTableValues"), QVariant(tableSize));
    lookupTable->UpdateVTKObjects();
    this->Display->renderViewEventually();
    }
}

void pqColorScaleEditor::setScalarRange(double min, double max)
{
  // Update the opacity function if volume rendering.
  if(this->OpacityFunction)
    {
    this->Form->InSetColors = true;
    this->OpacityFunction->setScalarRange(min, max);
    this->Form->InSetColors = false;
    }

  // Update the color map and the rendered views.
  this->ColorMap->setScalarRange(min, max);
  this->Display->renderViewEventually();
}

void pqColorScaleEditor::checkForLegend()
{
  if(!this->Form->MakingLegend && this->ColorMap)
    {
    pqRenderViewBase *view = qobject_cast<pqRenderViewBase *>(
        this->Display->getView());
    this->setLegend(this->ColorMap->getScalarBar(view));
    }
}

void pqColorScaleEditor::setLegendVisibility(bool visible)
{
  if(visible && !this->Legend)
    {
    if(this->ColorMap)
      {
      // Create a scalar bar in the current view. Use the display to
      // set up the title.
      this->Form->MakingLegend = true;
      pqLookupTableManager* lutManager =
        pqApplicationCore::instance()->getLookupTableManager();
      pqScalarBarRepresentation* legend = lutManager->setScalarBarVisibility(
        this->Display, visible);     
      
      this->setLegend(legend);
      this->Form->MakingLegend = false;
      }
    else
      {
      qDebug() << "Error: No color map to add a color legend to.";
      }
    }

  if(this->Legend)
    {
    this->Legend->setVisible(visible);
    this->Legend->renderViewEventually();
    }

  this->Form->ShowColorLegend->blockSignals(true);
  this->Form->ShowColorLegend->setChecked(this->Legend && visible);
  this->Form->ShowColorLegend->blockSignals(false);
  this->enableLegendControls(this->Legend && visible);
}

void pqColorScaleEditor::updateLegendVisibility(bool visible)
{
  if(this->Legend)
    {
    this->Form->ShowColorLegend->blockSignals(true);
    this->Form->ShowColorLegend->setChecked(visible);
    this->Form->ShowColorLegend->blockSignals(false);
    }
}

void pqColorScaleEditor::setLegendName(const QString &text)
{
  this->setLegendTitle(text, this->Form->TitleComponent->text());
}

void pqColorScaleEditor::setLegendComponent(const QString &text)
{
  this->setLegendTitle(this->Form->TitleName->text(), text);
}

void pqColorScaleEditor::setLegendTitle(const QString &name,
    const QString &component)
{
  if(this->Legend)
    {
    this->Legend->setTitle(name, component);
    this->Legend->renderViewEventually();
    }
}

void pqColorScaleEditor::updateLegendTitle()
{
  if(this->Legend)
    {
    QPair<QString, QString> title = this->Legend->getTitle();
    this->Form->TitleName->blockSignals(true);
    this->Form->TitleName->setText(title.first);
    this->Form->TitleName->blockSignals(false);

    this->Form->TitleComponent->blockSignals(true);
    this->Form->TitleComponent->setText(title.second);
    this->Form->TitleComponent->blockSignals(false);
    }
}

void pqColorScaleEditor::cleanupDisplay()
{
  this->setRepresentation(0);
}

void pqColorScaleEditor::cleanupLegend()
{
  this->setLegend(0);
}

void pqColorScaleEditor::loadBuiltinColorPresets()
{
  pqColorMapModel colorMap;
  pqColorPresetModel *model = this->Form->Presets->getModel();
  colorMap.setColorSpace(pqColorMapModel::DivergingSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor( 59, 76, 192), 0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(180,  4,  38), 1.0);
  colorMap.setNanColor(QColor(63, 0, 0));
  model->addBuiltinColorMap(colorMap, "Cool to Warm");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::HsvSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 0, 255), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 0, 0), (double)0.0);
  colorMap.setNanColor(QColor(127, 127, 127));
  model->addBuiltinColorMap(colorMap, "Blue to Red Rainbow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::HsvSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(255, 0, 0), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(0, 0, 255), (double)1.0);
  colorMap.setNanColor(QColor(127, 127, 127));
  model->addBuiltinColorMap(colorMap, "Red to Blue Rainbow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0,   0,   0  ), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 255, 255), (double)1.0);
  colorMap.setNanColor(QColor(255, 0, 0));
  model->addBuiltinColorMap(colorMap, "Grayscale");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor( 10,  10, 242), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(242, 242,  10), (double)1.0);
  colorMap.setNanColor(QColor(255, 0, 0));
  model->addBuiltinColorMap(colorMap, "Blue to Yellow");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0,   0,   0  ), (double)0.0);
  colorMap.addPoint(pqChartValue((double)0.4), QColor(230, 0,   0  ), (double)0.4);
  colorMap.addPoint(pqChartValue((double)0.8), QColor(230, 230, 0  ), (double)0.8);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 255, 255), (double)1.0);
  colorMap.setNanColor(QColor(0, 127, 255));
  model->addBuiltinColorMap(colorMap, "Black-Body Radiation");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::LabSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 153, 191), (double)0.0);
  colorMap.addPoint(pqChartValue((double)1.0), QColor(196, 119, 87),(double)1.0);
  colorMap.setNanColor(QColor(255, 255, 0));
  model->addBuiltinColorMap(colorMap, "CIELab Blue to Red");
}

void pqColorScaleEditor::loadColorPoints()
{
  // Clean up the previous data.
  vtkPiecewiseFunction *opacities = 0;
  vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
  colors->RemoveAllPoints();
  if(this->OpacityFunction)
    {
    opacities = this->Viewer->GetOpacityFunction();
    opacities->RemoveAllPoints();
    }

  if(this->ColorMap)
    {
    // Update the displayed min and max.
    QPair<double, double> range = this->ColorMap->getScalarRange();
    this->updateScalarRange(range.first, range.second);

    // Add the new data to the editor.
    QList<QVariant> list;
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("RGBPoints"));
    for(int i = 0; (i + 3) < list.size(); i += 4)
      {
      colors->AddRGBPoint(list[i].toDouble(), list[i + 1].toDouble(),
          list[i + 2].toDouble(), list[i + 3].toDouble());
      }

    if(this->OpacityFunction)
      {
      list = pqSMAdaptor::getMultipleElementProperty(
          this->OpacityFunction->getProxy()->GetProperty("Points"));
      for(int j = 0; (j + 1) < list.size(); j += 2)
        {
        opacities->AddPoint(list[j].toDouble(), list[j + 1].toDouble());
        }
      }
    }
  else
    {
    this->Form->MinimumLabel->setText("");
    this->Form->MaximumLabel->setText("");
    }
}

void pqColorScaleEditor::initColorScale()
{
  // Clear any pending changes and clear the current point index.
  this->Form->CurrentIndex = -1;

  // Ignore changes during editor setup.
  this->Form->IgnoreEditor = true;

  // See if the display supports opacity editing.
  if(this->Display)
    {
    this->OpacityFunction = this->Display->getScalarOpacityFunction();
    }

  bool usingOpacity = this->OpacityFunction != 0;
  bool hasOpacity = this->Form->ScalarOpacityUnitDistance->isEnabled();
  if(usingOpacity != hasOpacity)
    {
    this->Viewer->GetColorFunction()->RemoveAllPoints();
    if(this->OpacityFunction)
      {
      this->Viewer->SetModificationTypeToColorAndOpacity();
      }
    else
      {
      this->Viewer->GetOpacityFunction()->RemoveAllPoints();
      this->Viewer->SetModificationTypeToColor();
      }
    }

  if(this->OpacityFunction)
    {
    this->Form->Listener->Connect(
        this->OpacityFunction->getProxy()->GetProperty("Points"),
        vtkCommand::ModifiedEvent, this, SLOT(handlePointsChanged()));
    this->Form->ReprLinks.addPropertyLink(
      this->Form->ScalarOpacityUnitDistance, "text", SIGNAL(editingFinished()),
      this->Display->getProxy(),
      this->Display->getProxy()->GetProperty("ScalarOpacityUnitDistance"));
    }

  this->Form->ScaleLabel->setEnabled(usingOpacity);
  this->Form->ScalarOpacityUnitDistance->setEnabled(usingOpacity);

  if(this->ColorMap)
    {
    // Set up the rescale controls.
    this->Form->UseAutoRescale->blockSignals(true);
    this->Form->UseAutoRescale->setChecked(
        !this->ColorMap->getScalarRangeLock());
    this->Form->UseAutoRescale->blockSignals(false);
    this->enableRescaleControls(!this->Form->UseAutoRescale->isChecked());

    // Set up the color table size elements.
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    int tableSize = pqSMAdaptor::getElementProperty(
      lookupTable->GetProperty("NumberOfTableValues")).toInt();
    this->Form->TableSize->blockSignals(true);
    this->Form->TableSize->setValue(tableSize);
    this->Form->TableSize->blockSignals(false);
    this->Form->TableSizeText->setText(QString::number(tableSize));

    int discretize = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("Discretize")).toInt();
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(discretize != 0);
    this->Form->UseDiscreteColors->blockSignals(false);
    this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());
    //this->Viewer->SetTableSize(tableSize); // TODO?

    // Set up the color space combo box.
    int space = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("ColorSpace")).toInt();
    this->Form->ColorSpace->blockSignals(true);

    // Set the ColorSpace index, accounting for the fact that "HSVNoWrap" is
    // a fake that is inserted at index 2.
    if(pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("HSVWrap")).toInt())
      {
      this->Form->ColorSpace->setCurrentIndex(2);
      }
    else if (space >= 2)
      {
      this->Form->ColorSpace->setCurrentIndex(space+1);
      }
    else
      {
      this->Form->ColorSpace->setCurrentIndex(space);
      }

    this->Form->ColorSpace->blockSignals(false);

    this->Viewer->SetColorSpace(this->Form->ColorSpace->currentIndex());

    // Set up the NaN color.
    this->Form->NanColor->blockSignals(true);
    QList<QVariant> nanColorValues = pqSMAdaptor::getMultipleElementProperty(
                                          lookupTable->GetProperty("NanColor"));
    QColor nanColor;
    nanColor.setRgbF(nanColorValues[0].toDouble(),
                     nanColorValues[1].toDouble(),
                     nanColorValues[2].toDouble());
    this->Form->NanColor->setChosenColor(nanColor);
    this->Form->NanColor->blockSignals(false);

    // Set up the log scale checkbox. If the log scale is not valid
    // because of the range, loadColorPoints will clear the flag.
    this->Form->UseLogScale->blockSignals(true);
    this->Form->UseLogScale->setChecked(pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("UseLogScale")).toInt() != 0);
    this->Form->UseLogScale->blockSignals(false);

    // Set the log scale flag on the editor.
    this->Viewer->GetColorFunction()->SetScale(
        this->Form->UseLogScale->isChecked() ? VTK_CTF_LOG10 : VTK_CTF_LINEAR);
    }

  // Load the new color points into the editor.
  this->loadColorPoints();

  this->Viewer->Render();

  // Set the current point.
  this->Viewer->SetCurrentElementId(0);
  this->Form->CurrentIndex = this->Viewer->GetCurrentElementId();

  // Update the displayed current point index.
  this->Form->IgnoreEditor = false;
  this->enablePointControls();
  this->updatePointValues();
}

void pqColorScaleEditor::enablePointControls()
{
  bool enable = this->Form->CurrentIndex != -1;
  this->Form->OpacityLabel->setEnabled(this->OpacityFunction != 0);
  this->Form->Opacity->setEnabled(this->OpacityFunction != 0 && enable);

  // The endpoint values are not editable if auto rescale is on.
  if(enable && this->Form->UseAutoRescale->isChecked())
    {
    enable = this->Form->CurrentIndex > 0;
    vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
    enable = enable && this->Form->CurrentIndex < colors->GetSize() - 1;
    }

  this->Form->ScalarValue->setEnabled(enable);
}

void pqColorScaleEditor::updatePointValues()
{
  if(this->Form->CurrentIndex == -1)
    {
    this->Form->ScalarValue->setText("");
    this->Form->Opacity->setText("");
    }
  else
    {
    double value = this->Viewer->GetElementScalar(this->Form->CurrentIndex);
    this->Form->ScalarValue->setText(QString::number(value, 'g', 6));
    if(this->OpacityFunction)
      {
      double opacity = this->Viewer->GetElementOpacity(
          this->Form->CurrentIndex);
      this->Form->Opacity->setText(QString::number(opacity, 'g', 6));
      }
    else
      {
      this->Form->Opacity->setText("");
      }
    }
}

void pqColorScaleEditor::enableRescaleControls(bool enable)
{
  this->Form->RescaleButton->setEnabled(enable);
}

void pqColorScaleEditor::enableResolutionControls(bool enable)
{
  this->Form->TableSizeLabel->setEnabled(enable);
  this->Form->TableSize->setEnabled(enable);
  this->Form->TableSizeText->setEnabled(enable);
}

void pqColorScaleEditor::updateScalarRange(double min, double max)
{
  // Update the spin box ranges and set the values.
  this->Form->MinimumLabel->setText(QString::number(min, 'g', 6));
  this->Form->MaximumLabel->setText(QString::number(max, 'g', 6));

  // Update the editor scalar range.
  this->Viewer->SetWholeScalarRange(min, max);
  this->Viewer->SetVisibleScalarRange(min, max);
}

void pqColorScaleEditor::setLegend(pqScalarBarRepresentation *legend)
{
  if(this->Legend == legend)
    {
    return;
    }

  if(this->Legend)
    {
    // Clean up the current connections.
    this->disconnect(this->Legend, 0, this, 0);
    this->Form->Links.removeAllPropertyLinks();
    delete this->Form->TitleColorLink;
    this->Form->TitleColorLink = 0;
    delete this->Form->LabelColorLink;
    this->Form->LabelColorLink = 0;
    }

  this->Legend = legend;
  if(this->Legend)
    {
    this->connect(this->Legend, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupLegend()));
    this->connect(this->Legend, SIGNAL(visibilityChanged(bool)),
        this, SLOT(updateLegendVisibility(bool)));

    // Connect the legend controls.
    vtkSMProxy *proxy = this->Legend->getProxy();
    this->Form->Links.addPropertyLink(this->Form->TitleColorAdaptor, 
        "color", SIGNAL(colorChanged(const QVariant&)),
        proxy, proxy->GetProperty("TitleColor"));
    this->Form->Links.addPropertyLink(this->Form->TitleFontAdaptor,
        "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, proxy->GetProperty("TitleFontFamily"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleBold, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleBold"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleItalic, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleItalic"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleShadow, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("TitleShadow"));
    this->Form->Links.addPropertyLink(
        this->Form->TitleFontSize, "value", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("TitleFontSize"), 1);
    this->Form->Links.addPropertyLink(
        this->Form->TitleOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("TitleOpacity"));

    this->Form->Links.addPropertyLink(this->Form->LabelColorAdaptor, 
        "color", SIGNAL(colorChanged(const QVariant&)),
        proxy, proxy->GetProperty("LabelColor"));
    this->Form->Links.addPropertyLink(this->Form->LabelFontAdaptor,
        "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, proxy->GetProperty("LabelFontFamily"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelBold, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelBold"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelItalic, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelItalic"));
    this->Form->Links.addPropertyLink(
        this->Form->LabelShadow, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("LabelShadow"));   
    this->Form->Links.addPropertyLink(
        this->Form->LabelFontSize, "value", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("LabelFontSize"), 1);
    this->Form->Links.addPropertyLink(
        this->Form->LabelOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("LabelOpacity"));
    this->Form->Links.addPropertyLink(
        this->Form->AutomaticLabelFormat, "checked", SIGNAL(toggled(bool)),
        proxy, proxy->GetProperty("AutomaticLabelFormat"));   
    this->Form->Links.addPropertyLink(
        this->Form->LabelFormat, "text", SIGNAL(editingFinished()),
        proxy, proxy->GetProperty("LabelFormat"));
    this->connect(this->Form->AutomaticLabelFormat, SIGNAL(toggled(bool)),
                  this, SLOT(updateLabelFormatControls()));
    this->updateLabelFormatControls();
    
    this->Form->Links.addPropertyLink(this->Form->NumberOfLabels,
        "value", SIGNAL(valueChanged(int)),
        proxy, proxy->GetProperty("NumberOfLabels"));
    this->Form->Links.addPropertyLink(this->Form->AspectRatio,
                                      "value", SIGNAL(valueChanged(double)),
                                      proxy, proxy->GetProperty("AspectRatio"));

    // this manages the linking between the global properties and the color
    // properties.
    this->Form->TitleColorLink = new pqStandardColorLinkAdaptor(
      this->Form->TitleColorButton, proxy, "TitleColor");
    this->Form->LabelColorLink = new pqStandardColorLinkAdaptor(
      this->Form->LabelColorButton, proxy, "LabelColor");
    // Update the legend title gui.
    this->updateLegendTitle();
    }

  bool showing = this->Legend && this->Legend->isVisible();
  this->Form->ShowColorLegend->blockSignals(true);
  this->Form->ShowColorLegend->setChecked(showing);
  this->Form->ShowColorLegend->blockSignals(false);
  this->enableLegendControls(showing);
}

void pqColorScaleEditor::enableLegendControls(bool enable)
{
  this->Form->TitleFrame->setEnabled(enable);
  this->Form->LabelFrame->setEnabled(enable);
  this->Form->NumberOfLabels->setEnabled(enable);
  this->Form->CountLabel->setEnabled(enable);
  this->Form->AspectRatio->setEnabled(enable);
  this->Form->AspectRatioLabel->setEnabled(enable);
}

void pqColorScaleEditor::updateLabelFormatControls()
{
  bool autoFormat = this->Form->AutomaticLabelFormat->isChecked();
  this->Form->LabelFormatLabel->setEnabled(!autoFormat);
  this->Form->LabelFormat->setEnabled(!autoFormat);
}

void pqColorScaleEditor::makeDefault()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  if (lut_mgr)
    {
    lut_mgr->saveLUTAsDefault(this->ColorMap);
    if(this->OpacityFunction)
      {
      lut_mgr->saveOpacityFunctionAsDefault(this->OpacityFunction);
      }
    }
}
