/*=========================================================================

   Program: ParaView
   Module:    pqXYChartOptionsEditor.cxx

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

#include "pqXYChartOptionsEditor.h"
#include "ui_pqChartOptionsWidget.h"

#include "pqSampleScalarAddRangeDialog.h"

#include <QAbstractItemDelegate>
#include <QColor>
#include <QFont>
#include <QFontDialog>
#include <QItemSelectionModel>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QStringListModel>

// Use the property links/manager etc
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqXYChartView.h"
#include "pqXYBarChartView.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

#include <math.h>

class pqXYChartOptionsEditorForm;

class pqXYChartOptionsEditor::pqInternal
{
public:
  QPointer<pqXYChartView> XYChartView;
  QPointer<pqXYBarChartView> XYBarChartView;
  pqPropertyManager Links;
  pqXYChartOptionsEditorForm *Form;
};

class pqXYChartOptionsEditorAxis
{
public:
  pqXYChartOptionsEditorAxis();
  ~pqXYChartOptionsEditorAxis() {}

  QColor AxisColor;
  QColor GridColor;
  QColor LabelColor;
  QColor TitleColor;
  QFont LabelFont;
  QFont TitleFont;
  QString Title;
  QString Minimum;
  QString Maximum;
  QStringListModel Labels;
  int GridType;
  int Notation;
  int Precision;
  int AxisLayout;
  int TitleAlignment;
  bool ShowAxis;
  bool ShowGrid;
  bool ShowLabels;
  bool UseLogScale;
};

class pqXYChartOptionsEditorForm : public Ui::pqChartOptionsWidget
{
public:
  pqXYChartOptionsEditorForm();
  ~pqXYChartOptionsEditorForm();

  void setCurrentAxis(const QString &name);
  int getIndexForLocation(vtkQtChartAxis::AxisLocation location) const;

  QString CurrentPage;
  QFont TitleFont;
  pqXYChartOptionsEditorAxis *AxisData[4];
  vtkQtChartAxis::AxisLocation CurrentAxis;
  int AxisIndex;
  pqSampleScalarAddRangeDialog *RangeDialog;
};

//----------------------------------------------------------------------------
pqXYChartOptionsEditorAxis::pqXYChartOptionsEditorAxis()
  : AxisColor(Qt::black), GridColor(Qt::lightGray), LabelColor(Qt::black),
    TitleColor(Qt::black), LabelFont(), TitleFont(), Title(), Minimum(),
    Maximum(), Labels()
{
  this->GridType = 0;
  this->Notation = 3;
  this->Precision = 2;
  this->AxisLayout = 0;
  this->TitleAlignment = 1;
  this->ShowAxis = true;
  this->ShowGrid = true;
  this->ShowLabels = true;
  this->UseLogScale = false;
}

//----------------------------------------------------------------------------
pqXYChartOptionsEditorForm::pqXYChartOptionsEditorForm()
  : Ui::pqChartOptionsWidget(), CurrentPage(), TitleFont()
{
  this->CurrentAxis = vtkQtChartAxis::Left;
  this->AxisIndex = -1;
  this->RangeDialog = 0;

  // Create the axis data objects.
  for(int i = 0; i < 4; i++)
    {
    this->AxisData[i] = new pqXYChartOptionsEditorAxis();
    }
}

pqXYChartOptionsEditorForm::~pqXYChartOptionsEditorForm()
{
  // Clean up the axis data objects.
  for(int i = 0; i < 4; i++)
    {
    delete this->AxisData[i];
    }
}

void pqXYChartOptionsEditorForm::setCurrentAxis(const QString &name)
{
  if(name == "Left Axis")
    {
    this->CurrentAxis = vtkQtChartAxis::Left;
    this->AxisIndex = 0;
    }
  else if(name == "Bottom Axis")
    {
    this->CurrentAxis = vtkQtChartAxis::Bottom;
    this->AxisIndex = 1;
    }
  else if(name == "Right Axis")
    {
    this->CurrentAxis = vtkQtChartAxis::Right;
    this->AxisIndex = 2;
    }
  else if(name == "Top Axis")
    {
    this->CurrentAxis = vtkQtChartAxis::Top;
    this->AxisIndex = 3;
    }
  else
    {
    this->CurrentAxis = vtkQtChartAxis::Left;
    this->AxisIndex = -1;
    }
}

int pqXYChartOptionsEditorForm::getIndexForLocation(
    vtkQtChartAxis::AxisLocation location) const
{
  switch(location)
    {
    case vtkQtChartAxis::Bottom:
      {
      return 1;
      }
    case vtkQtChartAxis::Right:
      {
      return 2;
      }
    case vtkQtChartAxis::Top:
      {
      return 3;
      }
    case vtkQtChartAxis::Left:
    default:
      {
      return 0;
      }
    }
}

//----------------------------------------------------------------------------
pqXYChartOptionsEditor::pqXYChartOptionsEditor(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->Form = new pqXYChartOptionsEditorForm();
  this->Internal->Form->setupUi(this);

  // Connect up some of the form elements
  QObject::connect(this->Internal->Form->UseChartSelect, SIGNAL(toggled(bool)),
                   this, SLOT(changeLayoutPage(bool)));
  QObject::connect(this->Internal->Form->UseBestFit, SIGNAL(toggled(bool)),
                   this, SLOT(changeLayoutPage(bool)));
  QObject::connect(this->Internal->Form->UseFixedInterval, SIGNAL(toggled(bool)),
                   this, SLOT(changeLayoutPage(bool)));
  QObject::connect(this->Internal->Form->ShowAxis, SIGNAL(toggled(bool)),
                   this, SLOT(setAxisVisibility(bool)));
  QObject::connect(this->Internal->Form->ShowAxisGrid, SIGNAL(toggled(bool)),
                   this, SLOT(setGridVisibility(bool)));
  QObject::connect(this->Internal->Form->UseLogScale, SIGNAL(toggled(bool)),
                   this, SLOT(setUsingLogScale(bool)));

  // Connect up some signals and slots for the property links
  QObject::connect(&this->Internal->Links, SIGNAL(modified()),
                   this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->Form->ChartTitle, SIGNAL(textEdited(QString)),
                   this, SIGNAL(changesAvailable()));

}

pqXYChartOptionsEditor::~pqXYChartOptionsEditor()
{
  delete this->Internal->Form;
  delete this->Internal;
}

void pqXYChartOptionsEditor::setView(pqView* view)
{
  this->disconnectGUI();
  this->Internal->XYChartView = qobject_cast<pqXYChartView *>(view);
  this->Internal->XYBarChartView = qobject_cast<pqXYBarChartView *>(view);
  if (this->Internal->XYChartView || this->Internal->XYBarChartView)
    {
    this->connectGUI();
    }
}

pqView* pqXYChartOptionsEditor::getView()
{
  if (this->Internal->XYChartView)
    {
    return this->Internal->XYChartView;
    }
  else
    {
    return this->Internal->XYBarChartView;
    }
}

void pqXYChartOptionsEditor::setPage(const QString &page)
{
  if(this->Internal->Form->CurrentPage == page)
    {
    return;
    }

  this->Internal->Form->CurrentPage = page;
  this->Internal->Form->AxisIndex = -1;

  // Split the page path into its components. Use the page path to
  // determine which widget to show.
  QWidget *widget = 0;
  QStringList path = page.split(".", QString::SkipEmptyParts);
  if(path[0] == "General")
    {
    widget = this->Internal->Form->General;
    }
  else
    {
    // Use the axis location string to set the current axis.
    this->Internal->Form->setCurrentAxis(path[0]);

    // Load the axis data for the specified axis.
    if(path.size() == 1)
      {
      widget = this->Internal->Form->Axis;
      this->loadAxisPage();

      // Set the page label to the current axis name.
      this->Internal->Form->AxisGeneralLabel->setText(path[0]);
      }
    else if(path[1] == "Layout")
      {
      widget = this->Internal->Form->AxisLayout;
      this->loadAxisLayoutPage();
      }
    else if(path[1] == "Title")
      {
      widget = this->Internal->Form->AxisTitle;
      this->loadAxisTitlePage();
      }
    }

  if(widget)
    {
    this->Internal->Form->ChartPages->setCurrentWidget(widget);
    }
}

QStringList pqXYChartOptionsEditor::getPageList()
{
  QStringList pages;
  pages.append("General");
  pages.append("Left Axis");
  pages.append("Left Axis.Layout");
  pages.append("Left Axis.Title");
  pages.append("Bottom Axis");
  pages.append("Bottom Axis.Layout");
  pages.append("Bottom Axis.Title");
  pages.append("Right Axis");
  pages.append("Right Axis.Layout");
  pages.append("Right Axis.Title");
  pages.append("Top Axis");
  pages.append("Top Axis.Layout");
  pages.append("Top Axis.Title");
  return pages;
}

void pqXYChartOptionsEditor::applyChanges()
{
  if (!this->Internal->XYChartView && !this->Internal->XYBarChartView)
    {
    return;
    }

  this->Internal->Links.accept();
  this->applyAxisOptions();
}

void pqXYChartOptionsEditor::resetChanges()
{
  this->Internal->Links.reject();
}

void pqXYChartOptionsEditor::connectGUI()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return;
    }

  this->blockSignals(true);

  this->Internal->Links.registerLink(this->Internal->Form->ChartTitle,
                                     "text",
                                     SIGNAL(editingFinished()),
                                     proxy, proxy->GetProperty("ChartTitle"));
  this->Internal->Links.registerLink(this->Internal->Form->ChartTitleAlignment,
                                     "currentIndex",
                                     SIGNAL(currentIndexChanged(int)),
                                     proxy,
                                     proxy->GetProperty("ChartTitleAlignment"));

  this->Internal->Links.registerLink(this->Internal->Form->ShowLegend,
                                     "checked",
                                     SIGNAL(toggled(bool)),
                                     proxy, proxy->GetProperty("ShowLegend"));

  this->updateOptions();

  this->blockSignals(false);
}

void pqXYChartOptionsEditor::disconnectGUI()
{
  this->Internal->Links.removeAllLinks();
}

void pqXYChartOptionsEditor::setAxisVisibility(bool visible)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->ShowAxis
        = visible;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setGridVisibility(bool visible)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->ShowGrid
        = visible;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setUsingLogScale(bool usingLogScale)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->UseLogScale
        = usingLogScale;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::updateOptions()
{
  // Use the server properties to update the options on the charts
  QList<QVariant> values;
  vtkSMProxy *proxy = this->getProxy();
  this->blockSignals(true);

  // Get the legend parameters.
  this->Internal->Form->ShowLegend->setChecked(pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ShowLegend")).toInt() != 0);

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->updateDescription(this->Internal->Form->ChartTitleFont,
                            QFont(values[0].toString(), values[1].toInt(),
                                  values[2].toInt() != 0 ? QFont::Bold : -1,
                                  values[3].toInt() != 0));
    }
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    this->Internal->Form->ChartTitleColor->setChosenColor(
        QColor::fromRgbF(values[0].toDouble(), values[1].toDouble(),
                         values[2].toDouble()));
    }

  this->Internal->Form->ChartTitleAlignment->setCurrentIndex(
      pqSMAdaptor::getElementProperty(
          proxy->GetProperty("ChartTitleAlignment")).toInt());

  // Get the general axis parameters.
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxis"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->ShowAxis = values[i].toInt() != 0;
    }
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisGrid"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->ShowGrid = values[i].toInt() != 0;
    }

  this->blockSignals(false);

}

void pqXYChartOptionsEditor::applyAxisOptions()
{
  // Apply updated axis options to the server properties
  QList<QVariant> values;
  vtkSMProxy *proxy = this->getProxy();

  // Axis visibility
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->ShowAxis);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ShowAxis"), values);

  // Show axis grid lines
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->ShowGrid);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ShowAxisGrid"), values);

  // Axis use log scale
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->UseLogScale);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLogScale"), values);
}

void pqXYChartOptionsEditor::loadAxisPage()
{
  this->blockSignals(true);
  pqXYChartOptionsEditorAxis *axis =
          this->Internal->Form->AxisData[this->Internal->Form->AxisIndex];
  this->Internal->Form->ShowAxis->setChecked(axis->ShowAxis);
  this->Internal->Form->ShowAxisGrid->setChecked(axis->ShowGrid);
  this->Internal->Form->GridType->setHidden(true);
  this->Internal->Form->label_18->setHidden(true);
  this->Internal->Form->AxisColor->setChosenColor(axis->AxisColor);
  this->Internal->Form->GridColor->setChosenColor(axis->GridColor);
  this->Internal->Form->ShowAxisLabels->setChecked(axis->ShowLabels);
  this->updateDescription(this->Internal->Form->AxisLabelFont, axis->LabelFont);
  this->Internal->Form->LabelColor->setChosenColor(axis->LabelColor);
  this->Internal->Form->LabelNotation->setCurrentIndex(axis->Notation);
  this->Internal->Form->LabelPrecision->setValue(axis->Precision);
  this->blockSignals(false);
}

void pqXYChartOptionsEditor::loadAxisLayoutPage()
{
  this->blockSignals(true);
  pqXYChartOptionsEditorAxis *axis =
          this->Internal->Form->AxisData[this->Internal->Form->AxisIndex];
  this->Internal->Form->UseLogScale->setChecked(axis->UseLogScale);
  if(axis->AxisLayout == 0)
    {
    this->Internal->Form->UseChartSelect->setChecked(true);
    }
  else if(axis->AxisLayout == 1)
    {
    this->Internal->Form->UseBestFit->setChecked(true);
    }
  else
    {
    this->Internal->Form->UseFixedInterval->setChecked(true);
    }
  this->changeLayoutPage(true);
  this->Internal->Form->AxisMinimum->setText(axis->Minimum);
  this->Internal->Form->AxisMaximum->setText(axis->Maximum);
  QItemSelectionModel *model =
          this->Internal->Form->LabelList->selectionModel();
  if(model)
    {
    this->disconnect(model, 0, this, 0);
    }

  this->Internal->Form->LabelList->setModel(&axis->Labels);
  this->connect(this->Internal->Form->LabelList->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                this, SLOT(updateRemoveButton()));
  this->updateRemoveButton();
  this->blockSignals(false);
}

void pqXYChartOptionsEditor::loadAxisTitlePage()
{
  this->blockSignals(true);
  pqXYChartOptionsEditorAxis *axis =
          this->Internal->Form->AxisData[this->Internal->Form->AxisIndex];
  this->Internal->Form->AxisTitleText->setText(axis->Title);
  this->updateDescription(this->Internal->Form->AxisTitleFont, axis->TitleFont);
  this->Internal->Form->AxisTitleColor->setChosenColor(axis->TitleColor);
  this->Internal->Form->AxisTitleAlignment->setCurrentIndex(axis->TitleAlignment);
  this->blockSignals(false);
}

void pqXYChartOptionsEditor::changeLayoutPage(bool checked)
{
  if(checked && this->Internal->Form->AxisIndex != -1)
    {
    // Change the axis layout stack page when the user picks an option.
    pqXYChartOptionsEditorAxis *axis =
        this->Internal->Form->AxisData[this->Internal->Form->AxisIndex];
    if(this->Internal->Form->UseBestFit->isChecked())
      {
      this->Internal->Form->AxisLayoutPages->setCurrentWidget(
              this->Internal->Form->RangePage);
      axis->AxisLayout = 1;
      }
    else if(this->Internal->Form->UseFixedInterval->isChecked())
      {
      this->Internal->Form->AxisLayoutPages->setCurrentWidget(
              this->Internal->Form->ListPage);
      axis->AxisLayout = 2;
      }
    else
      {
      this->Internal->Form->AxisLayoutPages->setCurrentWidget(
              this->Internal->Form->BlankPage);
      axis->AxisLayout = 0;
      }
    }
}

void pqXYChartOptionsEditor::updateRemoveButton()
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    QItemSelectionModel *model = this->Internal->Form->LabelList->selectionModel();
    this->Internal->Form->RemoveButton->setEnabled(model->hasSelection());
    }
}

void pqXYChartOptionsEditor::updateDescription(QLabel *label,
                                               const QFont &newFont)
{
  QString description = newFont.family();
  description.append(", ").append(QString::number(newFont.pointSize()));
  if(newFont.bold())
    {
    description.append(", bold");
    }

  if(newFont.italic())
    {
    description.append(", italic");
    }

  label->setText(description);
}

vtkSMProxy* pqXYChartOptionsEditor::getProxy()
{
  if (this->Internal->XYChartView)
    {
    return this->Internal->XYChartView->getProxy();
    }
  else if (this->Internal->XYBarChartView)
    {
    return this->Internal->XYBarChartView->getProxy();
    }
  return 0;
}
