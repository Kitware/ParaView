/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqLineChartWidgetPlugin_h
#define _pqLineChartWidgetPlugin_h

#include <QObject>
#include <QtDesigner/QDesignerCustomWidgetInterface>


class pqLineChartWidgetPlugin : public QObject,
    public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
  pqLineChartWidgetPlugin(QObject *parent=0);
  virtual ~pqLineChartWidgetPlugin() {}

  virtual QWidget *createWidget(QWidget *parent=0);
  virtual QString domXml() const;
  virtual QString group() const {return QLatin1String("ParaQ Charts");}
  virtual QIcon icon() const;
  virtual QString includeFile() const;
  virtual bool isContainer() const {return false;}
  virtual QString name() const {return QLatin1String("pqLineChartWidget");}
  virtual QString toolTip() const;
  virtual QString whatsThis() const;
};

#endif
