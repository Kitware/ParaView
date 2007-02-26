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
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqGenericViewModule.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewModule.h"
#include "pqScalarBarDisplay.h"
#include "pqScalarsToColors.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QSpacerItem>
#include <QString>
#include <QtDebug>
#include <QTimer>
#include <QVariant>

#include "vtkColorTransferFunction.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkTransferFunctionViewer.h"
#include "vtkType.h"


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
};


//----------------------------------------------------------------------------
pqColorScaleEditorForm::pqColorScaleEditorForm()
  : Ui::pqColorScaleDialog(), Links()
{
  this->TitleColorAdaptor = 0;
  this->LabelColorAdaptor = 0;
  this->TitleFontAdaptor = 0;
  this->LabelFontAdaptor = 0;
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
  this->Form->ColorScale->SetRenderWindow(this->Viewer->GetRenderWindow());
  //this->Viewer->SetInteractor(this->Form->ColorScale->GetInteractor());
  //this->Viewer->SetRenderWindow(this->Form->ColorScale->GetRenderWindow());
  this->Viewer->SetTransferFunctionEditorType(vtkTransferFunctionViewer::SIMPLE_1D);
  this->Viewer->SetModificationTypeToColor();
  //this->Viewer->SetBackgroundColor(1.0, 1.0, 1.0);

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

  // Connect the color scale widgets.
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

  // Add a spacer to the legend page. Adding the spacer in the designer
  // messed up the layout.
  QGridLayout *grid = qobject_cast<QGridLayout *>(
      this->Form->LegendPage->layout());
  grid->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding,
      QSizePolicy::MinimumExpanding), 4, 0, 1, 3);
}

pqColorScaleEditor::~pqColorScaleEditor()
{
  delete this->Form->LabelColorAdaptor;
  delete this->Form->TitleColorAdaptor;
  delete this->Form->LabelFontAdaptor;
  delete this->Form->TitleFontAdaptor;
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
  this->Viewer->GetColorFunction()->RemoveAllPoints();

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

void pqColorScaleEditor::closeEvent(QCloseEvent *e)
{
  // If the edit delay timer is active, set the final user entry.
  if(this->EditDelay->isActive())
    {
    this->EditDelay->stop();
    this->setSizeFromText();
    }

  QDialog::closeEvent(e);
}

void pqColorScaleEditor::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);
  //this->Viewer->Render();
}

void pqColorScaleEditor::setUseDiscreteColors(bool on)
{
  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  pqSMAdaptor::setElementProperty(
      lookupTable->GetProperty("Discretize"), (on ? 1 : 0));
  this->enableResolutionControls(on);
  this->Viewer->Render();
  lookupTable->UpdateVTKObjects();
  this->Display->renderAllViews();
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
  // TODO
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
  this->Viewer->Render();

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
      pqPipelineBuilder *builder =
          pqApplicationCore::instance()->getPipelineBuilder();
      pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(
          this->Display->getViewModule(0));
      pqScalarBarDisplay *legend = builder->createScalarBar(this->ColorMap,
          renderModule);
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
  this->Viewer->SetVisibleScalarRange(range.first, range.second);

  // Set up the color table size elements.
  vtkSMProxy *lookupTable = this->ColorMap->getProxy();
  int tableSize = pqSMAdaptor::getElementProperty(
    lookupTable->GetProperty("NumberOfTableValues")).toInt();
  this->Form->TableSize->blockSignals(true);
  this->Form->TableSize->setValue(tableSize);
  this->Form->TableSize->blockSignals(false);
  this->Form->TableSizeText->setText(QString::number(tableSize));
  //this->Viewer->SetTableSize(tableSize); // TODO

  QList<QVariant> list;
  vtkColorTransferFunction *colors = this->Viewer->GetColorFunction();
  if(lookupTable->GetXMLName() == QString("PVLookupTable"))
    {
    this->Form->UseDiscreteColors->setEnabled(true);
    int discretize = pqSMAdaptor::getElementProperty(
        lookupTable->GetProperty("Discretize")).toInt();
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(discretize != 0);
    this->Form->UseDiscreteColors->blockSignals(false);

    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("RGBPoints"));
    for(int i = 0; (i + 3) < list.size(); i += 4)
      {
      colors->AddRGBPoint(list[i].toDouble(), list[i + 1].toDouble(),
          list[i + 2].toDouble(), list[i + 3].toDouble());
      }
    }
  else
    {
    this->Form->UseDiscreteColors->setEnabled(false);
    this->Form->UseDiscreteColors->blockSignals(true);
    this->Form->UseDiscreteColors->setChecked(true);
    this->Form->UseDiscreteColors->blockSignals(false);

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
    list = pqSMAdaptor::getMultipleElementProperty(
        lookupTable->GetProperty("ScalarRange"));
    colors->AddHSVPoint(list[0].toDouble(), h1, s1, v1);
    colors->AddHSVPoint(list[1].toDouble(), h2, s2, v2);
    }

  this->enableResolutionControls(this->Form->UseDiscreteColors->isChecked());
  this->Viewer->Render();
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

  // Update the transfer function editor.
  this->Viewer->SetWholeScalarRange(min, max);
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


