// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPlotter.cxx

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

#include "pqPlotter.h"
#include "pqSierraPlotToolsManager.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPlotVariablesDialog.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqSierraPlotToolsUtils.h"
#include "pqView.h"
#include "pqXYChartView.h"
#include <pqServer.h>
#include <pqServerManagerModel.h>

#include <QLabel>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QtDebug>

#include "ui_pqSierraToolsRichTextDocs.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

// used to show line number in #pragma messages
#define STRING2(x) #x
#define STRING(x) STRING2(x)

class pqPlotter::pqInternal
{
public:
  pqInternal();
  ~pqInternal();

  QStringList validTensorComponentSuffixes;
  QStringList validSeriesComponentSuffixes;

  QString stripTensorComponent(QString variableAsString);

  QString stripSeriesComponent(QString variableAsString);

  QString seriesComponentSuffixString(QString variableAsString);

  QString tensorComponentSuffixString(QString variableAsString);

  QString tensorOrVectorSuffixToSeriesSuffix(
    QString shortVarName, QString suffix, QMap<QString, int>& seriesCounter);

  pqSierraPlotToolsUtils util;

  QMap<int, QMap<QString, QString> > tensorLookup;

  QWidget* richTextDocsPlaceholder;
  Ui::pqSierraPlotToolsRichTextDocs richTextDocs;
};

///////////////////////////////////////////////////////////////////////////////
pqPlotter::pqInternal::pqInternal()
{
  // This widget serves no real purpose other than initializing the QTextEdit
  // structure created with designer that holds the Rich Text documents that
  // we want to be able to display (as tooltip pop-ups or whatever) in this
  // application at run-time
  this->richTextDocsPlaceholder = new QWidget(NULL);
  this->richTextDocs.setupUi(this->richTextDocsPlaceholder);

  validTensorComponentSuffixes.append("_x");
  validTensorComponentSuffixes.append("_y");
  validTensorComponentSuffixes.append("_z");

  // The Symmetric Tensor – six components index order is defined
  // by the VTK Exodus reader
  //   see vtkExodusIIReaderPrivate.h
  //   and vtkExodusIIReader.cxx in VTK library

  validTensorComponentSuffixes.append("_xx");
  validTensorComponentSuffixes.append("_xy");
  // symetric -- could be xz, or zx, but Exodus reader stores as zx
  validTensorComponentSuffixes.append("_zx");
  validTensorComponentSuffixes.append("_yy");
  validTensorComponentSuffixes.append("_yz");
  validTensorComponentSuffixes.append("_zz");
  validTensorComponentSuffixes.append("_magnitude");

  validSeriesComponentSuffixes.append(" (0)");
  validSeriesComponentSuffixes.append(" (1)");
  validSeriesComponentSuffixes.append(" (2)");
  validSeriesComponentSuffixes.append(" (3)");
  validSeriesComponentSuffixes.append(" (4)");
  validSeriesComponentSuffixes.append(" (5)");
  validSeriesComponentSuffixes.append(" (Magnitude)");

  QMap<QString, QString> tensorToSeries;

  tensorToSeries["_xx"] = QString(" (0)");
  tensorToSeries["_yy"] = QString(" (1)");
  tensorToSeries["_zz"] = QString(" (2)");
  tensorToSeries["_xy"] = QString(" (3)");
  tensorToSeries["_yz"] = QString(" (4)");
  tensorToSeries["_zx"] = QString(" (5)");
  tensorToSeries["_magnitude"] = QString(" (Magnitude)");

  // The Symmetric Tensor – 6 components + 1 (magnitude)
  const int SYMETIC_SIX_PLUS_MAGNITUDE = 7;
  this->tensorLookup[SYMETIC_SIX_PLUS_MAGNITUDE] = tensorToSeries;

  tensorToSeries.clear();
  tensorToSeries["_x"] = QString(" (0)");
  tensorToSeries["_y"] = QString(" (1)");
  tensorToSeries["_z"] = QString(" (2)");
  tensorToSeries["_magnitude"] = QString(" (Magnitude)");

  // A vector – 3 components + 1 (magnitude)
  const int VECTOR_PLUS_MAGNITUDE = 4;
  this->tensorLookup[VECTOR_PLUS_MAGNITUDE] = tensorToSeries;
}

///////////////////////////////////////////////////////////////////////////////
pqPlotter::pqInternal::~pqInternal()
{
  delete this->richTextDocsPlaceholder;
}

//-----------------------------------------------------------------------------
QString pqPlotter::pqInternal::seriesComponentSuffixString(QString variableAsString)
{
  for (int i = 0; i < this->validSeriesComponentSuffixes.size(); i++)
  {
    if (variableAsString.endsWith(this->validSeriesComponentSuffixes[i]))
    {
      return this->validSeriesComponentSuffixes[i];
    }
  }

  return QString("");
}

///////////////////////////////////////////////////////////////////////////////
QString pqPlotter::pqInternal::stripSeriesComponent(QString variableAsString)
{
  QString suffixString = this->seriesComponentSuffixString(variableAsString);

  if (suffixString.size() > 0)
  {
    int positionToTruncate = variableAsString.size() - suffixString.size();
    if (positionToTruncate > 0)
    {
      variableAsString.truncate(positionToTruncate);
    }
  }

  return variableAsString;
}

//-----------------------------------------------------------------------------
QString pqPlotter::pqInternal::stripTensorComponent(QString variableAsString)
{
  QString retString = this->util.removeAllWhiteSpace(variableAsString);

  QString suffixString = this->tensorComponentSuffixString(retString);

  if (suffixString.size() > 0)
  {
    int positionToTruncate = retString.size() - suffixString.size();
    if (positionToTruncate > 0)
    {
      retString.truncate(positionToTruncate);
    }
  }

  return retString;
}

//-----------------------------------------------------------------------------
QString pqPlotter::pqInternal::tensorComponentSuffixString(QString variableAsString)
{
  for (int i = 0; i < this->validTensorComponentSuffixes.size(); i++)
  {
    if (variableAsString.endsWith(this->validTensorComponentSuffixes[i]))
    {
      return this->validTensorComponentSuffixes[i];
    }
  }

  return QString("");
}

//-----------------------------------------------------------------------------
QString pqPlotter::pqInternal::tensorOrVectorSuffixToSeriesSuffix(
  QString shortVarName, QString suffix, QMap<QString, int>& seriesCounter)
{
  QMap<QString, QString> tensorToSeries;
  int numComponents = seriesCounter[shortVarName];
  tensorToSeries = this->tensorLookup[numComponents];

  QString seriesString = tensorToSeries[suffix];

  return seriesString;
}

///////////////////////////////////////////////////////////////////////////////
pqPlotter::pqPlotter()
{
  this->Internal = new pqPlotter::pqInternal();
}

///////////////////////////////////////////////////////////////////////////////
pqPlotter::~pqPlotter()
{
  delete this->Internal;
}

///////////////////////////////////////////////////////////////////////////////
QStringList pqPlotter::getTheVars(vtkSMProxy* /*meshReaderProxy*/)
{
  QStringList theVars;
  theVars.clear();
  return theVars;
}

///////////////////////////////////////////////////////////////////////////////
void pqPlotter::setDomain(plotDomain /*theDomain*/)
{
}

///////////////////////////////////////////////////////////////////////////////
pqPlotter::plotDomain pqPlotter::getDomain()
{
  return this->domain;
}

///////////////////////////////////////////////////////////////////////////////
QStringList pqPlotter::getStringsFromProperty(vtkSMProperty* prop)
{
  QStringList theVars;
  theVars.clear();

  vtkSMStringVectorProperty* stringVecProp = dynamic_cast<vtkSMStringVectorProperty*>(prop);
  if (stringVecProp != NULL)
  {
    unsigned int uNumElems = stringVecProp->GetNumberOfElements();
    unsigned int i;
    for (i = 0; i < uNumElems; i += 2)
    {
      const char* elemPtr = stringVecProp->GetElement(i);
      theVars.append(QString(elemPtr));
    }
  }

  return theVars;
}

//-----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* pqPlotter::getDataSetAttributesInformation(
  vtkPVDataInformation* /*pvDataInfo*/)
{
  // should never get here, but the default behaviour is...
  return NULL;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqPlotter::getSMVariableProperty(vtkSMProxy* /*meshReaderProxy*/)
{
  // should never get here, but the default behaviour is...
  return NULL;
}

//-----------------------------------------------------------------------------
bool pqPlotter::amIAbleToSelectByNumber()
{
  // should never get here, but the default behaviour is...
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPlotter::getPlotFilter()
{
  // should never get here, but the default behaviour is...
  return NULL;
}

//-----------------------------------------------------------------------------
//
// NOTE: DUPLICATED METHOD (other in pqSierraPlotToolsManager)
//
pqPipelineSource* pqPlotter::findPipelineSource(const char* SMName)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(this->getActiveServer());
  foreach (pqPipelineSource* s, sources)
  {
    if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0)
      return s;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqPlotter::getSMNamedVariableProperty(vtkSMProxy* meshReaderProxy, QString propName)
{
  vtkSMProperty* prop = meshReaderProxy->GetProperty(qPrintable(propName));
  if (prop == NULL)
  {
    qWarning() << "pqPlotter::getSMNamedVariableProperty; Error: property is NULL for " << propName
               << " in mesh reader with VTKClassName: " << meshReaderProxy->GetVTKClassName()
               << " And GetXMLName: " << meshReaderProxy->GetXMLName();
  }

  return prop;
}

//-----------------------------------------------------------------------------
//
// NOTE: DUPLICATED METHOD (other in pqSierraPlotToolsManager)
//
pqServer* pqPlotter::getActiveServer()
{
  pqApplicationCore* app = pqApplicationCore::instance();
  pqServerManagerModel* smModel = app->getServerManagerModel();
  pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
void pqPlotter::setVarsStatus(vtkSMProxy* /*meshReaderProxy*/, bool /*flag*/)
{
  // should never get here, but the default behaviour is...
  return;
}

//-----------------------------------------------------------------------------
void pqPlotter::setVarElementsStatus(vtkSMProperty* prop, bool flag)
{
  if (prop != NULL)
  {
    // add variable names for all the global variables
    vtkSMStringVectorProperty* stringVecProp = dynamic_cast<vtkSMStringVectorProperty*>(prop);
    if (stringVecProp != NULL)
    {
      unsigned int uNumElems = stringVecProp->GetNumberOfElements();
      unsigned int i;
      for (i = 0; i < uNumElems; i += 2)
      {
#if 0
        const char * elemPtr = stringVecProp->GetElement(i);
        const char * elemPtr_status = stringVecProp->GetElement(i+1);
#endif

        if (flag)
        {
          stringVecProp->SetElement(i + 1, "1");
        }
        else
        {
          stringVecProp->SetElement(i + 1, "0");
        }
      }
    }
    else
    {
      // No global variables
      return;
    }
  }
  else
  {
    // error condition
    qWarning() << "pqPlotter::setVarElementsStatus: vtkSMProperty * prop IS NULL";
    return;
  }
}

//-----------------------------------------------------------------------------
void pqPlotter::setVarsActive(
  vtkSMProxy* /*meshReaderProxy*/, QString /*varName*/, bool /*activeFlag*/)
{
  // should never get here, but the default behaviour is...
  return;
}

//-----------------------------------------------------------------------------
void pqPlotter::setVarElementsActive(vtkSMProperty* prop, QString varName, bool activeFlag)
{
  if (prop != NULL)
  {
    // add variable names for all the global variables
    vtkSMStringVectorProperty* stringVecProp = dynamic_cast<vtkSMStringVectorProperty*>(prop);

    //#pragma message (__FILE__ "[" STRING(__LINE__) "]: pqPlotter::setVarElementsActive: CAUTION:
    // vtkSMStringVectorProperty * stringVecProp being used to directly update data, i.e. not using
    // pqSMAdaptor::setElementProperty()")

    if (stringVecProp != NULL)
    {
      unsigned int uNumElems = stringVecProp->GetNumberOfElements();
      unsigned int i;
      for (i = 0; i < uNumElems; i += 2)
      {
#if 0
        const char * elemPtr_status = stringVecProp->GetElement(i+1);
#endif

        const char* elemPtr = stringVecProp->GetElement(i);

        QString elemStr(elemPtr);

        if (elemStr.compare(varName) == 0)
        {
          if (activeFlag)
          {
            stringVecProp->SetElement(i + 1, "1");
          }
          else
          {
            stringVecProp->SetElement(i + 1, "0");
          }

          break; // GET OUTTA THIS LOOP
        }
        else
        {
          // stillLooking
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
QString pqPlotter::getFilterName()
{
  // should never get here, but the default behaviour is...
  return QString("");
}

//-----------------------------------------------------------------------------
QMap<QString, QList<pqOutputPort*> > pqPlotter::buildNamedInputs(
  pqPipelineSource* meshReader, QList<int> /*itemList*/, bool& success)
{
  success = true;

  QMap<QString, QList<pqOutputPort*> > namedInputs;
  QList<pqOutputPort*> inputs;
  inputs.push_back(meshReader->getOutputPort(0));
  namedInputs["Input"] = inputs;

  return namedInputs;
}

//-----------------------------------------------------------------------------
QString pqPlotter::getNumberItemsLabel()
{
  // should never get here, but the default behaviour is...
  return QString("");
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqPlotter::getArrayInformation(vtkPVDataSetAttributesInformation*)
{
  // should never get here, but the default behaviour is...
  return NULL;
}

//-----------------------------------------------------------------------------
bool pqPlotter::selectionWithinRange(QList<int> selectedItems, pqPipelineSource* meshReader)
{
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();

  vtkSMSourceProxy* sourceProxy = dynamic_cast<vtkSMSourceProxy*>(meshReaderProxy);

  if (sourceProxy != NULL)
  {
    vtkSMOutputPort* smOutputPort = sourceProxy->GetOutputPort((unsigned int)(0));
    vtkPVDataInformation* pvDataInfo = smOutputPort->GetDataInformation();
    if (pvDataInfo != NULL)
    {
      vtkPVDataSetAttributesInformation* pvDataSetAttributesInformation =
        this->getDataSetAttributesInformation(pvDataInfo);

      vtkPVArrayInformation* arrayInfo = this->getArrayInformation(pvDataSetAttributesInformation);

      if (arrayInfo != NULL)
      {
        int numComponents = arrayInfo->GetNumberOfComponents();

        if (numComponents > 1)
        {
          // some sort of crazy error!
          qWarning()
            << "pqPlotter::selectionWithinRange: ERROR - array returned more than one component!";
          return false;
        }

        double range[2];
        arrayInfo->GetComponentRange(0, range);
        int rangeLow = int(range[0]);
        int rangeHigh = int(range[1]);

        int i;
        long int max = -1;
        long int min = LONG_MAX;
        for (i = 0; i < selectedItems.size(); i++)
        {
          int val = selectedItems[i];

          if (val < min)
          {
            min = val;
          }

          if (val > max)
          {
            max = val;
          }
        }

        if (min >= rangeLow && max <= rangeHigh)
        {
          return true;
        }
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
pqView* pqPlotter::findView(pqPipelineSource* source, int port, const QString& viewType)
{
  // Step 1, try to find a view in which the source is already shown.
  if (source)
  {
    foreach (pqView* view, source->getViews())
    {
      pqDataRepresentation* repr = source->getRepresentation(port, view);
      if (repr && repr->isVisible())
        return view;
    }
  }

  // Step 2, check to see if the active view is the right type.
  pqView* view = pqActiveObjects::instance().activeView();

  if (view == NULL)
  {
    qWarning() << "You have the wrong view type... a new view type needs to be created";
    return NULL;
  }

  if (view->getViewType() == viewType)
    return view;

  // Step 3, check all the views and see if one is the right type and not
  // showing anything.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  foreach (view, smModel->findItems<pqView*>())
  {
    if (view && (view->getViewType() == viewType) &&
      (view->getNumberOfVisibleRepresentations() < 1))
    {
      return view;
    }
  }

  // Give up.  A new view needs to be created.
  return NULL;
}

//-----------------------------------------------------------------------------
pqView* pqPlotter::getPlotView(pqPipelineSource* plotFilter)
{
  return this->findView(plotFilter, 0, pqXYChartView::XYChartViewType());
}

//-----------------------------------------------------------------------------
pqView* pqPlotter::getMeshView(pqPipelineSource* meshReader)
{
  if (!meshReader)
  {
    return NULL;
  }

  return this->findView(meshReader, 0, pqRenderView::renderViewType());
}

//-----------------------------------------------------------------------------
void pqPlotter::setDisplayOfVariables(
  pqPipelineSource* meshReader, const QMap<QString, QString>& vars)
{
  if (!meshReader)
    return;

  // Get the plot source, view, and representation.
  pqPipelineSource* plotFilter = this->getPlotFilter();
  if (!plotFilter)
    return;

  pqView* plotView = this->getPlotView(plotFilter);
  if (!plotView)
    return;

  vtkSMProxy* viewProxy = plotView->getProxy();
  // viewProxy->UpdateVTKObjects();

  pqDataRepresentation* repr = plotFilter->getRepresentation(plotView);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  // Get the information property that lists all the available series (as well
  // as their current visibility).
  QList<QVariant> visibilityInfo =
    pqSMAdaptor::getMultipleElementProperty(reprProxy->GetProperty("SeriesVisibilityInfo"));

  QMap<QString, int> seriesCounter;

  // First find all and the number of components
  for (int i = 0; i < visibilityInfo.size(); i += 2)
  {
    QString varComponent = visibilityInfo[i].toString();
    QString simpleVar = this->Internal->stripSeriesComponent(varComponent);
    seriesCounter[simpleVar]++;
  }

  QList<QVariant> visibility;
  // Iterate over all the series, turn everything off first
  for (int i = 0; i < visibilityInfo.size(); i += 2)
  {
    QString varComponent = visibilityInfo[i].toString();
    visibility << varComponent << 0;
  }

  // now turn on the ones the user selected
  QList<QString> keys;
  keys = vars.keys();
  // QMap<QString,QString>::iterator iter = vars.begin();
  QList<QString>::iterator iter = keys.begin();
  while (iter != keys.end())
  {
    QString var = *iter;
    QString simpleVar = this->Internal->stripTensorComponent(var);
    QString seriesToShow = var;
    if (simpleVar != var)
    {
      QString suffix = this->Internal->tensorComponentSuffixString(var);
      // only get the mapping for tensors, or vectors
      QString seriesSuffix =
        this->Internal->tensorOrVectorSuffixToSeriesSuffix(simpleVar, suffix, seriesCounter);
      seriesToShow = simpleVar + seriesSuffix;
    }

    // now turn on this series
    visibility << seriesToShow << 1;
    iter++;
  }

  pqSMAdaptor::setMultipleElementProperty(reprProxy->GetProperty("SeriesVisibility"), visibility);
  reprProxy->UpdateVTKObjects();
  viewProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqPlotter::popUpHelp()
{
}

//-----------------------------------------------------------------------------
QString pqPlotter::getPlotterTextEditObjectName()
{
  // should never get here, derived classes should handle this,
  //    but the default behaviour is...
  return QString("");
}

//-----------------------------------------------------------------------------
QString pqPlotter::getPlotterHeadingHoverText()
{
  QString textEditObjName = this->getPlotterTextEditObjectName();
  QString richText("");

  QTextEdit* textEdit =
    this->Internal->richTextDocsPlaceholder->findChild<QTextEdit*>(textEditObjName);

  if (textEdit != NULL)
  {
    richText = textEdit->toHtml();
  }

  return richText;
}
