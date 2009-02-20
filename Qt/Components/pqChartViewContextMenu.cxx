/*=========================================================================

   Program: ParaView
   Module:    pqChartViewContextMenu.cxx

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

#include "pqChartViewContextMenu.h"

#include "pqActiveViewOptionsManager.h"
#include "pqView.h"

#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QWidget>

#include "vtkQtChartArea.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartTitle.h"
#include "vtkQtChartWidget.h"


pqChartViewContextMenu::pqChartViewContextMenu(pqView *view,
    pqActiveViewOptionsManager *manager)
  : QObject(view)
{
  this->View = view;
  this->Chart = qobject_cast<vtkQtChartWidget *>(this->View->getWidget());
  this->Manager = manager;
  this->Point = new QPoint();
  this->Page = new QString();

  // Set up the chart area context menu.
  vtkQtChartArea *area = this->Chart->getChartArea();
  area->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(area, SIGNAL(customContextMenuRequested(const QPoint &)),
      this, SLOT(showContextMenu(const QPoint &)));

  // Listen for chart widget changes.
  this->connect(this->Chart, SIGNAL(newChartTitle(vtkQtChartTitle *)),
      this, SLOT(setupTitleMenu(vtkQtChartTitle *)));
  this->connect(this->Chart, SIGNAL(newChartLegend(vtkQtChartLegend *)),
      this, SLOT(setupLegendMenu(vtkQtChartLegend *)));
  this->connect(this->Chart,
      SIGNAL(newAxisTitle(vtkQtChartAxis::AxisLocation, vtkQtChartTitle *)),
      this,
      SLOT(setupAxisTitleMenu(vtkQtChartAxis::AxisLocation, vtkQtChartTitle *)));

  // Set up the context menus for any title or legend objects already
  // instantiated.
  this->setupTitleMenu(this->Chart->getTitle());
  this->setupLegendMenu(this->Chart->getLegend());
  this->setupAxisTitleMenu(vtkQtChartAxis::Left,
      this->Chart->getAxisTitle(vtkQtChartAxis::Left));
  this->setupAxisTitleMenu(vtkQtChartAxis::Bottom,
      this->Chart->getAxisTitle(vtkQtChartAxis::Bottom));
  this->setupAxisTitleMenu(vtkQtChartAxis::Right,
      this->Chart->getAxisTitle(vtkQtChartAxis::Right));
  this->setupAxisTitleMenu(vtkQtChartAxis::Top,
      this->Chart->getAxisTitle(vtkQtChartAxis::Top));
}

pqChartViewContextMenu::~pqChartViewContextMenu()
{
  delete this->Point;
  delete this->Page;
}

const QString &pqChartViewContextMenu::getChartLayerPage() const
{
  return *this->Page;
}

void pqChartViewContextMenu::setChartLayerPage(const QString &page)
{
  *this->Page = page;
}

void pqChartViewContextMenu::showContextMenu(const QPoint &location)
{
  *this->Point = location;
  QMenu menu;
  menu.setObjectName("ChartAreaContextMenu");
  this->addCommonActions(&menu);

  menu.addAction("&Properties", this, SLOT(showChartAreaProperties()));

  menu.exec(this->Chart->getChartArea()->mapToGlobal(location));
}

void pqChartViewContextMenu::setupTitleMenu(vtkQtChartTitle *title)
{
  if(title && title->contextMenuPolicy() != Qt::ActionsContextMenu)
    {
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

void pqChartViewContextMenu::setupLegendMenu(vtkQtChartLegend *legend)
{
  if(legend && legend->contextMenuPolicy() != Qt::ActionsContextMenu)
    {
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

void pqChartViewContextMenu::setupAxisTitleMenu(
    vtkQtChartAxis::AxisLocation axis, vtkQtChartTitle *title)
{
  if(title && title->contextMenuPolicy() != Qt::ActionsContextMenu)
    {
    title->setContextMenuPolicy(Qt::ActionsContextMenu);
    this->addCommonActions(title);
    QAction *action = new QAction("&Properties", title);
    action->setObjectName("PropertiesAction");
    if(axis == vtkQtChartAxis::Left)
      {
      action->setData(QVariant(QString("Left Axis.Title")));
      }
    else if(axis == vtkQtChartAxis::Bottom)
      {
      action->setData(QVariant(QString("Bottom Axis.Title")));
      }
    else if(axis == vtkQtChartAxis::Right)
      {
      action->setData(QVariant(QString("Right Axis.Title")));
      }
    else if(axis == vtkQtChartAxis::Top)
      {
      action->setData(QVariant(QString("Top Axis.Title")));
      }

    this->connect(action, SIGNAL(triggered()),
        this, SLOT(showOtherProperties()));
    title->addAction(action);
    }
}

void pqChartViewContextMenu::showChartAreaProperties()
{
  if(this->Chart && this->Manager)
    {
    // Use the context menu location to determine which part of the
    // chart was clicked.
    vtkQtChartArea *area = this->Chart->getChartArea();
    vtkQtChartAxisLayer *layer = area->getAxisLayer();
    const char *pages[] =
      {
      "Left Axis",
      "Bottom Axis",
      "Right Axis",
      "Top Axis"
      };

    vtkQtChartAxis::AxisLocation location[] =
      {
      vtkQtChartAxis::Left,
      vtkQtChartAxis::Bottom,
      vtkQtChartAxis::Right,
      vtkQtChartAxis::Top
      };

    QString page;
    for(int i = 0; i < 4; i++)
      {
      vtkQtChartAxis *axis = layer->getAxis(location[i]);
      if(axis)
        {
        QRectF bounds = axis->getBounds();
        if(bounds.contains(*this->Point))
          {
          page = pages[i];
          break;
          }
        }
      }

    // If the location is not on one of the axes, use the chart layer
    // options page.
    if(page.isEmpty())
      {
      page = this->getChartOptionsPage(*this->Point);
      }

    this->Manager->showOptions(page);
    }
}

void pqChartViewContextMenu::showOtherProperties()
{
  // Get the properties page path from the action.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(this->Manager && action)
    {
    QString page = action->data().toString();
    this->Manager->showOptions(page);
    }
}

const QString &pqChartViewContextMenu::getChartOptionsPage(const QPoint &)
{
  return this->getChartLayerPage();
}

void pqChartViewContextMenu::addCommonActions(QWidget *widget)
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


