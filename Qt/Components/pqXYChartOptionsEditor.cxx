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
#include <QDebug>

// Use the property links/manager etc
#include "pqComparativeXYBarChartView.h"
#include "pqComparativeXYChartView.h"
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqXYBarChartView.h"
#include "pqXYChartView.h"

#include <math.h>

class pqXYChartOptionsEditorForm;

class pqXYChartOptionsEditor::pqInternal
{
public:
  pqPropertyManager Links;
  pqXYChartOptionsEditorForm *Form;
  QPointer<pqView> View;

  enum ChartType
    {
    INVALID,
    LINE,
    BAR
    };

  ChartType Type;
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
  this->Notation = 0;
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
  this->Internal->Type = pqInternal::INVALID;
  this->Internal->Form = new pqXYChartOptionsEditorForm();
  this->Internal->Form->setupUi(this);

  // Adjust a few of the form elements
  this->Internal->Form->GridType->setHidden(true);
  this->Internal->Form->label_18->setHidden(true);
  this->Internal->Form->label_25->setHidden(true);
  this->Internal->Form->LabelNotation->clear();
  this->Internal->Form->LabelNotation->addItem("Mixed");
  this->Internal->Form->LabelNotation->addItem("Scientific");
  this->Internal->Form->LabelNotation->addItem("Fixed");
  this->Internal->Form->UseFixedInterval->setHidden(true);
  this->Internal->Form->label_12->setHidden(true);
  this->Internal->Form->AxisTitleAlignment->setHidden(true);

  // Connect up some of the form elements
  QObject::connect(this->Internal->Form->ChartTitleFontButton,
                   SIGNAL(clicked()), this, SLOT(pickTitleFont()));
  QObject::connect(this->Internal->Form->ChartTitleColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SIGNAL(changesAvailable()));
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
  QObject::connect(this->Internal->Form->AxisColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SLOT(setAxisColor(QColor)));
  QObject::connect(this->Internal->Form->GridColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SLOT(setGridColor(QColor)));
  QObject::connect(this->Internal->Form->ShowAxisLabels, SIGNAL(toggled(bool)),
                   this, SLOT(setLabelVisibility(bool)));
  QObject::connect(this->Internal->Form->AxisLabelFontButton,
                   SIGNAL(clicked()), this, SLOT(pickLabelFont()));
  QObject::connect(this->Internal->Form->LabelColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SLOT(setAxisLabelColor(QColor)));
  QObject::connect(this->Internal->Form->LabelNotation,
                   SIGNAL(currentIndexChanged(int)),
                   this, SLOT(setLabelNotation(int)));
  QObject::connect(this->Internal->Form->LabelPrecision,
                   SIGNAL(valueChanged(int)),
                   this, SLOT(setLabelPrecision(int)));

  QObject::connect(this->Internal->Form->AxisMinimum, SIGNAL(textEdited(QString)),
                   this, SLOT(setAxisMinimum()));
  QObject::connect(this->Internal->Form->AxisMaximum, SIGNAL(textEdited(QString)),
                   this, SLOT(setAxisMaximum()));

  QObject::connect(this->Internal->Form->UseLogScale, SIGNAL(toggled(bool)),
                   this, SLOT(setUsingLogScale(bool)));

  // Axis title
  QObject::connect(this->Internal->Form->AxisTitleText,
                   SIGNAL(textChanged(QString)),
                   this, SLOT(setAxisTitle(QString)));
  QObject::connect(this->Internal->Form->AxisTitleFontButton,
                   SIGNAL(clicked()), this, SLOT(pickAxisTitleFont()));
  QObject::connect(this->Internal->Form->AxisTitleColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SLOT(setAxisTitleColor(QColor)));

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
  if (qobject_cast<pqXYChartView*>(view) ||
    qobject_cast<pqComparativeXYChartView*>(view))
    {
    this->Internal->Type = pqInternal::LINE;
    }
  else if (qobject_cast<pqXYBarChartView*>(view) ||
    qobject_cast<pqComparativeXYBarChartView*>(view))
    {
    this->Internal->Type = pqInternal::BAR;
    }
  else
    {
    this->Internal->Type = pqInternal::INVALID;
    }

  this->Internal->View = NULL;
  if (this->Internal->Type != pqInternal::INVALID)
    {
    this->Internal->View = view;
    this->connectGUI();
    this->setPage(this->Internal->Form->CurrentPage);
    }
}

pqView* pqXYChartOptionsEditor::getView()
{
  return this->Internal->View;
}

void pqXYChartOptionsEditor::setPage(const QString &page)
{
  if (page.isEmpty())
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
  if (this->Internal->Type == pqInternal::LINE)
    {
    pages.append("Right Axis");
    pages.append("Right Axis.Layout");
    pages.append("Right Axis.Title");
    pages.append("Top Axis");
    pages.append("Top Axis.Layout");
    pages.append("Top Axis.Title");
    }
  return pages;
}

void pqXYChartOptionsEditor::applyChanges()
{
  if (this->Internal->Type == pqInternal::INVALID)
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

void pqXYChartOptionsEditor::setAxisColor(const QColor& color)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->AxisColor
        = color;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setGridColor(const QColor& color)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->GridColor
        = color;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setLabelVisibility(bool visible)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->ShowLabels
        = visible;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::pickLabelFont()
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->pickFont(this->Internal->Form->AxisLabelFont,
        this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->LabelFont);
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setAxisLabelColor(const QColor& color)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->LabelColor
        = color;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setLabelNotation(int notation)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->Notation
        = notation;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setLabelPrecision(int precision)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->Precision
        = precision;
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

void pqXYChartOptionsEditor::setAxisMinimum()
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->Minimum
        = this->Internal->Form->AxisMinimum->text();
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setAxisMaximum()
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->Maximum
        = this->Internal->Form->AxisMaximum->text();
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::pickTitleFont()
{
  this->pickFont(this->Internal->Form->ChartTitleFont,
                 this->Internal->Form->TitleFont);
}

void pqXYChartOptionsEditor::pickAxisTitleFont()
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->pickFont(this->Internal->Form->AxisTitleFont,
      this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->TitleFont);
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setAxisTitleColor(const QColor& color)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->TitleColor
        = color;
    this->changesAvailable();
    }
}

void pqXYChartOptionsEditor::setAxisTitle(const QString& text)
{
  if(this->Internal->Form->AxisIndex != -1)
    {
    this->Internal->Form->AxisData[this->Internal->Form->AxisIndex]->Title
        = text;
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
    this->Internal->Form->TitleFont = QFont(values[0].toString(), values[1].toInt(),
                                            values[2].toInt() != 0 ? QFont::Bold : -1,
                                            values[3].toInt() != 0);
    this->updateDescription(this->Internal->Form->ChartTitleFont,
                            this->Internal->Form->TitleFont);
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
      proxy->GetProperty("AxisLogScale"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->UseLogScale = values[i].toInt() != 0;
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisGrid"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->ShowGrid = values[i].toInt() != 0;
    }
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisColor"));
  if(values.size() == 12)
    {
    for(int i = 0; i < 4; ++i)
      {
      this->Internal->Form->AxisData[i]->AxisColor = QColor::fromRgbF(
          values[3*i].toDouble(), values[3*i + 1].toDouble(),
          values[3*i + 2].toDouble());
      }
    }
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"));
  if(values.size() == 12)
    {
    for(int i = 0; i < 4; ++i)
      {
      this->Internal->Form->AxisData[i]->GridColor = QColor::fromRgbF(
          values[3*i].toDouble(), values[3*i + 1].toDouble(),
          values[3*i + 2].toDouble());
      }
    }

  // Axis label parameters
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisLabels"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->ShowLabels = values[i].toInt() != 0;
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"));
  if (values.size() == 16)
    {
    for(int i = 0; i < 4; ++i)
      {
      int j = 4*i;
      this->Internal->Form->AxisData[i]->LabelFont =
          QFont(values[j].toString(), values[j + 1].toInt(),
                values[j + 2].toInt() != 0 ? QFont::Bold : -1,
                values[j + 3].toInt() != 0);
      }
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"));
  if (values.size() == 12)
    {
    for(int i = 0; i < 4; ++i)
      {
      this->Internal->Form->AxisData[i]->LabelColor = QColor::fromRgbF(
          values[3*i].toDouble(), values[3*i + 1].toDouble(),
          values[3*i + 2].toDouble());
      }
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelNotation"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->Notation = values[i].toInt();
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelPrecision"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->Precision = values[i].toInt();
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisBehavior"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->AxisLayout = values[i].toInt();
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisRange"));
  for(int i = 0; i < 4 && values.size() == 8; ++i)
    {
    this->Internal->Form->AxisData[i]->Minimum = values[2*i].toString();
    this->Internal->Form->AxisData[i]->Maximum = values[2*i+1].toString();
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitle"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->Form->AxisData[i]->Title = values[i].toString();
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"));
  if (values.size() == 16)
    {
    for(int i = 0; i < 4; ++i)
      {
      int j = 4*i;
      this->Internal->Form->AxisData[i]->TitleFont =
          QFont(values[j].toString(), values[j + 1].toInt(),
                values[j + 2].toInt() != 0 ? QFont::Bold : -1,
                values[j + 3].toInt() != 0);
      }
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"));
  if (values.size() == 12)
    {
    for(int i = 0; i < 4; ++i)
      {
      this->Internal->Form->AxisData[i]->TitleColor = QColor::fromRgbF(
          values[3*i].toDouble(), values[3*i + 1].toDouble(),
          values[3*i + 2].toDouble());
      }
    }

  this->blockSignals(false);

}

void pqXYChartOptionsEditor::applyAxisOptions()
{
  // Apply updated axis options to the server properties
  QList<QVariant> values;
  vtkSMProxy *proxy = this->getProxy();

  // Apply the font type info
  values.clear();
  values.append(QVariant(this->Internal->Form->TitleFont.family()));
  values.append(QVariant(this->Internal->Form->TitleFont.pointSize()));
  values.append(QVariant(this->Internal->Form->TitleFont.bold() ? 1 : 0));
  values.append(QVariant(this->Internal->Form->TitleFont.italic() ? 1 : 0));
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"), values);

  // The chart title color
  QColor color = this->Internal->Form->ChartTitleColor->chosenColor();
  values.clear();
  values.append(QVariant(static_cast<double>(color.redF())));
  values.append(QVariant(static_cast<double>(color.greenF())));
  values.append(QVariant(static_cast<double>(color.blueF())));
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ChartTitleColor"), values);

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

  // Axis color
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    color = this->Internal->Form->AxisData[i]->AxisColor;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisColor"), values);

  // Axis color
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    color = this->Internal->Form->AxisData[i]->GridColor;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"), values);

  // Axis label visibility
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->ShowLabels);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ShowAxisLabels"), values);

  // Label color
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    color = this->Internal->Form->AxisData[i]->LabelColor;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"), values);

  // Axis label font
  values.clear();
  for (int i = 0; i < 4; ++i)
    {
    QFont font = this->Internal->Form->AxisData[i]->LabelFont;
    values.append(QVariant(font.family()));
    values.append(QVariant(font.pointSize()));
    values.append(QVariant(font.bold() ? 1 : 0));
    values.append(QVariant(font.italic() ? 1 : 0));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"), values);

  // Axis label notation
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->Notation);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelNotation"), values);

  // Axis label precision
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->Precision);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelPrecision"), values);

  // Axis behavior
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->AxisLayout);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisBehavior"), values);

  // Axis range
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->Minimum);
    values.append(this->Internal->Form->AxisData[i]->Maximum);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisRange"), values);

  // Axis use log scale
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(this->Internal->Form->AxisData[i]->UseLogScale);
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLogScale"), values);

  // Axis title
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    values.append(QVariant(this->Internal->Form->AxisData[i]->Title));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitle"), values);

  // Axis title color
  values.clear();
  for(int i = 0; i < 4; ++i)
    {
    color = this->Internal->Form->AxisData[i]->TitleColor;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"), values);

  // Axis title font
  values.clear();
  for (int i = 0; i < 4; ++i)
    {
    QFont font = this->Internal->Form->AxisData[i]->TitleFont;
    values.append(QVariant(font.family()));
    values.append(QVariant(font.pointSize()));
    values.append(QVariant(font.bold() ? 1 : 0));
    values.append(QVariant(font.italic() ? 1 : 0));
    }
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"), values);
}

void pqXYChartOptionsEditor::loadAxisPage()
{
  this->blockSignals(true);
  pqXYChartOptionsEditorAxis *axis =
          this->Internal->Form->AxisData[this->Internal->Form->AxisIndex];
  this->Internal->Form->ShowAxis->setChecked(axis->ShowAxis);
  this->Internal->Form->ShowAxisGrid->setChecked(axis->ShowGrid);
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
    this->changesAvailable();
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

bool pqXYChartOptionsEditor::pickFont(QLabel *label, QFont &font)
{
  bool ok = false;
  font = QFontDialog::getFont(&ok, font, this);
  if(ok)
    {
    this->updateDescription(label, font);
    this->changesAvailable();
    return true;
    }
  else
    {
    return false;
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
  return (this->getView()?  this->getView()->getProxy() : NULL);
}
