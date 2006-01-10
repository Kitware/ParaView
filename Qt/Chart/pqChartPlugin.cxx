/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#if !defined(_DEBUG)
# if !defined(QT_NO_DEBUG)
#  define QT_NO_DEBUG
# endif
#endif

#include "pqChartPlugin.h"
#include "pqHistogramWidget.h"
#include "pqHistogramWidgetPlugin.h"
#include "pqLineChartWidget.h"
#include "pqLineChartWidgetPlugin.h"


pqHistogramWidgetPlugin::pqHistogramWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqHistogramWidgetPlugin::createWidget(QWidget *p)
{
  return new pqHistogramWidget(p);
}

QString pqHistogramWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqHistogramWidget\" name=\"pqHistogram\">\n"
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

QIcon pqHistogramWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqHistogram22.png"));
}

QString pqHistogramWidgetPlugin::includeFile() const
{
  return QLatin1String("pqHistogramWidget.h");
}

QString pqHistogramWidgetPlugin::toolTip() const
{
  return QLatin1String("Qt Histogram");
}

QString pqHistogramWidgetPlugin::whatsThis() const
{
  return QLatin1String("Qt Line Chart");
}


pqLineChartWidgetPlugin::pqLineChartWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqLineChartWidgetPlugin::createWidget(QWidget *p)
{
  return new pqLineChartWidget(p);
}

QString pqLineChartWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqLineChartWidget\" name=\"pqLineChart\">\n"
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

QIcon pqLineChartWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqLineChart22.png"));
}

QString pqLineChartWidgetPlugin::includeFile() const
{
  return QLatin1String("pqLineChartWidget.h");
}

QString pqLineChartWidgetPlugin::toolTip() const
{
  return QLatin1String("Qt Line Chart");
}

QString pqLineChartWidgetPlugin::whatsThis() const
{
  return QLatin1String("A Qt Line Chart.");
}


pqChartPlugin::pqChartPlugin(QObject *p)
  : QObject(p), QDesignerCustomWidgetCollectionInterface()
{
  this->Histogram = new pqHistogramWidgetPlugin();
  this->LineChart = new pqLineChartWidgetPlugin();
}

pqChartPlugin::~pqChartPlugin()
{
  if(this->Histogram)
    delete this->Histogram;
  if(this->LineChart)
    delete this->LineChart;
}

QList<QDesignerCustomWidgetInterface*> pqChartPlugin::customWidgets() const
{
  QList<QDesignerCustomWidgetInterface*> plugins;
  plugins.append(Histogram);
  plugins.append(LineChart);
  return plugins;
}

Q_EXPORT_PLUGIN(pqChartPlugin)
