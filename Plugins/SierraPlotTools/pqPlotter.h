
// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPlotter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef pqPlotter_h
#define pqPlotter_h

#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>

class QLabel;
class pqOutputPort;
class pqPipelineSource;
class pqServer;
class pqView;
class QAction;
class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMProperty;
class vtkSMProxy;

//=============================================================================
class pqPlotter : public QObject
{
  Q_OBJECT;

public:
  enum plotDomain
  {
    eTime,
    ePath,
    eVariable
  };

  pqPlotter();

  virtual ~pqPlotter();

  virtual QStringList getTheVars(vtkSMProxy* meshReaderProxy);

  virtual void setDomain(plotDomain theDomain);

  virtual plotDomain getDomain();

  virtual vtkSMProperty* getSMVariableProperty(vtkSMProxy* meshReaderProxy);

  virtual vtkPVDataSetAttributesInformation* getDataSetAttributesInformation(
    vtkPVDataInformation* pvDataInfo);

  virtual vtkPVArrayInformation* getArrayInformation(vtkPVDataSetAttributesInformation*);

  virtual bool amIAbleToSelectByNumber();

  virtual pqPipelineSource* getPlotFilter();

  virtual pqPipelineSource* findPipelineSource(const char* SMName);

  virtual pqServer* getActiveServer();

  pqView* getPlotView(pqPipelineSource* plotFilter);

  pqView* getMeshView(pqPipelineSource* meshReader);

  pqView* findView(pqPipelineSource* source, int port, const QString& viewType);

  virtual void setVarsStatus(vtkSMProxy* meshReaderProxy, bool flag);

  virtual void setVarsActive(vtkSMProxy* meshReaderProxy, QString varName, bool activeFlag);

  virtual QString getFilterName();

  virtual QMap<QString, QList<pqOutputPort*> > buildNamedInputs(
    pqPipelineSource* meshReader, QList<int> itemList, bool& success);

  virtual QString getNumberItemsLabel();

  // for plots that need some sort of selection range
  virtual bool selectionWithinRange(QList<int> selectedItems, pqPipelineSource* meshReader);

  void setDisplayOfVariables(pqPipelineSource* meshReader, const QMap<QString, QString>& vars);

  virtual void popUpHelp();

  virtual QString getPlotterHeadingHoverText();

  virtual QString getPlotterTextEditObjectName();

signals:
  void activateAllVariables(pqPlotter*);

protected:
  virtual QStringList getStringsFromProperty(vtkSMProperty* prop);

  virtual void setVarElementsStatus(vtkSMProperty* prop, bool flag);

  virtual void setVarElementsActive(vtkSMProperty* prop, QString varName, bool activeFlag);

  virtual vtkSMProperty* getSMNamedVariableProperty(vtkSMProxy* meshReaderProxy, QString propName);

  plotDomain domain;

  class pqInternal;
  pqInternal* Internal;
};

#endif // pqPlotter_h
