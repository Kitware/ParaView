/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqChartPlugin_h
#define _pqChartPlugin_h

#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/QDesignerCustomWidgetCollectionInterface>
#include <QtCore/qplugin.h>
#include <QObject>

class pqHistogramWidgetPlugin;
class pqLineChartWidgetPlugin;

class pqChartPlugin : public QObject,
    public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
  pqChartPlugin(QObject *parent=0);
  virtual ~pqChartPlugin();

  virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
  pqHistogramWidgetPlugin *Histogram;
  pqLineChartWidgetPlugin *LineChart;
};

#endif
