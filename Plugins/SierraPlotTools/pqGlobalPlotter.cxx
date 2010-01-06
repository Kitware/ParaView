// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqGlobalPlotter.cxx

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

#include "warningState.h"

#include "pqGlobalPlotter.h"

#include <QAction>
#include <QLabel>
#include <QStringList>
#include <QtDebug>

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkPVDataInformation.h"

///
/// pqGlobalPlotter::pqInternal
///

class pqGlobalPlotter::pqInternal
{
public:

  pqInternal();
  ~pqInternal();

  QAction * headingHoverAction;
};

///////////////////////////////////////////////////////////////////////////////
pqGlobalPlotter::pqInternal::pqInternal() :
  headingHoverAction(NULL)
{
}

///
/// pqGlobalPlotter
///

//-----------------------------------------------------------------------------
pqGlobalPlotter::pqGlobalPlotter()
{
  this->Internal = new pqGlobalPlotter::pqInternal();
}

//-----------------------------------------------------------------------------
QStringList pqGlobalPlotter::getTheVars(vtkSMProxy * meshReaderProxy)
{
  vtkSMProperty * prop = meshReaderProxy->GetProperty("GlobalVariablesInfo");

  return getStringsFromProperty(prop);
}

//-----------------------------------------------------------------------------
vtkSMProperty * pqGlobalPlotter::getSMVariableProperty(vtkSMProxy *meshReaderProxy)
{
  return this->getSMNamedVariableProperty(meshReaderProxy, QString("GlobalVariables"));
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation * pqGlobalPlotter::getDataSetAttributesInformation(vtkPVDataInformation * pvDataInfo)
{
  return pvDataInfo->GetFieldDataInformation();
}

//-----------------------------------------------------------------------------
bool pqGlobalPlotter::amIAbleToSelectByNumber()
{
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource * pqGlobalPlotter::getPlotFilter()
{
  return this->findPipelineSource("ExtractFieldDataOverTime");
}

//-----------------------------------------------------------------------------
void pqGlobalPlotter::setVarsStatus(vtkSMProxy * meshReaderProxy, bool flag)
{
  vtkSMProperty * prop = meshReaderProxy->GetProperty("GlobalVariables");

  setVarElementsStatus(prop, flag);
}

//-----------------------------------------------------------------------------
void pqGlobalPlotter::setVarsActive(vtkSMProxy * meshReaderProxy, QString varName, bool activeFlag)
{
  vtkSMProperty * prop = meshReaderProxy->GetProperty("GlobalVariables");

  setVarElementsActive(prop, varName, activeFlag);

  meshReaderProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QString pqGlobalPlotter::getFilterName()
{
  return QString("ExtractFieldDataOverTime");
}

///////////////////////////////////////////////////////////////////////////////
QString pqGlobalPlotter::getPlotterTextEditObjectName()
{
  return QString("globalVarsVsTimeTextEdit");
}
