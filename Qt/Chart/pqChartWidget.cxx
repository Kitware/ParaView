/*=========================================================================

   Program: ParaView
   Module:    pqChartWidget.cxx

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

/// \file pqChartWidget.cxx
/// \date 11/21/2006

#include "pqChartWidget.h"

#include "pqChartArea.h"
#include "pqChartLegend.h"
#include "pqChartTitle.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPrinter>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>


pqChartWidget::pqChartWidget(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  // Initialize the chart members
  this->Title = 0;
  this->Legend = 0;
  this->Charts = new pqChartArea(this);
  this->LeftTitle = 0;
  this->TopTitle = 0;
  this->RightTitle = 0;
  this->BottomTitle = 0;

  // Set the background color.
  this->setBackgroundRole(QPalette::Base);
  this->setAutoFillBackground(true);

  // Set up the chart layout.
  this->TitleLayout = new QVBoxLayout(this);
  this->TitleLayout->setMargin(6);
  this->TitleLayout->setSpacing(4);
  this->LegendLayout = new QGridLayout();
  this->LegendLayout->setMargin(0);
  this->LegendLayout->setSpacing(4);
  this->TopLayout = new QVBoxLayout();
  this->TopLayout->setMargin(0);
  this->TopLayout->setSpacing(4);
  this->ChartLayout = new QHBoxLayout();
  this->ChartLayout->setMargin(0);
  this->ChartLayout->setSpacing(4);

  this->TitleLayout->addLayout(this->LegendLayout);
  this->LegendLayout->addLayout(this->TopLayout, 1, 1);
  this->TopLayout->addLayout(this->ChartLayout);

  // Add the chart to its place in the layout.
  this->Charts->setObjectName("ChartArea");
  this->ChartLayout->addWidget(this->Charts);

  this->setFocusPolicy(Qt::WheelFocus);
}

pqChartWidget::~pqChartWidget()
{
}

void pqChartWidget::setTitle(pqChartTitle *title)
{
  if(this->Title != title)
    {
    if(this->Title)
      {
      // Remove the current title from the layout.
      this->Title->hide();
      this->TitleLayout->removeWidget(this->Title);
      }

    this->Title = title;
    if(this->Title)
      {
      // Make sure the new title has the proper parent. Then, insert
      // the new title in the layout.
      this->Title->setParent(this);
      this->TitleLayout->insertWidget(0, this->Title);
      this->Title->show();
      }

    emit this->newChartTitle(this->Title);
    }
}

void pqChartWidget::setLegend(pqChartLegend *legend)
{
  if(this->Legend != legend)
    {
    if(this->Legend)
      {
      // Remove the current legend from the layout.
      this->disconnect(this->Legend, 0, this, 0);
      this->Legend->hide();
      this->LegendLayout->removeWidget(this->Legend);
      }

    this->Legend = legend;
    if(this->Legend)
      {
      this->Legend->setParent(this);
      if(this->Legend->getLocation() == pqChartLegend::Left)
        {
        this->LegendLayout->addWidget(this->Legend, 1, 0);
        }
      else if(this->Legend->getLocation() == pqChartLegend::Top)
        {
        this->LegendLayout->addWidget(this->Legend, 0, 1);
        }
      else if(this->Legend->getLocation() == pqChartLegend::Right)
        {
        this->LegendLayout->addWidget(this->Legend, 1, 2);
        }
      else if(this->Legend->getLocation() == pqChartLegend::Bottom)
        {
        this->LegendLayout->addWidget(this->Legend, 3, 1);
        }

      this->connect(this->Legend, SIGNAL(locationChanged()),
          this, SLOT(changeLegendLocation()));
      this->Legend->show();
      }

    emit this->newChartLegend(this->Legend);
    }
}

pqChartTitle *pqChartWidget::getAxisTitle(pqChartAxis::AxisLocation axis) const
{
  if(axis == pqChartAxis::Left)
    {
    return this->LeftTitle;
    }
  else if(axis == pqChartAxis::Top)
    {
    return this->TopTitle;
    }
  else if(axis == pqChartAxis::Right)
    {
    return this->RightTitle;
    }
  else
    {
    return this->BottomTitle;
    }
}

void pqChartWidget::setAxisTitle(pqChartAxis::AxisLocation axis,
    pqChartTitle *title)
{
  if(axis == pqChartAxis::Left)
    {
    if(this->LeftTitle != title)
      {
      if(this->LeftTitle)
        {
        this->LeftTitle->hide();
        this->ChartLayout->removeWidget(this->LeftTitle);
        }

      this->LeftTitle = title;
      if(this->LeftTitle)
        {
        this->LeftTitle->setParent(this);
        this->LeftTitle->setOrientation(Qt::Vertical);
        this->ChartLayout->insertWidget(0, this->LeftTitle);
        this->LeftTitle->show();
        }

      emit this->newAxisTitle(axis, this->LeftTitle);
      }
    }
  else if(axis == pqChartAxis::Top)
    {
    if(this->TopTitle != title)
      {
      if(this->TopTitle)
        {
        this->TopTitle->hide();
        this->TopLayout->removeWidget(this->TopTitle);
        }

      this->TopTitle = title;
      if(this->TopTitle)
        {
        this->TopTitle->setParent(this);
        this->TopTitle->setOrientation(Qt::Horizontal);
        this->TopLayout->insertWidget(0, this->TopTitle);
        this->TopTitle->show();
        }

      emit this->newAxisTitle(axis, this->TopTitle);
      }
    }
  else if(axis == pqChartAxis::Right)
    {
    if(this->RightTitle != title)
      {
      if(this->RightTitle)
        {
        this->RightTitle->hide();
        this->ChartLayout->removeWidget(this->RightTitle);
        }

      this->RightTitle = title;
      if(this->RightTitle)
        {
        this->RightTitle->setParent(this);
        this->RightTitle->setOrientation(Qt::Vertical);
        this->ChartLayout->addWidget(this->RightTitle);
        this->RightTitle->show();
        }

      emit this->newAxisTitle(axis, this->RightTitle);
      }
    }
  else if(this->BottomTitle != title)
    {
    if(this->BottomTitle)
      {
      this->BottomTitle->hide();
      this->TopLayout->removeWidget(this->BottomTitle);
      }

    this->BottomTitle = title;
    if(this->BottomTitle)
      {
      this->BottomTitle->setParent(this);
      this->BottomTitle->setOrientation(Qt::Horizontal);
      this->TopLayout->addWidget(this->BottomTitle);
      this->BottomTitle->show();
      }

    emit this->newAxisTitle(axis, this->BottomTitle);
    }
}

QSize pqChartWidget::sizeHint() const
{
  this->ensurePolished();
  return QSize(150, 150);
}

void pqChartWidget::printChart(QPrinter &printer)
{
  // Set up the painter for the printer.
  QSize viewportSize = this->size();
  viewportSize.scale(printer.pageRect().size(), Qt::KeepAspectRatio);

  QPainter painter(&printer);
  painter.setWindow(this->rect());
  painter.setViewport(QRect(QPoint(0, 0), viewportSize));

  // Print each of the child components.
  if(this->Title)
    {
    painter.save();
    painter.translate(this->Title->mapToParent(QPoint(0, 0)));
    this->Title->drawTitle(painter);
    painter.restore();
    }

  if(this->Legend)
    {
    painter.save();
    painter.translate(this->Legend->mapToParent(QPoint(0, 0)));
    this->Legend->drawLegend(painter);
    painter.restore();
    }

  if(this->LeftTitle)
    {
    painter.save();
    painter.translate(this->LeftTitle->mapToParent(QPoint(0, 0)));
    this->LeftTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->TopTitle)
    {
    painter.save();
    painter.translate(this->TopTitle->mapToParent(QPoint(0, 0)));
    this->TopTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->RightTitle)
    {
    painter.save();
    painter.translate(this->RightTitle->mapToParent(QPoint(0, 0)));
    this->RightTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->BottomTitle)
    {
    painter.save();
    painter.translate(this->BottomTitle->mapToParent(QPoint(0, 0)));
    this->BottomTitle->drawTitle(painter);
    painter.restore();
    }

  painter.translate(this->Charts->mapToParent(QPoint(0, 0)));
  this->Charts->drawChart(painter, this->Charts->rect());
}

void pqChartWidget::saveChart(const QStringList &files)
{
  QStringList::ConstIterator iter = files.begin();
  for( ; iter != files.end(); ++iter)
    {
    this->saveChart(*iter);
    }
}

void pqChartWidget::saveChart(const QString &filename)
{
  if(filename.endsWith(".pdf", Qt::CaseInsensitive))
    {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filename);
    this->printChart(printer);
    }
  else
    {
    QPixmap grab = QPixmap::grabWidget(this);
    grab.save(filename);
    }
}

void pqChartWidget::changeLegendLocation()
{
  // Remove the legend from its current location.
  this->LegendLayout->removeWidget(this->Legend);

  // Put the legend back in the appropriate spot.
  if(this->Legend->getLocation() == pqChartLegend::Left)
    {
    this->LegendLayout->addWidget(this->Legend, 1, 0);
    }
  else if(this->Legend->getLocation() == pqChartLegend::Top)
    {
    this->LegendLayout->addWidget(this->Legend, 0, 1);
    }
  else if(this->Legend->getLocation() == pqChartLegend::Right)
    {
    this->LegendLayout->addWidget(this->Legend, 1, 2);
    }
  else if(this->Legend->getLocation() == pqChartLegend::Bottom)
    {
    this->LegendLayout->addWidget(this->Legend, 3, 1);
    }
}


