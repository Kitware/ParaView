/*=========================================================================

   Program: ParaView
   Module:    pqPlotMatrixOptionsEditor.cxx

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

#include "pqPlotMatrixOptionsEditor.h"
#include "ui_pqPlotMatrixOptionsWidget.h"

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
#include "vtkSMPlotMatrixViewProxy.h"
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqPlotMatrixView.h"

#include "vtkAxis.h"
#include "vtkVector.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTextProperty.h"

#include <cmath>

class pqPlotMatrixOptionsEditorForm;

class pqPlotMatrixOptionsEditor::pqInternal
{
public:
  pqPlotMatrixOptionsEditorForm *Form;
  QPointer<pqView> View;

};

class pqPlotMatrixOptionsChartSetting
{
public:
  pqPlotMatrixOptionsChartSetting();
  ~pqPlotMatrixOptionsChartSetting() {}

  QColor BackGroundColor;
  QColor AxisColor;
  QColor GridColor;
  QColor LabelColor;
  QFont LabelFont;
  int Notation;
  int Precision;
  int ToolTipNotation;
  int ToolTipPrecision;
  bool ShowGrid;
  bool ShowLabels;
};

class pqPlotMatrixOptionsEditorForm : public Ui::pqPlotMatrixOptionsWidget
{
public:
  pqPlotMatrixOptionsEditorForm();
  ~pqPlotMatrixOptionsEditorForm();

  void setCurrentPlot(const QString &name);
  int getIndexForLocation(int location) const;

  QString CurrentPage;
  vtkVector2f Gutter;
  
  //Title properties
  QFont TitleFont;
  int TitleAlignment;
  QString Title;
  QColor TitleColor;
  QColor SelectedActiveScatterChartBGColor;
  QColor SelectedRowColumnScatterChartBGColor;
  
  QMap<int, pqPlotMatrixOptionsChartSetting*> PlotData;
  int CurrentPlot;
  int Borders[4];
};

//----------------------------------------------------------------------------
pqPlotMatrixOptionsChartSetting::pqPlotMatrixOptionsChartSetting()
  : BackGroundColor(Qt::white),AxisColor(Qt::black), GridColor(Qt::lightGray),
    LabelColor(Qt::black), LabelFont("Arial", 12)
{ 
  this->Notation = 0;
  this->Precision = 2;
  this->ToolTipNotation = 0;
  this->ToolTipPrecision = 2;
  this->ShowGrid = true;
  this->ShowLabels = true;
  
}

//----------------------------------------------------------------------------
pqPlotMatrixOptionsEditorForm::pqPlotMatrixOptionsEditorForm()
  : Ui::pqPlotMatrixOptionsWidget(), CurrentPage("General"),
   Gutter(15.0, 15.0), TitleFont("Arial", 12)
{
  this->CurrentPlot = vtkScatterPlotMatrix::NOPLOT;
  // create the plot data objects
  this->PlotData[vtkScatterPlotMatrix::ACTIVEPLOT] = new pqPlotMatrixOptionsChartSetting();
  this->PlotData[vtkScatterPlotMatrix::SCATTERPLOT] = new pqPlotMatrixOptionsChartSetting();
  this->PlotData[vtkScatterPlotMatrix::HISTOGRAM] = new pqPlotMatrixOptionsChartSetting();
  
  this->PlotData[vtkScatterPlotMatrix::ACTIVEPLOT]->BackGroundColor.fromRgbF(1.0, 1.0, 1.0, 0.0);
  this->PlotData[vtkScatterPlotMatrix::SCATTERPLOT]->BackGroundColor.fromRgbF(1.0, 1.0, 1.0, 0.0);
  this->PlotData[vtkScatterPlotMatrix::HISTOGRAM]->BackGroundColor.fromRgbF(0.5, 0.5, 0.5, 0.4);

  this->SelectedActiveScatterChartBGColor.fromRgbF(0, 0.8, 0, 0.4);
  this->SelectedRowColumnScatterChartBGColor.fromRgbF(0.8, 0, 0, 0.4);

  this->Borders[vtkAxis::LEFT] = 50;
  this->Borders[vtkAxis::BOTTOM] = 40;
  this->Borders[vtkAxis::RIGHT] = 50;
  this->Borders[vtkAxis::TOP] = 40;
}

pqPlotMatrixOptionsEditorForm::~pqPlotMatrixOptionsEditorForm()
{
  // Clean up the plot data objects.
  delete this->PlotData[vtkScatterPlotMatrix::ACTIVEPLOT];
  delete this->PlotData[vtkScatterPlotMatrix::SCATTERPLOT];
  delete this->PlotData[vtkScatterPlotMatrix::HISTOGRAM];
}

//----------------------------------------------------------------------------
pqPlotMatrixOptionsEditor::pqPlotMatrixOptionsEditor(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->Form = new pqPlotMatrixOptionsEditorForm();
  this->Internal->Form->setupUi(this);

  // Adjust a few of the form elements
  this->Internal->Form->LabelNotation->clear();
  this->Internal->Form->LabelNotation->addItem("Mixed");
  this->Internal->Form->LabelNotation->addItem("Scientific");
  this->Internal->Form->LabelNotation->addItem("Fixed");

  // Connect up some of the form elements
  QObject::connect(this->Internal->Form->ChartTitleFontButton,
                   SIGNAL(clicked()), this, SLOT(pickTitleFont()));
  QObject::connect(this->Internal->Form->ChartTitleColor,
                   SIGNAL(chosenColorChanged(QColor)),
                   this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->ChartTitleAlignment,
    SIGNAL(currentIndexChanged(int)), this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->Form->ShowAxisGrid, SIGNAL(toggled(bool)),
                   this, SLOT(setGridVisibility(bool)));
                   
  QObject::connect(this->Internal->Form->BackgroundColor,
                  SIGNAL(chosenColorChanged(QColor)),
                  this, SLOT(setChartBackgroundColor(QColor)));
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
  QObject::connect(this->Internal->Form->TooltipNotation,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(setToolTipNotation(int)));
  QObject::connect(this->Internal->Form->TooltipPrecision,
    SIGNAL(valueChanged(int)),
    this, SLOT(setToolTipPrecision(int)));

  // Connect up some signals and slots for the property links
  // These should really be cached locally
  QObject::connect(this->Internal->Form->ChartTitle, SIGNAL(textChanged(QString)),
                   this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->LeftMargin, SIGNAL(valueChanged(int)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->BottomMargin, SIGNAL(valueChanged(int)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->TopMargin, SIGNAL(valueChanged(int)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->RightMargin, SIGNAL(valueChanged(int)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->GutterX, SIGNAL(valueChanged(double)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->GutterY, SIGNAL(valueChanged(double)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->selRowColBackgroundColor, 
    SIGNAL(chosenColorChanged(QColor)),
    this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Form->selActiveBackgroundColor,
    SIGNAL(chosenColorChanged(QColor)),
    this, SIGNAL(changesAvailable()));

}

pqPlotMatrixOptionsEditor::~pqPlotMatrixOptionsEditor()
{
  delete this->Internal->Form;
  delete this->Internal;
}

void pqPlotMatrixOptionsEditor::setView(pqView* view)
{
  if (!view || !qobject_cast<pqPlotMatrixView*>(view))
    {
    return;
    }

  this->Internal->View = view;
  this->connectGUI();
  this->setPage(this->Internal->Form->CurrentPage);
}

pqView* pqPlotMatrixOptionsEditor::getView()
{
  return this->Internal->View;
}

void pqPlotMatrixOptionsEditor::setPage(const QString &page)
{
  if (page.isEmpty())
    {
    return;
    }

  this->Internal->Form->CurrentPage = page;

  // Split the page path into its components. Use the page path to
  // determine which widget to show.
  QWidget *widget = 0;
  QStringList path = page.split(".", QString::SkipEmptyParts);
  this->Internal->Form->CurrentPlot = vtkScatterPlotMatrix::NOPLOT;

  this->Internal->Form->frameScatterPlot->setVisible(0);
  if(path[0] == "General")
    {
    widget = this->Internal->Form->GeneralPlot;
    this->Internal->Form->CurrentPlot = vtkScatterPlotMatrix::NOPLOT;
    }
  else
    {
    widget = this->Internal->Form->ActivePlot;
    
    if(path[0] == "Active Plot")
      {
      this->Internal->Form->CurrentPlot = vtkScatterPlotMatrix::ACTIVEPLOT;
      }
    else if(path[0] == "Scatter Plots")
      {
      this->Internal->Form->frameScatterPlot->setVisible(1);
      this->Internal->Form->CurrentPlot = vtkScatterPlotMatrix::SCATTERPLOT;
      }
    else if(path[0] == "Histogram Plots")
      {
      this->Internal->Form->CurrentPlot = vtkScatterPlotMatrix::HISTOGRAM;
      }
    }

  if(widget)
    {
    this->Internal->Form->labelGeneral->setText(path[0]);
    this->Internal->Form->ChartPages->setCurrentWidget(widget);
    this->loadChartPage();
    }
}

QStringList pqPlotMatrixOptionsEditor::getPageList()
{
  QStringList pages;
  pages.append("General");
  pages.append("Active Plot");
  pages.append("Scatter Plots");
  pages.append("Histogram Plots");
  return pages;
}

void pqPlotMatrixOptionsEditor::applyChanges()
{
  this->applyChartOptions();
}

void pqPlotMatrixOptionsEditor::connectGUI()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return;
    }

  this->blockSignals(true);

  // TODO
  this->updateOptions();

  this->blockSignals(false);
}

void pqPlotMatrixOptionsEditor::resetChanges()
{
  this->updateOptions();
  this->loadChartPage();
}

void pqPlotMatrixOptionsEditor::setGridVisibility(bool visible)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->ShowGrid
        = visible;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setChartBackgroundColor(const QColor& color)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->BackGroundColor
        = color;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setAxisColor(const QColor& color)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->AxisColor
        = color;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setGridColor(const QColor& color)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->GridColor
        = color;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setLabelVisibility(bool visible)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->ShowLabels
        = visible;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::pickLabelFont()
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->pickFont(this->Internal->Form->AxisLabelFont,
        this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->LabelFont);
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setAxisLabelColor(const QColor& color)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->LabelColor
        = color;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setLabelNotation(int notation)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->Notation
        = notation;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setLabelPrecision(int precision)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->Precision
        = precision;
    this->changesAvailable();
    }
}
void pqPlotMatrixOptionsEditor::setToolTipNotation(int notation)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->ToolTipNotation
        = notation;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::setToolTipPrecision(int precision)
{
  if(this->Internal->Form->CurrentPlot != vtkScatterPlotMatrix::NOPLOT)
    {
    this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot]->ToolTipPrecision
        = precision;
    this->changesAvailable();
    }
}

void pqPlotMatrixOptionsEditor::pickTitleFont()
{
  this->pickFont(this->Internal->Form->ChartTitleFont,
                 this->Internal->Form->TitleFont);
}

void pqPlotMatrixOptionsEditor::updateOptions()
{
  vtkSMPlotMatrixViewProxy *smproxy = vtkSMPlotMatrixViewProxy::SafeDownCast(
    this->getProxy());
  if(!smproxy)
    {
    return;
    }
  vtkScatterPlotMatrix* proxy = vtkScatterPlotMatrix::SafeDownCast(
    smproxy->GetContextItem());
  if(!proxy)
    {
    return;
    } 
  // TODO: Update GUI from proxy
  this->blockSignals(true);

  // Use the server properties to update the options on the charts
  this->Internal->Form->Title = proxy->GetTitle();
  vtkTextProperty *prop = proxy->GetTitleProperties();
  this->Internal->Form->TitleFont = 
    QFont(prop->GetFontFamilyAsString(),
          prop->GetFontSize(),
          prop->GetBold() ? QFont::Bold : -1,
          prop->GetItalic());
  this->updateDescription(this->Internal->Form->ChartTitleFont,
                          this->Internal->Form->TitleFont);
  double rgba[4];
  prop->GetColor(rgba);
  rgba[3] = prop->GetOpacity();
  this->Internal->Form->TitleColor = QColor::fromRgbF(rgba[0], rgba[1],
                                                     rgba[2], rgba[2]);
  this->Internal->Form->TitleAlignment = prop->GetJustification();
 
  vtkColor4ub color;
  color = proxy->GetScatterPlotSelectedRowColumnColor();
  this->Internal->Form->SelectedRowColumnScatterChartBGColor =
    QColor::fromRgb(color[0], color[1], color[2], color[3]);
  color = proxy->GetScatterPlotSelectedActiveColor();
  this->Internal->Form->SelectedActiveScatterChartBGColor =
    QColor::fromRgb(color[0], color[1], color[2], color[3]);
  
  vtkVector2f gutter;
  gutter = proxy->GetGutter();
  this->Internal->Form->Gutter.Set(gutter[0], gutter[1]);
  
  int borders[4];
  proxy->GetBorders(borders);
  memcpy(this->Internal->Form->Borders, borders, sizeof(int));

  for(QMap<int, pqPlotMatrixOptionsChartSetting*>::iterator dataIt=
    this->Internal->Form->PlotData.begin(); 
    dataIt!=this->Internal->Form->PlotData.end(); ++dataIt)
    {
    int plotType = dataIt.key();
    pqPlotMatrixOptionsChartSetting* settings = dataIt.value();
    color = proxy->GetAxisColor(plotType);
    settings->AxisColor = QColor::fromRgb(color[0], color[1], color[2],
                                          color[3]);
    color = proxy->GetBackgroundColor(plotType);
    settings->BackGroundColor = QColor::fromRgb(color[0], color[1], color[2],
                                                color[3]);
    color = proxy->GetGridColor(plotType);
    settings->GridColor = QColor::fromRgb(color[0], color[1], color[2],
                                          color[3]);
    prop = proxy->GetAxisLabelProperties(plotType);
    prop->GetColor(rgba);
    rgba[3] = prop->GetOpacity();
    settings->LabelColor = QColor::fromRgbF(rgba[0], rgba[1], rgba[2],
                                            rgba[3]);
    settings->Notation = proxy->GetAxisLabelNotation(plotType);
    settings->Precision = proxy->GetAxisLabelPrecision(plotType);
    settings->ShowGrid = proxy->GetGridVisibility(plotType);
    settings->ShowLabels = proxy->GetAxisLabelVisibility(plotType);
    settings->ToolTipNotation = proxy->GetTooltipNotation(plotType);
    settings->ToolTipPrecision = proxy->GetTooltipPrecision(plotType);

    settings->LabelFont =
      QFont(prop->GetFontFamilyAsString(),
            prop->GetFontSize(),
            prop->GetBold() ? QFont::Bold : -1,
            prop->GetItalic());
    }
  this->blockSignals(false);
}

void pqPlotMatrixOptionsEditor::applyChartOptions()
{
  vtkSMPlotMatrixViewProxy *smproxy = vtkSMPlotMatrixViewProxy::SafeDownCast(
    this->getProxy());
  if(!smproxy)
    {
    return;
    }
  vtkScatterPlotMatrix* proxy = vtkScatterPlotMatrix::SafeDownCast(
    smproxy->GetContextItem());
  if(!proxy)
    {
    return;
    } 

  // title
  this->Internal->Form->Title = this->Internal->Form->ChartTitle->text();
  proxy->SetTitle(this->Internal->Form->Title.toAscii().constData());
  this->Internal->Form->TitleAlignment = 
    this->Internal->Form->ChartTitleAlignment->currentIndex();
  vtkTextProperty *prop = proxy->GetTitleProperties();
  prop->SetJustification(this->Internal->Form->TitleAlignment);
  // Apply the Title font type info
  QString fontFamily = this->Internal->Form->TitleFont.family();
  prop->SetFontFamilyAsString(fontFamily.toAscii().constData());
  prop->SetFontSize(this->Internal->Form->TitleFont.pointSize());
  prop->SetBold(this->Internal->Form->TitleFont.bold() ? 1 : 0);
  prop->SetItalic(this->Internal->Form->TitleFont.italic() ? 1 : 0);

  // The chart title color
  QColor color = this->Internal->Form->ChartTitleColor->chosenColor();
  this->Internal->Form->TitleColor = color;
  prop->SetColor(static_cast<double>(color.redF()),
                 static_cast<double>(color.greenF()),
                 static_cast<double>(color.blueF()));

  // Gutter size
  this->Internal->Form->Gutter.Set(
    this->Internal->Form->GutterX->value(),
    this->Internal->Form->GutterY->value());
  proxy->SetGutter(this->Internal->Form->Gutter);

  // Margin size
  this->Internal->Form->Borders[0]= this->Internal->Form->LeftMargin->value();
  this->Internal->Form->Borders[1]=  this->Internal->Form->BottomMargin->value();
  this->Internal->Form->Borders[2]=  this->Internal->Form->RightMargin->value();
  this->Internal->Form->Borders[3]=  this->Internal->Form->TopMargin->value();
  proxy->SetBorders(
    this->Internal->Form->Borders[0],
    this->Internal->Form->Borders[1],
    this->Internal->Form->Borders[2],
    this->Internal->Form->Borders[3]);
  
  // Scatter plot selection background color
  color = this->Internal->Form->selRowColBackgroundColor->chosenColor();
  this->Internal->Form->SelectedRowColumnScatterChartBGColor = color;
  vtkColor4ub vcolor(static_cast<unsigned char>(color.red()),
                     static_cast<unsigned char>(color.green()),
                     static_cast<unsigned char>(color.blue()),
                     static_cast<unsigned char>(color.alpha()));
  proxy->SetScatterPlotSelectedRowColumnColor(vcolor);
  color = this->Internal->Form->selActiveBackgroundColor->chosenColor();
  this->Internal->Form->SelectedActiveScatterChartBGColor = color;
  vcolor.Set(static_cast<unsigned char>(color.red()),
             static_cast<unsigned char>(color.green()),
             static_cast<unsigned char>(color.blue()),
             static_cast<unsigned char>(color.alpha()));
  proxy->SetScatterPlotSelectedActiveColor(vcolor);

  foreach(int plotType, this->Internal->Form->PlotData.keys())
    {
    // Show axis grid lines
    proxy->SetGridVisibility(plotType,
      this->Internal->Form->PlotData[plotType]->ShowGrid);
    // Background color
    color = this->Internal->Form->PlotData[plotType]->BackGroundColor;
    vcolor.Set(static_cast<unsigned char>(color.red()),
               static_cast<unsigned char>(color.green()),
               static_cast<unsigned char>(color.blue()),
               static_cast<unsigned char>(color.alpha()));
    proxy->SetBackgroundColor(plotType, vcolor);
    // Axis color
    color = this->Internal->Form->PlotData[plotType]->AxisColor;
    vcolor.Set(static_cast<unsigned char>(color.red()),
               static_cast<unsigned char>(color.green()),
               static_cast<unsigned char>(color.blue()),
               static_cast<unsigned char>(color.alpha()));
    proxy->SetAxisColor(plotType, vcolor);
    // Axis grid color
    color = this->Internal->Form->PlotData[plotType]->GridColor;
    vcolor.Set(static_cast<unsigned char>(color.red()),
               static_cast<unsigned char>(color.green()),
               static_cast<unsigned char>(color.blue()),
               static_cast<unsigned char>(color.alpha()));
    proxy->SetGridColor(plotType, vcolor);
    // Axis label visibility
    proxy->SetAxisLabelVisibility(plotType,
      this->Internal->Form->PlotData[plotType]->ShowLabels);
    // Label color
    prop = proxy->GetAxisLabelProperties(plotType);
    color = this->Internal->Form->PlotData[plotType]->LabelColor;
    prop->SetColor(static_cast<double>(color.redF()),
                   static_cast<double>(color.greenF()),
                   static_cast<double>(color.blueF()));
    prop->SetOpacity(static_cast<double>(color.alphaF()));
    // Axis label font
    QFont plotfont = this->Internal->Form->PlotData[plotType]->LabelFont;
    prop->SetFontFamilyAsString(plotfont.family().toAscii().constData());
    prop->SetFontSize(plotfont.pointSize());
    prop->SetBold(plotfont.bold() ? 1 : 0);
    prop->SetItalic(plotfont.italic() ? 1 : 0);
    // Axis label notation
    proxy->SetAxisLabelNotation(plotType,
      this->Internal->Form->PlotData[plotType]->Notation);
    // Axis label precision
    proxy->SetAxisLabelPrecision(plotType,
      this->Internal->Form->PlotData[plotType]->Precision);
    }
  proxy->UpdateSettings();
}

void pqPlotMatrixOptionsEditor::loadChartPage()
{
  this->blockSignals(true);
  if(this->Internal->Form->CurrentPlot==vtkScatterPlotMatrix::NOPLOT)
    {
    // load general properties
    this->updateDescription(
      this->Internal->Form->ChartTitleFont,this->Internal->Form->TitleFont);
    this->Internal->Form->ChartTitleColor->setChosenColor(
      this->Internal->Form->TitleColor);
    this->Internal->Form->ChartTitle->setText(
      this->Internal->Form->Title);
    this->Internal->Form->ChartTitleAlignment->setCurrentIndex(
      this->Internal->Form->TitleAlignment);
    this->Internal->Form->LeftMargin->setValue(
      this->Internal->Form->Borders[0]);
    this->Internal->Form->BottomMargin->setValue(
      this->Internal->Form->Borders[1]);
    this->Internal->Form->RightMargin->setValue(
      this->Internal->Form->Borders[2]);
    this->Internal->Form->TopMargin->setValue(
      this->Internal->Form->Borders[3]);
    this->Internal->Form->GutterX->setValue(
      this->Internal->Form->Gutter[0]);
    this->Internal->Form->GutterY->setValue(
      this->Internal->Form->Gutter[1]);
    this->Internal->Form->selActiveBackgroundColor->setChosenColor(
      this->Internal->Form->SelectedActiveScatterChartBGColor);
    this->Internal->Form->selRowColBackgroundColor->setChosenColor(
      this->Internal->Form->SelectedRowColumnScatterChartBGColor);
    }
  else
    {
    pqPlotMatrixOptionsChartSetting *axis =
      this->Internal->Form->PlotData[this->Internal->Form->CurrentPlot];
    this->Internal->Form->ShowAxisGrid->setChecked(axis->ShowGrid);
    this->Internal->Form->BackgroundColor->setChosenColor(axis->BackGroundColor);
    this->Internal->Form->AxisColor->setChosenColor(axis->AxisColor);
    this->Internal->Form->GridColor->setChosenColor(axis->GridColor);
    this->Internal->Form->ShowAxisLabels->setChecked(axis->ShowLabels);
    this->updateDescription(this->Internal->Form->AxisLabelFont, axis->LabelFont);
    this->Internal->Form->LabelColor->setChosenColor(axis->LabelColor);
    this->Internal->Form->LabelNotation->setCurrentIndex(axis->Notation);
    this->Internal->Form->LabelPrecision->setValue(axis->Precision);
    this->Internal->Form->TooltipNotation->setCurrentIndex(axis->ToolTipNotation);
    this->Internal->Form->TooltipPrecision->setValue(axis->ToolTipPrecision);
    }
  this->blockSignals(false);
}

bool pqPlotMatrixOptionsEditor::pickFont(QLabel *label, QFont &pfont)
{
  bool ok = false;
  pfont = QFontDialog::getFont(&ok, pfont, this);
  if(ok)
    {
    this->updateDescription(label, pfont);
    this->changesAvailable();
    return true;
    }
  else
    {
    return false;
    }
}

void pqPlotMatrixOptionsEditor::updateDescription(QLabel *label,
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

vtkSMProxy* pqPlotMatrixOptionsEditor::getProxy()
{
  return (this->getView()?  this->getView()->getProxy() : NULL);
}
