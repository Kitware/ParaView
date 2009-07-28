/*=========================================================================

   Program: ParaView
   Module:    pqChartPlugin.cxx

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

#if defined(NDEBUG) && !defined(QT_NO_DEBUG)
# define QT_NO_DEBUG
#endif

#include "QtChartExport.h"
#ifndef QTCHART_BUILD_SHARED_LIBS
# define QT_STATICPLUGIN
#endif

#include "pqChartPlugin.h"
#include "pqChartWidget.h"
#include "pqChartWidgetPlugin.h"
#include "pqColorMapWidget.h"
#include "pqColorMapWidgetPlugin.h"


//----------------------------------------------------------------------------
pqChartWidgetPlugin::pqChartWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqChartWidgetPlugin::createWidget(QWidget *p)
{
  return new pqChartWidget(p);
}

QString pqChartWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqChartWidget\" name=\"pqChart\">\n"
      " <property name=\"geometry\">\n"
      "  <rect>\n"
      "   <x>0</x>\n"
      "   <y>0</y>\n"
      "   <width>100</width>\n"
      "   <height>100</height>\n"
      "  </rect>\n"
      " </property>\n"
      "</widget>\n");
}

QIcon pqChartWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqHistogram22.png"));
}

QString pqChartWidgetPlugin::includeFile() const
{
  return QLatin1String("pqChartWidget.h");
}

QString pqChartWidgetPlugin::toolTip() const
{
  return QLatin1String("Qt Chart");
}

QString pqChartWidgetPlugin::whatsThis() const
{
  return QLatin1String("Qt Chart");
}


//----------------------------------------------------------------------------
pqColorMapWidgetPlugin::pqColorMapWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqColorMapWidgetPlugin::createWidget(QWidget *p)
{
  return new pqColorMapWidget(p);
}

QString pqColorMapWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqColorMapWidget\" name=\"pqColorMap\">\n"
      " <property name=\"geometry\">\n"
      "  <rect>\n"
      "   <x>0</x>\n"
      "   <y>0</y>\n"
      "   <width>100</width>\n"
      "   <height>50</height>\n"
      "  </rect>\n"
      " </property>\n"
      "</widget>\n");
}

QIcon pqColorMapWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqColorMap22.png"));
}

QString pqColorMapWidgetPlugin::includeFile() const
{
  return QLatin1String("pqColorMapWidget.h");
}

QString pqColorMapWidgetPlugin::toolTip() const
{
  return QLatin1String("Color Map Widget");
}

QString pqColorMapWidgetPlugin::whatsThis() const
{
  return QLatin1String("A Color Map Widget.");
}


//----------------------------------------------------------------------------
pqChartPlugin::pqChartPlugin(QObject *p)
  : QObject(p), QDesignerCustomWidgetCollectionInterface()
{
  this->Chart = new pqChartWidgetPlugin();
  this->ColorMap = new pqColorMapWidgetPlugin();
}

pqChartPlugin::~pqChartPlugin()
{
  delete this->Chart;
  delete this->ColorMap;
}

QList<QDesignerCustomWidgetInterface*> pqChartPlugin::customWidgets() const
{
  QList<QDesignerCustomWidgetInterface*> plugins;
  plugins.append(this->Chart);
  plugins.append(this->ColorMap);
  return plugins;
}

Q_EXPORT_PLUGIN2(QtChart, pqChartPlugin)
