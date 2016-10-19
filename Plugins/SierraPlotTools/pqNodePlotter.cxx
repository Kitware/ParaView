// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqNodePlotter.cxx

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

#include "pqNodePlotter.h"
#include "pqOutputPort.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPlotVariablesDialog.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

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

// used to show line number in #pragma messages
#define STRING2(x) #x
#define STRING(x) STRING2(x)

///
/// pqNodePlotter
///

QStringList pqNodePlotter::getTheVars(vtkSMProxy* meshReaderProxy)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("PointVariablesInfo");

  return getStringsFromProperty(prop);
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqNodePlotter::getSMVariableProperty(vtkSMProxy* meshReaderProxy)
{
  return this->getSMNamedVariableProperty(meshReaderProxy, QString("PointVariables"));
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* pqNodePlotter::getDataSetAttributesInformation(
  vtkPVDataInformation* pvDataInfo)
{
  return pvDataInfo->GetPointDataInformation();
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqNodePlotter::getArrayInformation(
  vtkPVDataSetAttributesInformation* pvDataSetAttributesInformation)
{
  return pvDataSetAttributesInformation->GetArrayInformation("GlobalNodeId");
}

//-----------------------------------------------------------------------------
bool pqNodePlotter::amIAbleToSelectByNumber()
{
  return true;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqNodePlotter::getPlotFilter()
{
  return this->findPipelineSource("ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
void pqNodePlotter::setVarsStatus(vtkSMProxy* meshReaderProxy, bool flag)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("PointVariables");

  setVarElementsStatus(prop, flag);
}

//-----------------------------------------------------------------------------
void pqNodePlotter::setVarsActive(vtkSMProxy* meshReaderProxy, QString varName, bool activeFlag)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty("PointVariables");

  setVarElementsActive(prop, varName, activeFlag);

  meshReaderProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QString pqNodePlotter::getFilterName()
{
  return QString("ExtractSelectionOverTime");
}

//-----------------------------------------------------------------------------
QMap<QString, QList<pqOutputPort*> > pqNodePlotter::buildNamedInputs(
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
    qWarning() << "pqNodePlotter::buildNamedInputs: ERROR - can not find IDs in mesh ";
    success = false;
    return namedInputs;
  }

  vtkSMProperty* fieldTypeProp = sourceProxy->GetProperty("FieldType");
  if (fieldTypeProp != NULL)
  {
    vtkSMIntVectorProperty* intVecProp = dynamic_cast<vtkSMIntVectorProperty*>(fieldTypeProp);
    if (intVecProp != NULL)
    {
      // set field type to Point
      intVecProp->SetElement(0, vtkSelectionNode::POINT);
    }
  }

  return namedInputs;
}

//-----------------------------------------------------------------------------
QString pqNodePlotter::getNumberItemsLabel()
{
  return QString("select node #");
}

///////////////////////////////////////////////////////////////////////////////
QString pqNodePlotter::getPlotterTextEditObjectName()
{
  return QString("nodeVarsVsTimeTextEdit");
}
