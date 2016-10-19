
// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqNodePlotter.h

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

#ifndef pqNodePlotter_h
#define pqNodePlotter_h

#include "pqPlotter.h"
#include <QString>
class pqOutputPort;

class pqNodePlotter : public pqPlotter
{
  Q_OBJECT;

public:
  pqNodePlotter() {}

  virtual ~pqNodePlotter() {}

  virtual QStringList getTheVars(vtkSMProxy* meshReaderProxy);

  virtual vtkSMProperty* getSMVariableProperty(vtkSMProxy* meshReaderProxy);

  virtual vtkPVDataSetAttributesInformation* getDataSetAttributesInformation(
    vtkPVDataInformation* pvDataInfo);

  virtual vtkPVArrayInformation* getArrayInformation(vtkPVDataSetAttributesInformation*);

  virtual bool amIAbleToSelectByNumber();

  virtual pqPipelineSource* getPlotFilter();

  virtual void setVarsStatus(vtkSMProxy* meshReaderProxy, bool flag);

  virtual void setVarsActive(vtkSMProxy* meshReaderProxy, QString varName, bool activeFlag);

  virtual QString getFilterName();

  virtual QMap<QString, QList<pqOutputPort*> > buildNamedInputs(
    pqPipelineSource* meshReader, QList<int> itemList, bool& success);

  virtual QString getNumberItemsLabel();

  virtual QString getPlotterTextEditObjectName();
};

//=============================================================================
// class pqSierraPlotToolsManager::pqInternal::NodePlotter

#endif // pqNodePlotter_h
