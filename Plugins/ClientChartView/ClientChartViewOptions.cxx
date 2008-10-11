/*=========================================================================

   Program: ParaView
   Module:    ClientChartViewOptions.cxx

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

/// \file ClientChartViewOptions.cxx
/// \date 7/20/2007

#include "ClientChartViewOptions.h"
#include "ui_ClientChartViewOptions.h"

#include "pqSampleScalarAddRangeDialog.h"

#include <QAbstractItemDelegate>
#include <QColor>
#include <QFont>
#include <QFontDialog>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QString>
#include <QStringList>
#include <QStringListModel>

#include <math.h>


class ClientChartViewOptionsAxis
{
public:
  ClientChartViewOptionsAxis();
  ~ClientChartViewOptionsAxis() {}

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


class ClientChartViewOptionsForm : public Ui::ClientChartViewOptions
{
public:
  ClientChartViewOptionsForm();
  ~ClientChartViewOptionsForm();

  void setCurrentAxis(const QString &name);
  int getIndexForLocation(vtkQtChartAxis::AxisLocation location) const;

  QString CurrentPage;
  QFont TitleFont;
  ClientChartViewOptionsAxis *AxisData[4];
  vtkQtChartAxis::AxisLocation CurrentAxis;
  int AxisIndex;
  pqSampleScalarAddRangeDialog *RangeDialog;
};


//----------------------------------------------------------------------------
ClientChartViewOptionsAxis::ClientChartViewOptionsAxis()
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
ClientChartViewOptionsForm::ClientChartViewOptionsForm()
  : Ui::ClientChartViewOptions(), CurrentPage(), TitleFont()
{
  this->CurrentAxis = vtkQtChartAxis::Left;
  this->AxisIndex = -1;
  this->RangeDialog = 0;

  // Create the axis data objects.
  for(int i = 0; i < 4; i++)
    {
    this->AxisData[i] = new ClientChartViewOptionsAxis();
    }
}

ClientChartViewOptionsForm::~ClientChartViewOptionsForm()
{
  // Clean up the axis data objects.
  for(int i = 0; i < 4; i++)
    {
    delete this->AxisData[i];
    }
}

void ClientChartViewOptionsForm::setCurrentAxis(const QString &name)
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

int ClientChartViewOptionsForm::getIndexForLocation(
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
ClientChartViewOptions::ClientChartViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Form = new ClientChartViewOptionsForm();
  this->Form->setupUi(this);

  // Initialize the general options page.
  this->updateDescription(this->Form->ChartTitleFont, this->Form->TitleFont);
  this->Form->ChartTitleAlignment->setCurrentIndex(1);
  this->Form->ChartTitleColor->setChosenColor(QColor(Qt::black));
  this->Form->LegendLocation->setCurrentIndex(2);
  this->Form->LegendFlow->setCurrentIndex(1);

  // Listen for user changes.
  this->connect(this->Form->ChartTitle, SIGNAL(textChanged(const QString &)),
      this, SIGNAL(titleChanged(const QString &)));
  this->connect(this->Form->ChartTitleFontButton, SIGNAL(clicked()),
      this, SLOT(pickTitleFont()));
  this->connect(
      this->Form->ChartTitleColor, SIGNAL(chosenColorChanged(const QColor &)),
      this, SIGNAL(titleColorChanged(const QColor &)));
  this->connect(
      this->Form->ChartTitleAlignment, SIGNAL(currentIndexChanged(int)),
      this, SIGNAL(titleAlignmentChanged(int)));
  this->connect(this->Form->ShowLegend, SIGNAL(toggled(bool)),
      this, SIGNAL(showLegendChanged(bool)));
  this->connect(this->Form->LegendLocation, SIGNAL(currentIndexChanged(int)),
      this, SLOT(convertLegendLocation(int)));
  this->connect(this->Form->LegendFlow, SIGNAL(currentIndexChanged(int)),
      this, SLOT(convertLegendFlow(int)));

  this->connect(this->Form->ShowAxis, SIGNAL(toggled(bool)),
      this, SLOT(setAxisShowing(bool)));
  this->connect(this->Form->ShowAxisGrid, SIGNAL(toggled(bool)),
      this, SLOT(setAxisGridShowing(bool)));
  this->connect(this->Form->GridType, SIGNAL(currentIndexChanged(int)),
      this, SLOT(setGridColorType(int)));
  this->connect(
      this->Form->AxisColor, SIGNAL(chosenColorChanged(const QColor &)),
      this, SLOT(setAxisColor(const QColor &)));
  this->connect(
      this->Form->GridColor, SIGNAL(chosenColorChanged(const QColor &)),
      this, SLOT(setGridColor(const QColor &)));
  this->connect(this->Form->ShowAxisLabels, SIGNAL(toggled(bool)),
      this, SLOT(setAxisLabelsShowing(bool)));
  this->connect(this->Form->AxisLabelFontButton, SIGNAL(clicked()),
      this, SLOT(pickAxisLabelFont()));
  this->connect(
      this->Form->LabelColor, SIGNAL(chosenColorChanged(const QColor &)),
      this, SLOT(setLabelColor(const QColor &)));
  this->connect(this->Form->LabelNotation, SIGNAL(currentIndexChanged(int)),
      this, SLOT(setLabelNotation(int)));
  this->connect(this->Form->LabelPrecision, SIGNAL(valueChanged(int)),
      this, SLOT(setLabelPrecision(int)));

  this->connect(this->Form->UseLogScale, SIGNAL(toggled(bool)),
      this, SLOT(setUsingLogScale(bool)));
  this->connect(this->Form->UseChartSelect, SIGNAL(toggled(bool)),
      this, SLOT(changeLayoutPage(bool)));
  this->connect(this->Form->UseBestFit, SIGNAL(toggled(bool)),
      this, SLOT(changeLayoutPage(bool)));
  this->connect(this->Form->UseFixedInterval, SIGNAL(toggled(bool)),
      this, SLOT(changeLayoutPage(bool)));
  this->connect(this->Form->AxisMinimum, SIGNAL(textChanged(const QString &)),
      this, SLOT(setAxisMinimum(const QString &)));
  this->connect(this->Form->AxisMaximum, SIGNAL(textChanged(const QString &)),
      this, SLOT(setAxisMaximum(const QString &)));
  this->connect(this->Form->AddButton, SIGNAL(clicked()),
      this, SLOT(addAxisLabel()));
  this->connect(this->Form->RemoveButton, SIGNAL(clicked()),
      this, SLOT(removeSelectedLabels()));
  this->connect(this->Form->GenerateButton, SIGNAL(clicked()),
      this, SLOT(showRangeDialog()));
  QAbstractItemDelegate *delegate = this->Form->LabelList->itemDelegate();
  this->connect(delegate, SIGNAL(closeEditor(QWidget *)),
      this, SLOT(updateAxisLabels()), Qt::QueuedConnection);

  this->connect(
      this->Form->AxisTitleText, SIGNAL(textChanged(const QString &)),
      this, SLOT(setAxisTitle(const QString &)));
  this->connect(this->Form->AxisTitleFontButton, SIGNAL(clicked()),
      this, SLOT(pickAxisTitleFont()));
  this->connect(
      this->Form->AxisTitleColor, SIGNAL(chosenColorChanged(const QColor &)),
      this, SLOT(setAxisTitleColor(const QColor &)));
  this->connect(
      this->Form->AxisTitleAlignment, SIGNAL(currentIndexChanged(int)),
      this, SLOT(setAxisTitleAlignment(int)));

  this->Form->LabelList->installEventFilter(this);
}

ClientChartViewOptions::~ClientChartViewOptions()
{
  delete this->Form;
}

bool ClientChartViewOptions::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->Form->LabelList && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);
    if(ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace)
      {
      this->removeSelectedLabels();
      }
    }

  return false;
}

void ClientChartViewOptions::setPage(const QString &page)
{
  if(this->Form->CurrentPage == page)
    {
    return;
    }

  this->Form->CurrentPage = page;
  this->Form->AxisIndex = -1;

  // Split the page path into its components. Use the page path to
  // determine which widget to show.
  QWidget *widget = 0;
  QStringList path = page.split(".", QString::SkipEmptyParts);
  if(path[0] == "General")
    {
    widget = this->Form->General;
    }
  else
    {
    // Use the axis location string to set the current axis.
    this->Form->setCurrentAxis(path[0]);

    // Load the axis data for the specified axis.
    if(path.size() == 1)
      {
      widget = this->Form->Axis;
      this->loadAxisPage();

      // Set the page label to the current axis name.
      this->Form->AxisGeneralLabel->setText(path[0]);
      }
    else if(path[1] == "Layout")
      {
      widget = this->Form->AxisLayout;
      this->loadAxisLayoutPage();
      }
    else if(path[1] == "Title")
      {
      widget = this->Form->AxisTitle;
      this->loadAxisTitlePage();
      }
    }

  if(widget)
    {
    this->Form->ChartPages->setCurrentWidget(widget);
    }
}

QStringList ClientChartViewOptions::getPageList()
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

void ClientChartViewOptions::getTitle(QString &title) const
{
  title = this->Form->ChartTitle->text();
}

void ClientChartViewOptions::setTitle(const QString &title)
{
  this->Form->ChartTitle->setText(title);
}

const QFont &ClientChartViewOptions::getTitleFont() const
{
  return this->Form->TitleFont;
}

void ClientChartViewOptions::setTitleFont(const QFont &newFont)
{
  if(this->Form->TitleFont != newFont)
    {
    this->Form->TitleFont = newFont;
    emit this->titleFontChanged(newFont);
    }
}

void ClientChartViewOptions::getTitleColor(QColor &color) const
{
  color = this->Form->ChartTitleColor->chosenColor();
}

void ClientChartViewOptions::setTitleColor(const QColor &color)
{
  this->Form->ChartTitleColor->setChosenColor(color);
}

int ClientChartViewOptions::getTitleAlignment() const
{
  return this->Form->ChartTitleAlignment->currentIndex();
}

void ClientChartViewOptions::setTitleAlignment(int alignment)
{
  this->Form->ChartTitleAlignment->setCurrentIndex(alignment);
}

bool ClientChartViewOptions::isLegendShowing() const
{
  return this->Form->ShowLegend->isChecked();
}

void ClientChartViewOptions::setLegendShowing(bool legendShowing)
{
  this->Form->ShowLegend->setChecked(legendShowing);
}

vtkQtChartLegend::LegendLocation ClientChartViewOptions::getLegendLocation() const
{
  switch(this->Form->LegendLocation->currentIndex())
    {
    case 0:
      {
      return vtkQtChartLegend::Left;
      }
    case 1:
      {
      return vtkQtChartLegend::Top;
      }
    case 2:
    default:
      {
      return vtkQtChartLegend::Right;
      }
    case 3:
      {
      return vtkQtChartLegend::Bottom;
      }
    }
}

void ClientChartViewOptions::setLegendLocation(
    vtkQtChartLegend::LegendLocation location)
{
  switch(location)
    {
    case vtkQtChartLegend::Left:
      {
      this->Form->LegendLocation->setCurrentIndex(0);
      break;
      }
    case vtkQtChartLegend::Top:
      {
      this->Form->LegendLocation->setCurrentIndex(1);
      break;
      }
    case vtkQtChartLegend::Right:
      {
      this->Form->LegendLocation->setCurrentIndex(2);
      break;
      }
    case vtkQtChartLegend::Bottom:
      {
      this->Form->LegendLocation->setCurrentIndex(3);
      break;
      }
    }
}

vtkQtChartLegend::ItemFlow ClientChartViewOptions::getLegendFlow() const
{
  if(this->Form->LegendFlow->currentIndex() == 0)
    {
    return vtkQtChartLegend::LeftToRight;
    }

  return vtkQtChartLegend::TopToBottom;
}

void ClientChartViewOptions::setLegendFlow(vtkQtChartLegend::ItemFlow flow)
{
  if(flow == vtkQtChartLegend::LeftToRight)
    {
    this->Form->LegendFlow->setCurrentIndex(0);
    }
  else
    {
    this->Form->LegendFlow->setCurrentIndex(1);
    }
}

bool ClientChartViewOptions::isAxisShowing(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->ShowAxis;
}

void ClientChartViewOptions::setAxisShowing(vtkQtChartAxis::AxisLocation location,
    bool axisShowing)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->ShowAxis != axisShowing)
    {
    this->Form->AxisData[index]->ShowAxis = axisShowing;
    if(index == this->Form->AxisIndex)
      {
      this->Form->ShowAxis->setChecked(axisShowing);
      }
    else
      {
      emit this->showAxisChanged(location, axisShowing);
      }
    }
}

bool ClientChartViewOptions::isAxisGridShowing(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->ShowGrid;
}

void ClientChartViewOptions::setAxisGridShowing(
    vtkQtChartAxis::AxisLocation location, bool gridShowing)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->ShowGrid != gridShowing)
    {
    this->Form->AxisData[index]->ShowGrid = gridShowing;
    if(index == this->Form->AxisIndex)
      {
      this->Form->ShowAxisGrid->setChecked(gridShowing);
      }
    else
      {
      emit this->showAxisGridChanged(location, gridShowing);
      }
    }
}

vtkQtChartAxisOptions::AxisGridColor ClientChartViewOptions::getAxisGridType(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->GridType == 0)
    {
    return vtkQtChartAxisOptions::Lighter;
    }

  return vtkQtChartAxisOptions::Specified;
}

void ClientChartViewOptions::setAxisGridType(vtkQtChartAxis::AxisLocation location,
    vtkQtChartAxisOptions::AxisGridColor color)
{
  int index = this->Form->getIndexForLocation(location);
  int gridType = color == vtkQtChartAxisOptions::Lighter ? 0 : 1;
  if(this->Form->AxisData[index]->GridType != gridType)
    {
    this->Form->AxisData[index]->GridType = gridType;
    if(index == this->Form->AxisIndex)
      {
      this->Form->GridType->setCurrentIndex(gridType);
      }
    else
      {
      emit this->axisGridTypeChanged(location, color);
      }
    }
}

const QColor &ClientChartViewOptions::getAxisColor(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->AxisColor;
}

void ClientChartViewOptions::setAxisColor(vtkQtChartAxis::AxisLocation location,
    const QColor &color)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->AxisColor != color)
    {
    this->Form->AxisData[index]->AxisColor = color;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisColor->setChosenColor(color);
      }
    else
      {
      emit this->axisColorChanged(location, color);
      }
    }
}

const QColor &ClientChartViewOptions::getAxisGridColor(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->GridColor;
}

void ClientChartViewOptions::setAxisGridColor(vtkQtChartAxis::AxisLocation location,
    const QColor &color)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->GridColor != color)
    {
    this->Form->AxisData[index]->GridColor = color;
    if(index == this->Form->AxisIndex)
      {
      this->Form->GridColor->setChosenColor(color);
      }
    else
      {
      emit this->axisGridColorChanged(location, color);
      }
    }
}

bool ClientChartViewOptions::areAxisLabelsShowing(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->ShowLabels;
}

void ClientChartViewOptions::setAxisLabelsShowing(
    vtkQtChartAxis::AxisLocation location, bool labelsShowing)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->ShowLabels != labelsShowing)
    {
    this->Form->AxisData[index]->ShowLabels = labelsShowing;
    if(index == this->Form->AxisIndex)
      {
      this->Form->ShowAxisLabels->setChecked(labelsShowing);
      }
    else
      {
      emit this->showAxisLabelsChanged(location, labelsShowing);
      }
    }
}

const QFont &ClientChartViewOptions::getAxisLabelFont(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->LabelFont;
}

void ClientChartViewOptions::setAxisLabelFont(vtkQtChartAxis::AxisLocation location,
    const QFont &newFont)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->LabelFont != newFont)
    {
    this->Form->AxisData[index]->LabelFont = newFont;
    this->updateDescription(this->Form->AxisLabelFont, newFont);
    emit this->axisLabelFontChanged(location, newFont);
    }
}

const QColor &ClientChartViewOptions::getAxisLabelColor(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->LabelColor;
}

void ClientChartViewOptions::setAxisLabelColor(
    vtkQtChartAxis::AxisLocation location, const QColor &color)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->LabelColor != color)
    {
    this->Form->AxisData[index]->LabelColor = color;
    if(index == this->Form->AxisIndex)
      {
      this->Form->LabelColor->setChosenColor(color);
      }
    else
      {
      emit this->axisLabelColorChanged(location, color);
      }
    }
}

vtkQtChartAxisOptions::NotationType ClientChartViewOptions::getAxisLabelNotation(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  switch(this->Form->AxisData[index]->Notation)
    {
    case 0:
      {
      return vtkQtChartAxisOptions::Standard;
      }
    case 1:
      {
      return vtkQtChartAxisOptions::Exponential;
      }
    case 2:
      {
      return vtkQtChartAxisOptions::Engineering;
      }
    case 3:
    default:
      {
      return vtkQtChartAxisOptions::StandardOrExponential;
      }
    }
}

void ClientChartViewOptions::setAxisLabelNotation(
    vtkQtChartAxis::AxisLocation location, vtkQtChartAxisOptions::NotationType notation)
{
  int i = this->Form->getIndexForLocation(location);
  int index = 3;
  if(notation == vtkQtChartAxisOptions::Standard)
    {
    index = 0;
    }
  else if(notation == vtkQtChartAxisOptions::Exponential)
    {
    index = 1;
    }
  else if(notation == vtkQtChartAxisOptions::Engineering)
    {
    index = 2;
    }

  if(this->Form->AxisData[i]->Notation != index)
    {
    this->Form->AxisData[i]->Notation = index;
    if(i == this->Form->AxisIndex)
      {
      this->Form->LabelNotation->setCurrentIndex(index);
      }
    else
      {
      emit this->axisLabelNotationChanged(location, notation);
      }
    }
}

int ClientChartViewOptions::getAxisLabelPrecision(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->Precision;
}

void ClientChartViewOptions::setAxisLabelPrecision(
    vtkQtChartAxis::AxisLocation location, int precision)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->Precision != precision)
    {
    this->Form->AxisData[index]->Precision = precision;
    if(index == this->Form->AxisIndex)
      {
      this->Form->LabelPrecision->setValue(precision);
      }
    else
      {
      emit this->axisLabelPrecisionChanged(location, precision);
      }
    }
}

bool ClientChartViewOptions::isUsingLogScale(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->UseLogScale;
}

void ClientChartViewOptions::setAxisScale(vtkQtChartAxis::AxisLocation location,
    bool useLogScale)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->UseLogScale != useLogScale)
    {
    this->Form->AxisData[index]->UseLogScale = useLogScale;
    if(index == this->Form->AxisIndex)
      {
      this->Form->UseLogScale->setChecked(useLogScale);
      }
    else
      {
      emit this->axisScaleChanged(location, useLogScale);
      }
    }
}

vtkQtChartAxisLayer::AxisBehavior ClientChartViewOptions::getAxisBehavior(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->AxisLayout == 1)
    {
    return vtkQtChartAxisLayer::BestFit;
    }
  else if(this->Form->AxisData[index]->AxisLayout == 2)
    {
    return vtkQtChartAxisLayer::FixedInterval;
    }

  return vtkQtChartAxisLayer::ChartSelect;
}

void ClientChartViewOptions::setAxisBehavior(vtkQtChartAxis::AxisLocation location,
    vtkQtChartAxisLayer::AxisBehavior behavior)
{
  int index = this->Form->getIndexForLocation(location);
  int axisLayout = 0;
  if(behavior == vtkQtChartAxisLayer::BestFit)
    {
    axisLayout = 1;
    }
  else if(behavior == vtkQtChartAxisLayer::FixedInterval)
    {
    axisLayout = 2;
    }

  if(this->Form->AxisData[index]->AxisLayout != axisLayout)
    {
    this->Form->AxisData[index]->AxisLayout = axisLayout;
    if(index == this->Form->AxisIndex)
      {
      if(behavior == vtkQtChartAxisLayer::ChartSelect)
        {
        this->Form->UseChartSelect->setChecked(true);
        }
      else if(behavior == vtkQtChartAxisLayer::BestFit)
        {
        this->Form->UseBestFit->setChecked(true);
        }
      else if(behavior == vtkQtChartAxisLayer::FixedInterval)
        {
        this->Form->UseFixedInterval->setChecked(true);
        }
      }
    else
      {
      emit this->axisBehaviorChanged(location, behavior);
      }
    }
}

void ClientChartViewOptions::getAxisMinimum(vtkQtChartAxis::AxisLocation location,
    QVariant &minimum) const
{
  int index = this->Form->getIndexForLocation(location);
  minimum = this->Form->AxisData[index]->Minimum.toDouble();
}

void ClientChartViewOptions::setAxisMinimum(vtkQtChartAxis::AxisLocation location,
    const QVariant &minimum)
{
  int index = this->Form->getIndexForLocation(location);
  ClientChartViewOptionsAxis *axis = this->Form->AxisData[index];
  QString text = QString::number(minimum.toDouble(),'g',axis->Precision);
  if(axis->Minimum != text)
    {
    axis->Minimum = text;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisMinimum->setText(text);
      }
    else
      {
      emit this->axisMinimumChanged(location, minimum);
      }
    }
}

void ClientChartViewOptions::getAxisMaximum(vtkQtChartAxis::AxisLocation location,
    QVariant &maximum) const
{
  int index = this->Form->getIndexForLocation(location);
  maximum = this->Form->AxisData[index]->Maximum.toDouble();
}

void ClientChartViewOptions::setAxisMaximum(vtkQtChartAxis::AxisLocation location,
    const QVariant &maximum)
{
  int index = this->Form->getIndexForLocation(location);
  ClientChartViewOptionsAxis *axis = this->Form->AxisData[index];
  QString text = QString::number(maximum.toDouble(),'g',axis->Precision);
  if(axis->Maximum != text)
    {
    axis->Maximum = text;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisMaximum->setText(text);
      }
    else
      {
      emit this->axisMaximumChanged(location, maximum);
      }
    }
}

void ClientChartViewOptions::getAxisLabels(vtkQtChartAxis::AxisLocation location,
    QStringList &list) const
{
  int index = this->Form->getIndexForLocation(location);
  list = this->Form->AxisData[index]->Labels.stringList();
}

void ClientChartViewOptions::setAxisLabels(vtkQtChartAxis::AxisLocation location,
    const QStringList &list)
{
  int index = this->Form->getIndexForLocation(location);
  this->Form->AxisData[index]->Labels.setStringList(list);
  emit this->axisLabelsChanged(location, list);
}

const QString &ClientChartViewOptions::getAxisTitle(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->Title;
}

void ClientChartViewOptions::setAxisTitle(vtkQtChartAxis::AxisLocation location,
    const QString &title)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->Title != title)
    {
    this->Form->AxisData[index]->Title = title;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisTitleText->setText(title);
      }
    else
      {
      emit this->axisTitleChanged(location, title);
      }
    }
}

const QFont &ClientChartViewOptions::getAxisTitleFont(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->TitleFont;
}

void ClientChartViewOptions::setAxisTitleFont(vtkQtChartAxis::AxisLocation location,
    const QFont &newFont)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->TitleFont != newFont)
    {
    this->Form->AxisData[index]->TitleFont = newFont;
    this->updateDescription(this->Form->AxisTitleFont, newFont);
    emit this->axisTitleFontChanged(location, newFont);
    }
}

const QColor &ClientChartViewOptions::getAxisTitleColor(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->TitleColor;
}

void ClientChartViewOptions::setAxisTitleColor(
    vtkQtChartAxis::AxisLocation location, const QColor &color)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->TitleColor != color)
    {
    this->Form->AxisData[index]->TitleColor = color;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisTitleColor->setChosenColor(color);
      }
    else
      {
      emit this->axisTitleColorChanged(location, color);
      }
    }
}

int ClientChartViewOptions::getAxisTitleAlignment(
    vtkQtChartAxis::AxisLocation location) const
{
  int index = this->Form->getIndexForLocation(location);
  return this->Form->AxisData[index]->TitleAlignment;
}

void ClientChartViewOptions::setAxisTitleAlignment(
    vtkQtChartAxis::AxisLocation location, int alignment)
{
  int index = this->Form->getIndexForLocation(location);
  if(this->Form->AxisData[index]->TitleAlignment != alignment)
    {
    this->Form->AxisData[index]->TitleAlignment = alignment;
    if(index == this->Form->AxisIndex)
      {
      this->Form->AxisTitleAlignment->setCurrentIndex(alignment);
      }
    else
      {
      emit this->axisTitleAlignmentChanged(location, alignment);
      }
    }
}

void ClientChartViewOptions::pickTitleFont()
{
  bool ok = false;
  this->Form->TitleFont =
      QFontDialog::getFont(&ok, this->Form->TitleFont, this);
  if(ok)
    {
    this->updateDescription(this->Form->ChartTitleFont, this->Form->TitleFont);
    emit this->titleFontChanged(this->Form->TitleFont);
    }
}

void ClientChartViewOptions::convertLegendLocation(int)
{
  emit this->legendLocationChanged(this->getLegendLocation());
}

void ClientChartViewOptions::convertLegendFlow(int)
{
  emit this->legendFlowChanged(this->getLegendFlow());
}

void ClientChartViewOptions::setAxisShowing(bool axisShowing)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->ShowAxis = axisShowing;
    emit this->showAxisChanged(this->Form->CurrentAxis, axisShowing);
    }
}

void ClientChartViewOptions::setAxisGridShowing(bool gridShowing)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->ShowGrid = gridShowing;
    emit this->showAxisGridChanged(this->Form->CurrentAxis, gridShowing);
    }
}

void ClientChartViewOptions::setGridColorType(int index)
{
  if(this->Form->AxisIndex != -1)
    {
    // Save the grid color type in the axis data.
    this->Form->AxisData[this->Form->AxisIndex]->GridType = index;

    // Emit the change signal with the enum type.
    if(index == 0)
      {
      emit this->axisGridTypeChanged(this->Form->CurrentAxis,
          vtkQtChartAxisOptions::Lighter);
      }
    else
      {
      emit this->axisGridTypeChanged(this->Form->CurrentAxis,
          vtkQtChartAxisOptions::Specified);
      }
    }
}

void ClientChartViewOptions::setAxisColor(const QColor &color)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->AxisColor = color;
    emit this->axisColorChanged(this->Form->CurrentAxis, color);
    }
}

void ClientChartViewOptions::setGridColor(const QColor &color)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->GridColor = color;
    emit this->axisGridColorChanged(this->Form->CurrentAxis, color);
    }
}

void ClientChartViewOptions::setAxisLabelsShowing(bool labelsShowing)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->ShowLabels = labelsShowing;
    emit this->showAxisLabelsChanged(this->Form->CurrentAxis, labelsShowing);
    }
}

void ClientChartViewOptions::pickAxisLabelFont()
{
  if(this->Form->AxisIndex != -1)
    {
    bool ok = false;
    ClientChartViewOptionsAxis *axis =
        this->Form->AxisData[this->Form->AxisIndex];
    axis->LabelFont = QFontDialog::getFont(&ok, axis->LabelFont, this);
    if(ok)
      {
      this->updateDescription(this->Form->AxisLabelFont, axis->LabelFont);
      emit this->axisLabelFontChanged(this->Form->CurrentAxis, axis->LabelFont);
      }
    }
}

void ClientChartViewOptions::setLabelColor(const QColor &color)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->LabelColor = color;
    emit this->axisLabelColorChanged(this->Form->CurrentAxis, color);
    }
}

void ClientChartViewOptions::setLabelNotation(int index)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->Notation = index;

    // Emit the change signal with the enum type.
    vtkQtChartAxisOptions::NotationType notation = vtkQtChartAxisOptions::StandardOrExponential;
    switch(index)
      {
      case 0:
        {
        notation = vtkQtChartAxisOptions::Standard;
        break;
        }
      case 1:
        {
        notation = vtkQtChartAxisOptions::Exponential;
        break;
        }
      case 2:
        {
        notation = vtkQtChartAxisOptions::Engineering;
        break;
        }
      case 3:
      default:
        {
        notation = vtkQtChartAxisOptions::StandardOrExponential;
        break;
        }
      }

    emit this->axisLabelNotationChanged(this->Form->CurrentAxis, notation);
    }
}

void ClientChartViewOptions::setLabelPrecision(int precision)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->Precision = precision;
    emit this->axisLabelPrecisionChanged(this->Form->CurrentAxis, precision);
    }
}

void ClientChartViewOptions::setUsingLogScale(bool usingLogScale)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->UseLogScale = usingLogScale;
    emit this->axisScaleChanged(this->Form->CurrentAxis, usingLogScale);
    }
}

void ClientChartViewOptions::changeLayoutPage(bool checked)
{
  if(checked && this->Form->AxisIndex != -1)
    {
    // Change the axis layout stack page when the user picks an option.
    ClientChartViewOptionsAxis *axis =
        this->Form->AxisData[this->Form->AxisIndex];
    if(this->Form->UseBestFit->isChecked())
      {
      this->Form->AxisLayoutPages->setCurrentWidget(this->Form->RangePage);
      axis->AxisLayout = 1;
      emit this->axisBehaviorChanged(this->Form->CurrentAxis,
          vtkQtChartAxisLayer::BestFit);
      }
    else if(this->Form->UseFixedInterval->isChecked())
      {
      this->Form->AxisLayoutPages->setCurrentWidget(this->Form->ListPage);
      axis->AxisLayout = 2;
      emit this->axisBehaviorChanged(this->Form->CurrentAxis,
          vtkQtChartAxisLayer::FixedInterval);
      }
    else
      {
      this->Form->AxisLayoutPages->setCurrentWidget(this->Form->BlankPage);
      axis->AxisLayout = 0;
      emit this->axisBehaviorChanged(this->Form->CurrentAxis,
          vtkQtChartAxisLayer::ChartSelect);
      }
    }
}

void ClientChartViewOptions::setAxisMinimum(const QString &text)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->Minimum = text;
    QVariant value(text.toDouble());
    emit this->axisMinimumChanged(this->Form->CurrentAxis, value);
    }
}

void ClientChartViewOptions::setAxisMaximum(const QString &text)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->Maximum = text;
    QVariant value(text.toDouble());
    emit this->axisMaximumChanged(this->Form->CurrentAxis, value);
    }
}

void ClientChartViewOptions::addAxisLabel()
{
  if(this->Form->AxisIndex != -1)
    {
    ClientChartViewOptionsAxis *axis =
        this->Form->AxisData[this->Form->AxisIndex];
    int row = axis->Labels.rowCount();
    if(axis->Labels.insertRow(row))
      {
      QModelIndex index = axis->Labels.index(row, 0);
      this->Form->LabelList->setCurrentIndex(index);
      this->Form->LabelList->edit(index);
      }
    }
}

void ClientChartViewOptions::updateAxisLabels()
{
  if(this->Form->AxisIndex != -1)
    {
    // Get the current item, which should be the edited one.
    QModelIndex index = this->Form->LabelList->currentIndex();
    ClientChartViewOptionsAxis *axis =
        this->Form->AxisData[this->Form->AxisIndex];
    QString text = axis->Labels.data(index, Qt::DisplayRole).toString();
    if(text.isEmpty())
      {
      // Remove empty items.
      axis->Labels.removeRow(index.row());
      }
    else
      {
      // Make sure the label is in order.
      int row = 0;
      double current = text.toDouble();
      QStringList labels = axis->Labels.stringList();
      QStringList::Iterator iter = labels.begin();
      for( ; iter != labels.end(); ++iter, ++row)
        {
        if(row == index.row())
          {
          continue;
          }

        double value = iter->toDouble();
        if(current < value)
          {
          break;
          }
        }

      if(row != index.row() + 1)
        {
        if(row > index.row())
          {
          // If the row will be moved ahead in the list, adjust the
          // insertion index to account for the removed item.
          row -= 1;
          }

        axis->Labels.removeRow(index.row());
        axis->Labels.insertRow(row);
        index = axis->Labels.index(row, 0);
        axis->Labels.setData(index, text, Qt::DisplayRole);
        this->Form->LabelList->setCurrentIndex(index);
        }
      }

    emit this->axisLabelsChanged(this->Form->CurrentAxis,
        this->Form->AxisData[this->Form->AxisIndex]->Labels.stringList());
    }
}

void ClientChartViewOptions::updateRemoveButton()
{
  if(this->Form->AxisIndex != -1)
    {
    QItemSelectionModel *model = this->Form->LabelList->selectionModel();
    this->Form->RemoveButton->setEnabled(model->hasSelection());
    }
}

void ClientChartViewOptions::removeSelectedLabels()
{
  if(this->Form->AxisIndex != -1)
    {
    QItemSelectionModel *model = this->Form->LabelList->selectionModel();
    QModelIndexList indexes = model->selectedIndexes();
    if(indexes.size() > 0)
      {
      // Copy the model indexes to persistent model indexes for
      // removal.
      QList<QPersistentModelIndex> labels;
      QModelIndexList::Iterator iter = indexes.begin();
      for( ; iter != indexes.end(); ++iter)
        {
        labels.append(*iter);
        }

      ClientChartViewOptionsAxis *axis =
          this->Form->AxisData[this->Form->AxisIndex];
      QList<QPersistentModelIndex>::Iterator jter = labels.begin();
      for( ; jter != labels.end(); ++jter)
        {
        axis->Labels.removeRow(jter->row());
        }

      this->Form->RemoveButton->setEnabled(false);
      emit this->axisLabelsChanged(this->Form->CurrentAxis,
          axis->Labels.stringList());
      }
    }
}

void ClientChartViewOptions::showRangeDialog()
{
  if(this->Form->AxisIndex != -1)
    {
    if(this->Form->RangeDialog)
      {
      this->Form->RangeDialog->setResult(0);
      this->Form->RangeDialog->setLogarithmic(
          this->Form->AxisData[this->Form->AxisIndex]->UseLogScale);
      }
    else
      {
      this->Form->RangeDialog = new pqSampleScalarAddRangeDialog(0.0, 1.0, 10,
          this->Form->AxisData[this->Form->AxisIndex]->UseLogScale, this);
      this->Form->RangeDialog->setLogRangeStrict(true);
      this->Form->RangeDialog->setWindowTitle("Generate Axis Labels");
      this->connect(this->Form->RangeDialog, SIGNAL(accepted()),
          this, SLOT(generateAxisLabels()));
      }

    this->Form->RangeDialog->show();
    }
}

void ClientChartViewOptions::generateAxisLabels()
{
  if(this->Form->AxisIndex != -1 && this->Form->RangeDialog)
    {
    double minimum = this->Form->RangeDialog->from();
    double maximum = this->Form->RangeDialog->to();
    if(minimum != maximum)
      {
      QStringList list;
      unsigned long total = this->Form->RangeDialog->steps();
      double interval = 0.0;
      double exponent = 0.0;
      bool useLog = this->Form->RangeDialog->logarithmic();
      if(useLog)
        {
        exponent = log10(minimum);
        double maxExp = log10(maximum);
        interval = (maxExp - exponent) / (double)total;
        }
      else
        {
        interval = (maximum - minimum) / (double)total;
        }

      ClientChartViewOptionsAxis *axis =
          this->Form->AxisData[this->Form->AxisIndex];
      list.append(QString::number(minimum, 'f', axis->Precision));
      for(unsigned long i = 1; i < total; i++)
        {
        if(useLog)
          {
          exponent += interval;
          minimum = pow((double)10.0, exponent);
          }
        else
          {
          minimum += interval;
          }

        list.append(QString::number(minimum, 'f', axis->Precision));
        }

      list.append(QString::number(maximum, 'f', axis->Precision));
      axis->Labels.setStringList(list);

      emit this->axisLabelsChanged(this->Form->CurrentAxis, list);
      }
    }
}

void ClientChartViewOptions::setAxisTitle(const QString &text)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->Title = text;
    emit this->axisTitleChanged(this->Form->CurrentAxis, text);
    }
}

void ClientChartViewOptions::pickAxisTitleFont()
{
  if(this->Form->AxisIndex != -1)
    {
    bool ok = false;
    ClientChartViewOptionsAxis *axis =
        this->Form->AxisData[this->Form->AxisIndex];
    axis->TitleFont = QFontDialog::getFont(&ok, axis->TitleFont, this);
    if(ok)
      {
      this->updateDescription(this->Form->AxisTitleFont, axis->TitleFont);
      emit this->axisTitleFontChanged(this->Form->CurrentAxis, axis->TitleFont);
      }
    }
}

void ClientChartViewOptions::setAxisTitleColor(const QColor &color)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->TitleColor = color;
    emit this->axisTitleColorChanged(this->Form->CurrentAxis, color);
    }
}

void ClientChartViewOptions::setAxisTitleAlignment(int alignment)
{
  if(this->Form->AxisIndex != -1)
    {
    this->Form->AxisData[this->Form->AxisIndex]->TitleAlignment = alignment;
    emit this->axisTitleAlignmentChanged(this->Form->CurrentAxis, alignment);
    }
}

void ClientChartViewOptions::loadAxisPage()
{
  // Block the signals to avoid unnecessary updates.
  this->Form->ShowAxis->blockSignals(true);
  this->Form->ShowAxisGrid->blockSignals(true);
  this->Form->GridType->blockSignals(true);
  this->Form->AxisColor->blockSignals(true);
  this->Form->GridColor->blockSignals(true);
  this->Form->ShowAxisLabels->blockSignals(true);
  this->Form->LabelColor->blockSignals(true);
  this->Form->LabelNotation->blockSignals(true);
  this->Form->LabelPrecision->blockSignals(true);

  // Use the current axis index to get the data.
  ClientChartViewOptionsAxis *axis =
      this->Form->AxisData[this->Form->AxisIndex];
  this->Form->ShowAxis->setChecked(axis->ShowAxis);
  this->Form->ShowAxisGrid->setChecked(axis->ShowGrid);
  this->Form->GridType->setCurrentIndex(axis->GridType);
  this->Form->AxisColor->setChosenColor(axis->AxisColor);
  this->Form->GridColor->setChosenColor(axis->GridColor);
  this->Form->ShowAxisLabels->setChecked(axis->ShowLabels);
  this->updateDescription(this->Form->AxisLabelFont, axis->LabelFont);
  this->Form->LabelColor->setChosenColor(axis->LabelColor);
  this->Form->LabelNotation->setCurrentIndex(axis->Notation);
  this->Form->LabelPrecision->setValue(axis->Precision);

  this->Form->ShowAxis->blockSignals(false);
  this->Form->ShowAxisGrid->blockSignals(false);
  this->Form->GridType->blockSignals(false);
  this->Form->AxisColor->blockSignals(false);
  this->Form->GridColor->blockSignals(false);
  this->Form->ShowAxisLabels->blockSignals(false);
  this->Form->LabelColor->blockSignals(false);
  this->Form->LabelNotation->blockSignals(false);
  this->Form->LabelPrecision->blockSignals(false);
}

void ClientChartViewOptions::loadAxisLayoutPage()
{
  // Block the signals to avoid unnecessary updates.
  this->Form->UseLogScale->blockSignals(true);
  this->Form->UseChartSelect->blockSignals(true);
  this->Form->UseBestFit->blockSignals(true);
  this->Form->UseFixedInterval->blockSignals(true);
  this->Form->AxisMinimum->blockSignals(true);
  this->Form->AxisMaximum->blockSignals(true);

  // Use the current axis index to get the data.
  ClientChartViewOptionsAxis *axis =
      this->Form->AxisData[this->Form->AxisIndex];
  this->Form->UseLogScale->setChecked(axis->UseLogScale);
  if(axis->AxisLayout == 0)
    {
    this->Form->UseChartSelect->setChecked(true);
    }
  else if(axis->AxisLayout == 1)
    {
    this->Form->UseBestFit->setChecked(true);
    }
  else
    {
    this->Form->UseFixedInterval->setChecked(true);
    }

  this->blockSignals(true);
  this->changeLayoutPage(true);
  this->blockSignals(false);
  this->Form->AxisMinimum->setText(axis->Minimum);
  this->Form->AxisMaximum->setText(axis->Maximum);
  QItemSelectionModel *model = this->Form->LabelList->selectionModel();
  if(model)
    {
    this->disconnect(model, 0, this, 0);
    }

  this->Form->LabelList->setModel(&axis->Labels);
  this->connect(this->Form->LabelList->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this, SLOT(updateRemoveButton()));
  this->updateRemoveButton();

  this->Form->UseLogScale->blockSignals(false);
  this->Form->UseChartSelect->blockSignals(false);
  this->Form->UseBestFit->blockSignals(false);
  this->Form->UseFixedInterval->blockSignals(false);
  this->Form->AxisMinimum->blockSignals(false);
  this->Form->AxisMaximum->blockSignals(false);
}

void ClientChartViewOptions::loadAxisTitlePage()
{
  // Block the signals to avoid unnecessary updates.
  this->Form->AxisTitleText->blockSignals(true);
  this->Form->AxisTitleColor->blockSignals(true);
  this->Form->AxisTitleAlignment->blockSignals(true);

  // Use the current axis index to get the data.
  ClientChartViewOptionsAxis *axis =
      this->Form->AxisData[this->Form->AxisIndex];
  this->Form->AxisTitleText->setText(axis->Title);
  this->updateDescription(this->Form->AxisTitleFont, axis->TitleFont);
  this->Form->AxisTitleColor->setChosenColor(axis->TitleColor);
  this->Form->AxisTitleAlignment->setCurrentIndex(axis->TitleAlignment);

  this->Form->AxisTitleText->blockSignals(false);
  this->Form->AxisTitleColor->blockSignals(false);
  this->Form->AxisTitleAlignment->blockSignals(false);
}

void ClientChartViewOptions::updateDescription(QLabel *label,
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


