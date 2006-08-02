/*=========================================================================

   Program: ParaView
   Module:    pqColorMapEditor.cxx

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

/// \file pqColorMapEditor.cxx
/// \date 7/31/2006

#include "pqColorMapEditor.h"
#include "ui_pqColorMapEditor.h"

#include "pqChartValue.h"
#include "pqColorMapColorChanger.h"
#include "pqColorMapWidget.h"
#include "pqPipelineDisplay.h"
#include "pqSMAdaptor.h"

#include <QColor>
#include <QList>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariant>

#include "vtkSMLookupTableProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"


class pqColorMapEditorForm : public Ui::pqColorMapEditor
{
public:
  pqColorMapEditorForm();
  ~pqColorMapEditorForm() {}

  QPointer<pqPipelineDisplay> Display;
};


pqColorMapEditorForm::pqColorMapEditorForm()
{
  this->Display = 0;
}


pqColorMapEditor::pqColorMapEditor(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqColorMapEditorForm();
  this->LookupTable = 0;
  this->EditDelay = new QTimer(this);
  this->Form->setupUi(this);

  // Set up the timer.
  this->EditDelay->setSingleShot(true);

  // Use the default color chooser. The pointer will get deleted with
  // the form.
  new pqColorMapColorChanger(this->Form->ColorScale);

  // TEMP: Disable the checkbox options until they can be supported.
  this->Form->UseTable->setEnabled(false);
  this->Form->UseGradient->setEnabled(false);

  // Connect the close button.
  this->connect(this->Form->CloseButton, SIGNAL(clicked()),
      this, SLOT(closeForm()));

  // Link the resolution slider and text box with the color scale.
  this->connect(this->Form->TableSize, SIGNAL(valueChanged(int)),
      this, SLOT(setSizeFromSlider(int)));
  this->connect(this->Form->TableSizeText, SIGNAL(textEdited(const QString &)),
      this, SLOT(handleTextEdit(const QString &)));
  this->connect(this->EditDelay, SIGNAL(timeout()),
      this, SLOT(setSizeFromText()));

  // Listen for color changes.
  this->connect(
      this->Form->ColorScale, SIGNAL(colorChanged(int, const QColor &)),
      this, SLOT(changeColor(int, const QColor &)));
}

pqColorMapEditor::~pqColorMapEditor()
{
  delete this->Form;
  delete this->EditDelay;
}

void pqColorMapEditor::setDisplay(pqPipelineDisplay *display)
{
  if(this->Form->Display == display)
    {
    return;
    }

  if(!this->Form->Display.isNull())
    {
    // Clean up the color scale.
    this->Form->ColorScale->clearPoints();
    }

  this->Form->Display = display;
  this->LookupTable = 0;
  if(this->Form->Display.isNull())
    {
    return;
    }

  // TODO: Support the color transfer function as well.
  // Get the lookup table from the display.
  vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(
      this->Form->Display->getProxy()->GetProperty("LookupTable"));
  if(prop)
    {
    this->LookupTable = vtkSMLookupTableProxy::SafeDownCast(prop->GetProxy(0));
    }

  if(this->LookupTable)
    {
    // Set the resolution from the lookup table.
    vtkSMProperty *prop = this->LookupTable->GetProperty("NumberOfTableValues");
    int value = pqSMAdaptor::getElementProperty(prop).toInt();
    this->Form->ColorScale->setTableSize(value);
    this->Form->TableSize->setValue(value);
    QString valueString;
    valueString.setNum(value);
    this->Form->TableSizeText->setText(valueString);

    // Set the end points from the lookup table.
    prop = this->LookupTable->GetProperty("HueRange");
    QList<QVariant> list = pqSMAdaptor::getMultipleElementProperty(prop);
    double h1 = list[0].toDouble();
    double h2 = list[1].toDouble();
    prop = this->LookupTable->GetProperty("SaturationRange");
    list = pqSMAdaptor::getMultipleElementProperty(prop);
    double s1 = list[0].toDouble();
    double s2 = list[1].toDouble();
    prop = this->LookupTable->GetProperty("ValueRange");
    list = pqSMAdaptor::getMultipleElementProperty(prop);
    double v1 = list[0].toDouble();
    double v2 = list[1].toDouble();
    prop = this->LookupTable->GetProperty("ScalarRange");
    list = pqSMAdaptor::getMultipleElementProperty(prop);
    pqChartValue scalar = list[0].toDouble();
    QColor color = QColor::fromHsvF(h1, s1, v1);
    this->Form->ColorScale->addPoint(scalar, color);
    scalar = list[1].toDouble();
    color = QColor::fromHsvF(h2, s2, v2);
    this->Form->ColorScale->addPoint(scalar, color);
    }
}

void pqColorMapEditor::closeEvent(QCloseEvent *e)
{
  // If the timer is running, cancel it.
  if(this->EditDelay->isActive())
    {
    this->EditDelay->stop();
    }

  QDialog::closeEvent(e);
}

void pqColorMapEditor::handleTextEdit(const QString &text)
{
  // TODO: Validate the text.

  // Start a timer to allow the user to enter more text. If the timer
  // is already running delay it for more text.
  this->EditDelay->start(600);
}

void pqColorMapEditor::setSizeFromText()
{
  // Get the size from the text. Set the size for the slider and the
  // color scale.
  QString text = this->Form->TableSizeText->text();
  int tableSize = text.toInt();
  this->Form->TableSize->setValue(tableSize);
  this->setTableSize(tableSize);
}

void pqColorMapEditor::setSizeFromSlider(int tableSize)
{
  QString sizeString;
  sizeString.setNum(tableSize);
  this->Form->TableSizeText->setText(sizeString);
  this->setTableSize(tableSize);
}

void pqColorMapEditor::setTableSize(int tableSize)
{
  // Set the table size for the color scale and the lookup table.
  this->Form->ColorScale->setTableSize(tableSize);
  if(this->LookupTable)
    {
    vtkSMProperty *prop = this->LookupTable->GetProperty(
        "NumberOfTableValues");
    pqSMAdaptor::setElementProperty(prop, QVariant(tableSize));
    this->LookupTable->UpdateVTKObjects();
    this->Form->Display->renderAllViews();
    }
}

void pqColorMapEditor::changeColor(int, const QColor &)
{
  if(this->LookupTable)
    {
    // Set the colors for the lookup table.
    QColor color1;
    QColor color2;
    this->Form->ColorScale->getPointColor(0, color1);
    this->Form->ColorScale->getPointColor(1, color2);
    QList<QVariant> list;
    list.append(QVariant(color1.hueF()));
    list.append(QVariant(color2.hueF()));
    vtkSMProperty *prop = this->LookupTable->GetProperty("HueRange");
    pqSMAdaptor::setMultipleElementProperty(prop, list);
    list[0] = QVariant(color1.saturationF());
    list[1] = QVariant(color2.saturationF());
    prop = this->LookupTable->GetProperty("SaturationRange");
    pqSMAdaptor::setMultipleElementProperty(prop, list);
    list[0] = QVariant(color1.valueF());
    list[1] = QVariant(color2.valueF());
    prop = this->LookupTable->GetProperty("ValueRange");
    pqSMAdaptor::setMultipleElementProperty(prop, list);
    this->LookupTable->UpdateVTKObjects();
    this->Form->Display->renderAllViews();
    }
}

void pqColorMapEditor::closeForm()
{
  // If the edit delay timer is active, set the final user entry.
  if(this->EditDelay->isActive())
    {
    this->EditDelay->stop();
    this->setSizeFromText();
    }

  this->accept();
}


