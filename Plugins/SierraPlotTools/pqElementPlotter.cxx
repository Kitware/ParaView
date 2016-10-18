// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqElementPlotter.cxx

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

#include "pqElementPlotter.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"

#include "pqPlotVariablesDialog.h"

#include <QStringList>

#include <QStringList>
#include <QtDebug>

#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSelectionNode.h"

///
/// pqElementPlotter
///

//-----------------------------------------------------------------------------
QStringList pqElementPlotter::getTheVars(vtkSMProxy* meshReaderProxy)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("ElementVariablesInfo");

  return getStringsFromProperty(prop);
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqElementPlotter::getSMVariableProperty(vtkSMProxy* meshReaderProxy)
{
  return this->getSMNamedVariableProperty(meshReaderProxy, QString("ElementVariables"));
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* pqElementPlotter::getDataSetAttributesInformation(
  vtkPVDataInformation* pvDataInfo)
{
  return pvDataInfo->GetCellDataInformation();
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqElementPlotter::getArrayInformation(
  vtkPVDataSetAttributesInformation* pvDataSetAttributesInformation)
{
  return pvDataSetAttributesInformation->GetArrayInformation("GlobalElementId");
}

//-----------------------------------------------------------------------------
bool pqElementPlotter::amIAbleToSelectByNumber()
{
  return true;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqElementPlotter::getPlotFilter()
{
  return this->findPipelineSource("ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
void pqElementPlotter::setVarsStatus(vtkSMProxy* meshReaderProxy, bool flag)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("ElementVariables");

  setVarElementsStatus(prop, flag);
}

//-----------------------------------------------------------------------------
void pqElementPlotter::setVarsActive(vtkSMProxy* meshReaderProxy, QString varName, bool activeFlag)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("ElementVariables");

  setVarElementsActive(prop, varName, activeFlag);

  meshReaderProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QString pqElementPlotter::getFilterName()
{
  return QString("ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
QMap<QString, QList<pqOutputPort*> > pqElementPlotter::buildNamedInputs(
  pqPipelineSource* meshReader, QList<int> itemList, bool& success)
{
  success = false;

  QMap<QString, QList<pqOutputPort*> > namedInputs =
    pqPlotter::buildNamedInputs(meshReader, itemList, success);
  if (!success)
  {
    return namedInputs;
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  // build a vtkPVSelectionSource filter
  pqPipelineSource* selectionSource = NULL;
  pqObjectBuilder* builder = core->getObjectBuilder();
  selectionSource =
    builder->createSource("sources", "GlobalIDSelectionSource", this->getActiveServer());
  vtkSMProxy* sourceProxy = selectionSource->getProxy();

  // inputs.push_back(selectionSource->getOutputPort(0));
  QList<pqOutputPort*> selectionInput;
  selectionInput.push_back(selectionSource->getOutputPort(0));
  namedInputs["Selection"] = selectionInput;

  vtkSMProperty* prop = sourceProxy->GetProperty("IDs");
  vtkSMVectorProperty* vecProp = dynamic_cast<vtkSMVectorProperty*>(prop);

  if (vecProp != NULL)
  {
    vtkSMIdTypeVectorProperty* idTypeProp = dynamic_cast<vtkSMIdTypeVectorProperty*>(vecProp);
    if (idTypeProp != NULL)
    {
      // add in all of them
      int i;
      for (i = 0; i < itemList.size(); i++)
      {
        int val = itemList[i];
        idTypeProp->SetElement(i, val);
      }
    }
  }
  else
  {
    qWarning() << "pqElementPlotter::buildNamedInputs: ERROR - can not find IDs in mesh ";
    success = false;
    return namedInputs;
  }

  vtkSMProperty* fieldTypeProp = sourceProxy->GetProperty("FieldType");
  if (fieldTypeProp != NULL)
  {
    vtkSMIntVectorProperty* intVecProp = dynamic_cast<vtkSMIntVectorProperty*>(fieldTypeProp);
    if (intVecProp != NULL)
    {
      // set field type to cell
      intVecProp->SetElement(0, vtkSelectionNode::CELL);
    }
  }

  return namedInputs;
}

//-----------------------------------------------------------------------------
QString pqElementPlotter::getNumberItemsLabel()
{
  return QString("select element #");
}

///////////////////////////////////////////////////////////////////////////////
QString pqElementPlotter::getPlotterTextEditObjectName()
{
  return QString("elementVarsVsTimeTextEdit");
}
