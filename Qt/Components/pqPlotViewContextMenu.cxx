/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewContextMenu.cxx

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

/// \file pqPlotViewContextMenu.cxx
/// \date 9/18/2007

#include "pqPlotViewContextMenu.h"

#include "pqActiveViewOptionsManager.h"
#include "pqChartArea.h"
#include "pqChartLegend.h"
#include "pqChartTitle.h"
#include "pqChartWidget.h"
#include "pqPlotView.h"

#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QRect>
#include <QString>


pqPlotViewContextMenu::pqPlotViewContextMenu(pqPlotView *view,
    pqActiveViewOptionsManager *manager)
  : QObject(view)
{
  this->View = view;
  this->Manager = manager;
  this->Point = new QPoint();

  // Set up the chart area context menu.
  pqChartWidget *chart = qobject_cast<pqChartWidget *>(
      this->View->getWidget());
  pqChartArea *area = chart->getChartArea();
  area->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(area, SIGNAL(customContextMenuRequested(const QPoint &)),
      this, SLOT(showContextMenu(const QPoint &)));

  // Set up the context menus for any title or legend objects already
  // instantiated.
  this->connect(chart, SIGNAL(newChartTitle(pqChartTitle *)),
      this, SLOT(setupChartTitle(pqChartTitle *)));
  this->connect(chart, SIGNAL(newChartLegend(pqChartLegend *)),
      this, SLOT(setupChartLegend(pqChartLegend *)));
  this->connect(chart,
      SIGNAL(newAxisTitle(pqChartAxis::AxisLocation, pqChartTitle *)),
      this, SLOT(setupAxisTitle(pqChartAxis::AxisLocation, pqChartTitle *)));

  this->setupChartTitle(chart->getTitle());
  this->setupChartLegend(chart->getLegend());
  this->setupAxisTitle(pqChartAxis::Left,
      chart->getAxisTitle(pqChartAxis::Left));
  this->setupAxisTitle(pqChartAxis::Bottom,
      chart->getAxisTitle(pqChartAxis::Bottom));
  this->setupAxisTitle(pqChartAxis::Right,
      chart->getAxisTitle(pqChartAxis::Right));
  this->setupAxisTitle(pqChartAxis::Top,
      chart->getAxisTitle(pqChartAxis::Top));
}

pqPlotViewContextMenu::~pqPlotViewContextMenu()
{
  delete this->Point;
}

void pqPlotViewContextMenu::showContextMenu(const QPoint &location)
{
  *this->Point = location;
  QMenu menu;
  menu.setObjectName("ChartAreaContextMenu");
  this->addCommonActions(&menu);

  menu.addAction("&Properties", this, SLOT(showChartAreaProperties()));

  pqChartWidget *chart = qobject_cast<pqChartWidget *>(
      this->View->getWidget());
  menu.exec(chart->getChartArea()->mapToGlobal(location));
}

void pqPlotViewContextMenu::showChartAreaProperties()
{
  if(this->View && this->Manager)
    {
    // Use the context menu location to determine which part of the
    // chart was clicked.
    pqChartWidget *chart = qobject_cast<pqChartWidget *>(
        this->View->getWidget());
    pqChartArea *area = chart->getChartArea();
    const char *pages[] =
      {
      "Left Axis",
      "Bottom Axis",
      "Right Axis",
      "Top Axis"
      };

    pqChartAxis::AxisLocation location[] =
      {
      pqChartAxis::Left,
      pqChartAxis::Bottom,
      pqChartAxis::Right,
      pqChartAxis::Top
      };

    QString page;
    for(int i = 0; i < 4; i++)
      {
      pqChartAxis *axis = area->getAxis(location[i]);
      if(axis)
        {
        QRect bounds;
        axis->getBounds(bounds);
        if(bounds.contains(*this->Point))
          {
          page = pages[i];
          break;
          }
        }
      }

    this->Manager->showOptions(page);
    }
}

void pqPlotViewContextMenu::showOtherProperties()
{
  // Get the properties page path from the action.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(this->Manager && action)
    {
    QString page = action->data().toString();
    this->Manager->showOptions(page);
    }
}

void pqPlotViewContextMenu::setupChartTitle(pqChartTitle *title)
{
  if(title)
    {
    pqChartWidget *chart = qobject_cast<pqChartWidget *>(
        this->View->getWidget());
    this->disconnect(chart, SIGNAL(newChartTitle(pqChartTitle *)),
        this, SLOT(setupChartTitle(pqChartTitle *)));

    title->setContextMenuPolicy(Qt::ActionsContextMenu);
    this->addCommonActions(title);
    QAction *action = new QAction("&Properties", title);
    action->setObjectName("PropertiesAction");
    action->setData(QVariant(QString()));
    this->connect(action, SIGNAL(triggered()),
        this, SLOT(showOtherProperties()));
    title->addAction(action);
    }
}

void pqPlotViewContextMenu::setupChartLegend(pqChartLegend *legend)
{
  if(legend)
    {
    pqChartWidget *chart = qobject_cast<pqChartWidget *>(
        this->View->getWidget());
    this->disconnect(chart, SIGNAL(newChartLegend(pqChartLegend *)),
        this, SLOT(setupChartLegend(pqChartLegend *)));

    legend->setContextMenuPolicy(Qt::ActionsContextMenu);
    this->addCommonActions(legend);
    QAction *action = new QAction("&Properties", legend);
    action->setObjectName("PropertiesAction");
    action->setData(QVariant(QString()));
    this->connect(action, SIGNAL(triggered()),
        this, SLOT(showOtherProperties()));
    legend->addAction(action);
    }
}

void pqPlotViewContextMenu::setupAxisTitle(pqChartAxis::AxisLocation axis,
    pqChartTitle *title)
{
  if(title && title->contextMenuPolicy() != Qt::ActionsContextMenu)
    {
    title->setContextMenuPolicy(Qt::ActionsContextMenu);
    this->addCommonActions(title);
    QAction *action = new QAction("&Properties", title);
    action->setObjectName("PropertiesAction");
    if(axis == pqChartAxis::Left)
      {
      action->setData(QVariant(QString("Left Axis.Title")));
      }
    else if(axis == pqChartAxis::Bottom)
      {
      action->setData(QVariant(QString("Bottom Axis.Title")));
      }
    else if(axis == pqChartAxis::Right)
      {
      action->setData(QVariant(QString("Right Axis.Title")));
      }
    else if(axis == pqChartAxis::Top)
      {
      action->setData(QVariant(QString("Top Axis.Title")));
      }

    this->connect(action, SIGNAL(triggered()),
        this, SLOT(showOtherProperties()));
    title->addAction(action);
    }
}

void pqPlotViewContextMenu::addCommonActions(QWidget *widget)
{
  QAction *action = new QAction("&Undo Camera", widget);
  action->setObjectName("UndoAction");
  this->connect(action, SIGNAL(triggered()), this->View, SLOT(undo()));
  this->connect(this->View, SIGNAL(canUndoChanged(bool)),
      action, SLOT(setEnabled(bool)));
  action->setEnabled(this->View->canUndo());
  widget->addAction(action);

  action = new QAction("&Redo Camera", widget);
  action->setObjectName("RedoAction");
  this->connect(action, SIGNAL(triggered()), this->View, SLOT(redo()));
  this->connect(this->View, SIGNAL(canRedoChanged(bool)),
      action, SLOT(setEnabled(bool)));
  action->setEnabled(this->View->canRedo());
  widget->addAction(action);

  action = new QAction("&Save Screenshot", widget);
  action->setObjectName("ScreenshotAction");
  this->connect(action, SIGNAL(triggered()),
      this, SIGNAL(screenshotRequested()));
  widget->addAction(action);

  action = new QAction(widget);
  action->setSeparator(true);
  widget->addAction(action);
}


