/*=========================================================================

   Program: ParaView
   Module:    pqColorScaleEditor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqColorMapColorChanger.h"
#include "pqColorMapModel.h"
#include "pqColorMapWidget.h"
#include "pqColorPresetManager.h"
#include "pqColorPresetModel.h"
#include "pqGenericViewModule.h"
#include "pqObjectBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewModule.h"
#include "pqScalarBarDisplay.h"
#include "pqScalarsToColors.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QGridLayout>
#include <QItemSelectionModel>
#include <QList>
#include <QMenu>
#include <QSpacerItem>
#include <QString>
#include <QtDebug>
#include <QTimer>
#include <QVariant>

#include "vtkColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkType.h"

// Temporary switch
#define USE_VTK_TFE 0


class pqColorScaleEditorForm : public Ui::pqColorScaleDialog
{
public:
  pqColorScaleEditorForm();
  ~pqColorScaleEditorForm() {}

  pqPropertyLinks Links;
  pqSignalAdaptorColor *TitleColorAdaptor;
  pqSignalAdaptorColor *LabelColorAdaptor;
  pqSignalAdaptorComboBox *TitleFontAdaptor;
  pqSignalAdaptorComboBox *LabelFontAdaptor;
  vtkEventQtSlotConnect *Listener;
  pqColorMapWidget *Gradient;
  pqColorPresetManager *Presets;
};


//----------------------------------------------------------------------------
pqColorScaleEditorForm::pqColorScaleEditorForm()
  : Ui::pqColorScaleDialog(), Links()
{
  this->TitleColorAdaptor = 0;
  this->LabelColorAdaptor = 0;
  this->TitleFontAdaptor = 0;
  this->LabelFontAdaptor = 0;
  this->Listener = 0;
  this->Gradient = 0;
  this->Presets = 0;
}


//----------------------------------------------------------------------------
pqColorScaleEditor::pqColorScaleEditor(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqColorScaleEditorForm();
  this->Viewer = vtkTransferFunctionViewer::New();
  this->Display = 0;
  this->ColorMap = 0;
  this->Legend = 0;
  this->EditDelay = new QTimer(this);

  // Set up the ui.
  this->Form->setupUi(this);
  this->Form->Listener = vtkEventQtSlotConnect::New();
  this->Form->Presets = new pqColorPresetManager(this);
  this->Form->Presets->restoreSettings();

#if USE_VTK_TFE
  this->Form->ColorScale->SetRenderWindow(this->Viewer->GetRenderWindow());
  this->Viewer->SetTransferFunctionEditorType(vtkTransferFunctionViewer::SIMPLE_1D);
  this->Viewer->SetModificationTypeToColor();
  this->Viewer->SetWholeScalarRange(0.0, 1.0);
  this->Viewer->SetVisibleScalarRange(0.0, 1.0);
  this->Viewer->SetLockEndPoints(1);
  this->Viewer->SetShowColorFunctionInBackground(1);
  this->Viewer->SetShowColorFunctionOnLines(0);
  this->Viewer->SetBackgroundColor(1.0, 1.0, 1.0);
  this->Viewer->SetLinesColor(0.0, 0.0, 0.0);

  this->Form->Listener->Connect(this->Viewer, vtkCommand::PickEvent,
      this, SLOT(changeCurrentColor()));
  this->Form->Listener->Connect(this->Viewer,
      vtkCommand::WidgetValueChangedEvent, this, SLOT(setColors()));
  this->Form->Listener->Connect(this->Viewer,
      vtkCommand::PlacePointEvent, this, SLOT(setColors()));
#else
  // Hide the qvtk widget and add in a color map widget.
  QGridLayout *grid = qobject_cast<QGridLayout *>(
      this->Form->ScalePage->layout());
  this->Form->Gradient = new pqColorMapWidget();
  this->Form->ColorScale->hide();
  grid->addWidget(this->Form->Gradient, 0, 0, 1, 4);
  pqColorMapModel *colorModel = new pqColorMapModel(this->Form->Gradient);
  colorModel->setObjectName("ColorModel");
  this->Form->Gradient->setModel(colorModel);

  new pqColorMapColorChanger(this->Form->Gradient);
  this->connect(colorModel, SIGNAL(colorChanged(int, const QColor &)),
      this, SLOT(setColors()));
  this->connect(colorModel, SIGNAL(pointAdded(int)), this, SLOT(setColors()));
  this->connect(colorModel, SIGNAL(pointRemoved(int)),
      this, SLOT(setColors()));
  this->connect(this->Form->Gradient, SIGNAL(pointMoved(int)),
      this, SLOT(setColors()));
#endif

  // Set up the timer. The timer is used when the user edits the
  // table size.
  this->EditDelay->setSingleShot(true);

  // Initialize the state of some of the controls.
  this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());
  this->enableRangeControls(this->Form->UseSpecifiedRange->isChecked());
  this->Form->ScalarRangeMin->setMinimum(-VTK_DOUBLE_MAX);
  this->Form->ScalarRangeMin->setMaximum(VTK_DOUBLE_MAX);
  this->Form->ScalarRangeMax->setMinimum(-VTK_DOUBLE_MAX);
  this->Form->ScalarRangeMax->setMaximum(VTK_DOUBLE_MAX);

  this->enableLegendControls(this->Form->ShowColorLegend->isChecked());

  // Add the color scale presets menu.
  this->initColorPresets();

  // Connect the color scale widgets.
  this->connect(this->Form->ColorSpace, SIGNAL(activated(int)),
      this, SLOT(setColorSpace(int)));

  this->connect(this->Form->SaveButton, SIGNAL(clicked()),
      this, SLOT(savePreset()));
  this->connect(this->Form->PresetButton, SIGNAL(clicked()),
      this, SLOT(loadPreset()));

  this->connect(this->Form->UseDiscreteColors, SIGNAL(toggled(bool)),
      this, SLOT(setUseDiscreteColors(bool)));
  this->connect(this->Form->TableSize, SIGNAL(valueChanged(int)),
      this, SLOT(setSizeFromSlider(int)));
  this->connect(this->Form->TableSizeText, SIGNAL(textEdited(const QString &)),
      this, SLOT(handleSizeTextEdit(const QString &)));
  this->connect(this->EditDelay, SIGNAL(timeout()),
      this, SLOT(setSizeFromText()));

  this->connect(this->Form->Component, SIGNAL(activated(int)),
      this, SLOT(setComponent(int)));

  this->connect(this->Form->UseDataRange, SIGNAL(toggled(bool)),
      this, SLOT(handleRangeLockChanged(bool)));
  this->connect(this->Form->UseSpecifiedRange, SIGNAL(toggled(bool)),
      this, SLOT(handleRangeLockChanged(bool)));
  this->connect(this->Form->ScalarRangeMin, SIGNAL(valueChanged(double)),
      this, SLOT(setMinimumScalar(double)));
  this->connect(this->Form->ScalarRangeMax, SIGNAL(valueChanged(double)),
      this, SLOT(setMaximumScalar(double)));

  this->connect(this->Form->ResetRangeButton, SIGNAL(clicked()),
      this, SLOT(resetRange()));
  this->connect(this->Form->ResetRangeToCurrentButton, SIGNAL(clicked()),
      this, SLOT(resetRangeToCurrent()));

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
  delete this->EditDelay;
}

void pqColorScaleEditor::setDisplay(pqPipelineDisplay *display)
{
  if(this->Display == display)
    {
    return;
    }

  this->setLegend(0);
  if(this->Display)
    {
    this->disconnect(this->Display, 0, this, 0);
    this->disconnect(&this->Form->Links, 0, this->Display, 0);
    if(this->ColorMap)
      {
      this->disconnect(this->ColorMap, 0, this, 0);
      }
    }

  this->Display = display;
  this->ColorMap = 0;
  if(this->Display)
    {
    this->connect(this->Display, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupDisplay()));
    this->connect(&this->Form->Links, SIGNAL(qtWidgetChanged()),
        this->Display, SLOT(renderAllViews()));

    // Get the color map object for the display's lookup table.
    this->ColorMap = this->Display->getLookupTable();
    if(this->ColorMap)
      {
      this->connect(this->ColorMap, SIGNAL(destroyed(QObject *)),
          this, SLOT(cleanupDisplay()));
      }
    }

  // Cleanup the previous gui data if any.
  this->Form->Component->clear();
  this->Form->ColorSpace->clear();
#if USE_VTK_TFE
  this->Viewer->GetColorFunction()->RemoveAllPoints();
#else
  this->Form->Gradient->getModel()->removeAllPoints();
#endif

  // Disable the gui elements if the color map is null.
  this->Form->ColorTabs->setEnabled(this->ColorMap != 0);
  if(!this->ColorMap)
    {
    return;
    }

  this->initColorScale();

  pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(
      this->Display->getViewModule(0));
  this->setLegend(this->ColorMap->getScalarBar(renderModule));
}

void pqColorScaleEditor::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);
  //this->Viewer->Render();
}

void pqColorScaleEditor::hideEvent(QHideEvent *e)
{
  // If the edit delay timer is active, set the final user entry.
  if(this->EditDelay->isActive())
    {
    this->EditDelay->stop();
    this->setSizeFromText();
    }

  QDialog::hideEvent(e);
}

void pqColorScaleEditor::setColors()
{
  if(!this->ColorMap)
    {
    return;
    }

  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  if(lookupTable->GetXMLName() == QString("PVLookupTable"))
    {
    QList<QVariant> rgbPoints;
#if USE_VTK_TFE
    double rgb[3];
    double scalar = 0.0;
    unsigned int total = static_cast<unsigned int>(
        this->Viewer->GetColorFunction()->GetSize());
    for(unsigned int i = 0; i < total; i++)
      {
      if(this->Viewer->GetElementRGBColor(i, rgb))
        {
        scalar = this->Viewer->GetElementScalar(i);
        rgbPoints << scalar << rgb[0] << rgb[1] << rgb[2];
        }
      }
#else
    pqColorMapModel *model = this->Form->Gradient->getModel();
    for(int i = 0; i < model->getNumberOfPoints(); ++i)
      {
      QColor color;
      pqChartValue scalar;
      model->getPointValue(i, scalar);
      model->getPointColor(i, color);
      rgbPoints << scalar.getDoubleValue() <<
          color.redF() << color.greenF() << color.blueF();
      }
#endif

    pqSMAdaptor::setMultipleElementProperty(
        lookupTable->GetProperty("RGBPoints"), rgbPoints);
    }
  else
    {
    QList<QVariant> list;
#if USE_VTK_TFE
    double hsv1[3];
    double hsv2[3];
    if(this->Viewer->GetElementHSVColor(0, hsv1))
      {
        if(this->Viewer->GetElementHSVColor(1, hsv2))
        {
        list.append(QVariant(hsv1[0]));
        list.append(QVariant(hsv2[0]));
        pqSMAdaptor::setMultipleElementProperty(
            lookupTable->GetProperty("HueRange"), list);
        list[0] = QVariant(hsv1[1]);
        list[1] = QVariant(hsv2[1]);
        pqSMAdaptor::setMultipleElementProperty(
            lookupTable->GetProperty("SaturationRange"), list);
        list[0] = QVariant(hsv1[2]);
        list[1] = QVariant(hsv2[2]);
        pqSMAdaptor::setMultipleElementProperty(
            lookupTable->GetProperty("ValueRange"), list);
        }
      }
#else
    QColor color1;
    QColor color2;
    pqColorMapModel *model = this->Form->Gradient->getModel();
    model->getPointColor(0, color1);
    model->getPointColor(1, color2);
    list.append(QVariant(color1.hueF()));
    list.append(QVariant(color2.hueF()));
    pqSMAdaptor::setMultipleElementProperty(
        lookupTable->GetProperty("HueRange"), list);
    list[0] = QVariant(color1.saturationF());
    list[1] = QVariant(color2.saturationF());
    pqSMAdaptor::setMultipleElementProperty(
        lookupTable->GetProperty("SaturationRange"), list);
    list[0] = QVariant(color1.valueF());
    list[1] = QVariant(color2.valueF());
    pqSMAdaptor::setMultipleElementProperty(
        lookupTable->GetProperty("ValueRange"), list);
#endif
    }

  lookupTable->UpdateVTKObjects();
  this->Display->renderAllViews();
}

void pqColorScaleEditor::changeCurrentColor()
{
#if USE_VTK_TFE
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
#endif
}

void pqColorScaleEditor::setColorSpace(int index)
{
#if USE_VTK_TFE
  this->Viewer->SetColorSpace(index);
  this->Viewer->Render();
#else
  pqColorMapModel *model = this->Form->Gradient->getModel();
  model->setColorSpaceFromInt(index);
#endif

  if(this->ColorMap)
    {
    // Set the property on the lookup table.
    int wrap = index == 2 ? 1 : 0;
    if(wrap == 1)
      {
      index = 1;
      }

    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("ColorSpace"), index);
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("HSVWrap"), wrap);
    lookupTable->UpdateVTKObjects();
    this->Display->renderAllViews();
    }
}

void pqColorScaleEditor::savePreset()
{
  // Save the current color scale settings as a preset.
  pqColorPresetModel *model = this->Form->Presets->getModel();
  model->addColorMap(*this->Form->Gradient->getModel(), "New Color Preset");
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
#if USE_VTK_TFE
      vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
#else
      pqColorMapModel *model = this->Form->Gradient->getModel();
#endif
      if(this->Form->UseDiscreteColors->isEnabled())
        {
        int colorSpace = colorMap->getColorSpaceAsInt();
#if USE_VTK_TFE
        QColor color;
        pqChartValue value;
        colors->RemoveAllPoints();
        for(int i = 0; i < colorMap->getNumberOfPoints(); i++)
          {
          colorMap->getPointColor(i, color);
          colorMap->getPointValue(i, value);
          colors->AddRGBPoint(value.getDoubleValue(), color.redF(),
              color.greenF(), color.blueF());
          }

        // Update the color space.
        this->Viewer->SetColorSpace(colorSpace);
        this->Viewer->Render();
#else
        model->startModifyingData();
        *model = *colorMap;
        model->finishModifyingData();
        if(colorSpace == 2)
          {
          colorSpace = 1; // TEMP
          }
#endif

        // Update the color space chooser.
        this->Form->ColorSpace->setCurrentIndex(colorSpace);
        if(this->ColorMap)
          {
          // Set the property on the lookup table.
          int wrap = colorSpace == 2 ? 1 : 0;
          if(wrap == 1)
            {
            colorSpace = 1;
            }

          vtkSMProxy *lookupTable = this->ColorMap->getProxy();
          pqSMAdaptor::setElementProperty(
              lookupTable->GetProperty("ColorSpace"), colorSpace);
          pqSMAdaptor::setElementProperty(
              lookupTable->GetProperty("HSVWrap"), wrap);
          }
        }
      else
        {
        QColor color1, color2;
        colorMap->getPointColor(0, color1);
        colorMap->getPointColor(colorMap->getNumberOfPoints() - 1, color2);
#if USE_VTK_TFE
        colors->RemoveAllPoints();
        colors->AddRGBPoint(0.0, color1.redF(), color1.greenF(),
            color1.blueF());
        colors->AddRGBPoint(1.0, color2.redF(), color2.greenF(),
            color2.blueF());
        this->Viewer->Render();
#else
        model->startModifyingData();
        model->removeAllPoints();
        model->addPoint(pqChartValue((double)0.0), color1);
        model->addPoint(pqChartValue((double)1.0), color2);
        model->finishModifyingData();
#endif
        }

      // Update the actual color map.
      this->setColors();
      }
    }
}

void pqColorScaleEditor::setUseDiscreteColors(bool on)
{
  // Update the color scale widget and gui controls.
  this->enableResolutionControls(on);
#if USE_VTK_TFE
  // TODO?
  this->Viewer->Render();
#else
  int tableSize = 0;
  if(!on)
    {
    tableSize = this->Form->TableSizeText->text().toInt();
    }

  this->Form->Gradient->setTableSize(tableSize);
#endif

  if(this->ColorMap)
    {
    // Set the property on the lookup table.
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("Discretize"), (on ? 1 : 0));
    lookupTable->UpdateVTKObjects();
    this->Display->renderAllViews();
    }
}

void pqColorScaleEditor::handleSizeTextEdit(const QString &)
{
  // TODO: Validate the text.

  // Start a timer to allow the user to enter more text. If the timer
  // is already running delay it for more text.
  this->EditDelay->start(600);
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
#if USE_VTK_TFE
  // TODO?
#else
  this->Form->Gradient->setTableSize(tableSize);
#endif

  if(this->ColorMap)
    {
    vtkSMProxy *lookupTable = this->ColorMap->getProxy();
    pqSMAdaptor::setElementProperty(
        lookupTable->GetProperty("NumberOfTableValues"), QVariant(tableSize));
    lookupTable->UpdateVTKObjects();
    this->Display->renderAllViews();
    }
}

void pqColorScaleEditor::setComponent(int index)
{
  int component = this->Form->Component->itemData(index).toInt();
  this->ColorMap->setVectorMode( 
    (component == -1) ? pqScalarsToColors::MAGNITUDE : pqScalarsToColors::COMPONENT,
    component);

  // TODO: Update the scalar range if the range is set from the data.

  // Update the color legend title.
  this->setLegendComponent(this->Form->Component->itemText(index));
  this->Display->renderAllViews();
}

void pqColorScaleEditor::handleRangeLockChanged(bool on)
{
  // This method is called from both radio buttons. Only react to the
  // one that is checked.
  if(on)
    {
    bool isLocked = this->Form->UseSpecifiedRange->isChecked();
    this->enableRangeControls(isLocked);
    this->ColorMap->setScalarRangeLock(isLocked);
    if(!isLocked)
      {
      // Reset the displayed range to the current.
      this->resetRangeToCurrent();
      }
    }
}

void pqColorScaleEditor::setMinimumScalar(double min)
{
  this->setScalarRange(min, this->Form->ScalarRangeMax->value());
}

void pqColorScaleEditor::setMaximumScalar(double max)
{
  this->setScalarRange(this->Form->ScalarRangeMin->value(), max);
}

void pqColorScaleEditor::setScalarRange(double min, double max)
{
  // Update the gui elements.
  this->updateScalarRange(min, max);

  // Update the color map and the rendered views.
  this->ColorMap->setScalarRange(min, max);
  this->Display->renderAllViews();
}

void pqColorScaleEditor::resetRange()
{
}

void pqColorScaleEditor::resetRangeToCurrent()
{
  QString colorField = this->Display->getColorField();
  int component_no = -1;
  if(this->ColorMap->getVectorMode() == pqScalarsToColors::COMPONENT)
    {
    component_no = this->ColorMap->getVectorComponent();
    }

  QPair<double, double> range = 
      this->Display->getColorFieldRange(colorField, component_no);
  this->setScalarRange(range.first, range.second);
}

void pqColorScaleEditor::setLegendVisibility(bool visible)
{
  if(visible && !this->Legend)
    {
    if(this->ColorMap)
      {
      // Create a scalar bar in the current view. Use the display to
      // set up the title.
      pqObjectBuilder *builder =
          pqApplicationCore::instance()->getObjectBuilder();
      pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(
          this->Display->getViewModule(0));
      pqScalarBarDisplay *legend = builder->createScalarBarDisplay(
        this->ColorMap, renderModule);
      legend->makeTitle(this->Display);
      this->setLegend(legend);
      }
    else
      {
      qDebug() << "Error: No color map to add a color legend to.";
      }
    }

  if(this->Legend)
    {
    this->Legend->setVisible(visible);
    this->Legend->renderAllViews();
    }

  this->Form->ShowColorLegend->blockSignals(true);
  this->Form->ShowColorLegend->setChecked(this->Legend && visible);
  this->Form->ShowColorLegend->blockSignals(false);
  this->enableLegendControls(this->Legend && visible);
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
    this->Legend->renderAllViews();
    }
}

void pqColorScaleEditor::cleanupDisplay()
{
  this->setDisplay(0);
}

void pqColorScaleEditor::cleanupLegend()
{
  this->setLegend(0);
}

void pqColorScaleEditor::initColorPresets()
{
  pqColorMapModel colorMap;
  pqColorPresetModel *model = this->Form->Presets->getModel();
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 0, 255));
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 0, 0));
  model->addBuiltinColorMap(colorMap, "Blue to Red");

  colorMap.removeAllPoints();
  colorMap.addPoint(pqChartValue((double)0.0), QColor(255, 0, 0));
  colorMap.addPoint(pqChartValue((double)1.0), QColor(0, 0, 255));
  model->addBuiltinColorMap(colorMap, "Red to Blue");

  colorMap.removeAllPoints();
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 0, 0));
  colorMap.addPoint(pqChartValue((double)1.0), QColor(255, 255, 255));
  model->addBuiltinColorMap(colorMap, "Grayscale");

  colorMap.removeAllPoints();
  colorMap.setColorSpace(pqColorMapModel::RgbSpace);
  colorMap.addPoint(pqChartValue((double)0.0), QColor(0, 153, 191));
  colorMap.addPoint(pqChartValue((double)1.0), QColor(196, 119, 87));
  model->addBuiltinColorMap(colorMap, "CIELab Blue to Red");
}

void pqColorScaleEditor::initColorScale()
{
  // Set up the component combo box.
  int numComponents = this->Display->getColorFieldNumberOfComponents(
      this->Display->getColorField());
  this->Form->Component->setEnabled(numComponents > 1);
  this->Form->ComponentLabel->setEnabled(numComponents > 1);
  if(numComponents > 1)
    {
    this->Form->Component->addItem("Magnitude", QVariant(-1));
    if(numComponents <= 3)
      {
      const char *titles[] = {"X", "Y", "Z"};
      for(int cc = 0; cc < numComponents; cc++)
        {
        this->Form->Component->addItem(titles[cc], QVariant(cc));
        }
      }
    else
      {
      for(int cc = 0; cc < numComponents; cc++)
        {
        this->Form->Component->addItem(QString::number(cc), QVariant(cc));
        }
      }

    int index = 0;
    if(this->ColorMap->getVectorMode() == pqScalarsToColors::COMPONENT)
      {
      index = this->ColorMap->getVectorComponent() + 1;
      }

    this->Form->Component->setCurrentIndex(index);
    }

  // Set up the scalar range elements.
  this->Form->UseSpecifiedRange->blockSignals(true);
  this->Form->UseDataRange->blockSignals(true);
  if(this->ColorMap->getScalarRangeLock())
    {
    this->Form->UseSpecifiedRange->setChecked(true);
    }
  else
    {
    this->Form->UseDataRange->setChecked(true);
    }

  this->enableRangeControls(this->Form->UseSpecifiedRange->isChecked());
  this->Form->UseSpecifiedRange->blockSignals(false);
  this->Form->UseDataRange->blockSignals(false);
  QPair<double, double> range = this->ColorMap->getScalarRange();
  this->updateScalarRange(range.first, range.second);

  // Set up the color table size elements.
  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  int tableSize = pqSMAdaptor::getElementProperty(
    lookupTable->GetProperty("NumberOfTableValues")).toInt();
  this->Form->TableSize->blockSignals(true);
  this->Form->TableSize->setValue(tableSize);
  this->Form->TableSize->blockSignals(false);
  this->Form->TableSizeText->setText(QString::number(tableSize));

  QList<QVariant> list;
#if USE_VTK_TFE
  vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
#else
  QColor color;
  pqColorMapModel *model = this->Form->Gradient->getModel();
  model->startModifyingData();
#endif
  if(lookupTable->GetXMLName() == QString("PVLookupTable"))
    {
    this->Form->UseDiscreteColors->setEnabled(true);
    int discretize = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("Discretize")).toInt();
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(discretize != 0);
    this->Form->UseDiscreteColors->blockSignals(false);

    // Add the color space options to the combo box.
    this->Form->ColorSpace->addItem("RGB");
    this->Form->ColorSpace->addItem("HSV");
#if USE_VTK_TFE
    this->Form->ColorSpace->addItem("Wrapped HSV");
#endif
    int space = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("ColorSpace")).toInt();
    this->Form->ColorSpace->setCurrentIndex(space);
#if USE_VTK_TFE
    if(pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("HSVWrap")).toInt())
      {
      this->Form->ColorSpace->setCurrentIndex(2);
      }

    this->Viewer->SetColorSpace(this->Form->ColorSpace->currentIndex());
#else
    model->setColorSpaceFromInt(this->Form->ColorSpace->currentIndex());
#endif

    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("RGBPoints"));
    for(int i = 0; (i + 3) < list.size(); i += 4)
      {
#if USE_VTK_TFE
      colors->AddRGBPoint(list[i].toDouble(), list[i + 1].toDouble(),
          list[i + 2].toDouble(), list[i + 3].toDouble());
#else
      color = QColor::fromRgbF(list[i + 1].toDouble(),
          list[i + 2].toDouble(), list[i + 3].toDouble());
      pqChartValue scalar = list[i].toDouble();
      model->addPoint(scalar, color);
#endif
      }
    }
  else // Old lookup table.
    {
    this->Form->UseDiscreteColors->setEnabled(false);
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(true);
    this->Form->UseDiscreteColors->blockSignals(false);

    // Add the color space options to the combo box.
    this->Form->ColorSpace->addItem("HSV");
    this->Form->ColorSpace->setCurrentIndex(0);
#if USE_VTK_TFE
    this->Viewer->SetColorSpace(1);
#else
    model->setColorSpace(pqColorMapModel::HsvSpace);
#endif

    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("HueRange"));
    double h1 = list[0].toDouble();
    double h2 = list[1].toDouble();
    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("SaturationRange"));
    double s1 = list[0].toDouble();
    double s2 = list[1].toDouble();
    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("ValueRange"));
    double v1 = list[0].toDouble();
    double v2 = list[1].toDouble();
#if USE_VTK_TFE
    colors->AddHSVPoint(0.0, h1, s1, v1);
    colors->AddHSVPoint(1.0, h2, s2, v2);
#else
    color = QColor::fromHsvF(h1, s1, v1);
    model->addPoint(pqChartValue((double)0.0), color);
    color = QColor::fromHsvF(h2, s2, v2);
    model->addPoint(pqChartValue((double)1.0), color);
#endif
    }

  this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());
#if USE_VTK_TFE
  //this->Viewer->SetTableSize(tableSize); // TODO?
  this->Viewer->SetAllowInteriorElements(
      this->Form->UseDiscreteColors->isEnabled() ? 1 : 0);
  this->Viewer->Render();
#else
  if(!this->Form->UseDiscreteColors->isChecked())
    {
    tableSize = 0;
    }

  this->Form->Gradient->setTableSize(tableSize);
  model->finishModifyingData();
  this->Form->Gradient->setAddingPointsAllowed(
      this->Form->UseDiscreteColors->isEnabled());
#endif
}

void pqColorScaleEditor::enableResolutionControls(bool enable)
{
  this->Form->TableSizeLabel->setEnabled(enable);
  this->Form->TableSize->setEnabled(enable);
  this->Form->TableSizeText->setEnabled(enable);
}

void pqColorScaleEditor::enableRangeControls(bool enable)
{
  this->Form->ScalarRangeMin->setEnabled(enable);
  this->Form->ScalarRangeMax->setEnabled(enable);
  //this->Form->ResetRangeButton->setEnabled(enable);
  this->Form->ResetRangeToCurrentButton->setEnabled(enable);
}

void pqColorScaleEditor::updateScalarRange(double min, double max)
{
  // Update the spin box ranges and set the values.
  this->Form->ScalarRangeMin->blockSignals(true);
  this->Form->ScalarRangeMax->blockSignals(true);
  this->Form->ScalarRangeMin->setMaximum(max);
  this->Form->ScalarRangeMax->setMinimum(min);
  this->Form->ScalarRangeMin->setValue(min);
  this->Form->ScalarRangeMax->setValue(max);
  this->Form->ScalarRangeMin->blockSignals(false);
  this->Form->ScalarRangeMax->blockSignals(false);
}

void pqColorScaleEditor::setLegend(pqScalarBarDisplay *legend)
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
    }

  this->Legend = legend;
  if(this->Legend)
    {
    this->connect(this->Legend, SIGNAL(destroyed(QObject *)),
        this, SLOT(cleanupLegend()));

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
        this->Form->TitleOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("TitleOpacity"));

    this->Form->Links.addPropertyLink(
        this->Form->LabelFormat, "text", SIGNAL(textChanged(const QString&)),
        proxy, proxy->GetProperty("LabelFormat"));   
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
        this->Form->LabelOpacity, "value", SIGNAL(valueChanged(double)),
        proxy, proxy->GetProperty("LabelOpacity"));

    this->Form->Links.addPropertyLink(this->Form->NumberOfLabels,
        "value", SIGNAL(valueChanged(int)),
        proxy, proxy->GetProperty("NumberOfLabels"));

    // Update the legend title gui.
    QPair<QString, QString> title = this->Legend->getTitle();
    this->Form->TitleName->blockSignals(true);
    this->Form->TitleName->setText(title.first);
    this->Form->TitleName->blockSignals(false);

    this->Form->TitleComponent->blockSignals(true);
    this->Form->TitleComponent->setText(title.second);
    this->Form->TitleComponent->blockSignals(false);
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
}


