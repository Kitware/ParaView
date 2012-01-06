/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotRepresentation.cxx

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

/// \file pqScatterPlotRepresentation.cxx
/// \date 4/24/2006

#include "pqScatterPlotRepresentation.h"


// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSmartPointer.h" 
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"
#include "vtkSMScatterPlotRepresentationProxy.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarOpacityFunction.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqDisplayPolicy.h"

//-----------------------------------------------------------------------------
class pqScatterPlotRepresentation::pqInternal
{
public:
  vtkSmartPointer<vtkSMScatterPlotRepresentationProxy> RepresentationProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }

  // Convenience method to get array information.
  vtkPVArrayInformation* getArrayInformation(
    const char* arrayname, int arrayType, vtkPVDataInformation* argInfo=0)
    {
    if (!arrayname || !arrayname[0] || !this->RepresentationProxy)
      {
      return 0; 
      }
    vtkSMScatterPlotRepresentationProxy* repr = this->RepresentationProxy;
    vtkPVDataInformation* dataInfo = 
      argInfo ? argInfo: repr->GetRepresentedDataInformation();
    if(!dataInfo)
      {
      return 0;
      }

    vtkPVArrayInformation* info = NULL;
    if(arrayType == pqScatterPlotRepresentation::COORD_DATA)
      {
      info = dataInfo->GetPointArrayInformation();
      }
    else if(arrayType == pqScatterPlotRepresentation::CELL_DATA)
      {
      vtkPVDataSetAttributesInformation* cellinfo = 
        dataInfo->GetCellDataInformation();
      info = cellinfo->GetArrayInformation(arrayname);
      }
    else if(arrayType == pqScatterPlotRepresentation::POINT_DATA) 
      {
      vtkPVDataSetAttributesInformation* pointinfo = 
        dataInfo->GetPointDataInformation();
      info = pointinfo->GetArrayInformation(arrayname);
      }
    else if(arrayType == pqScatterPlotRepresentation::FIELD_DATA) 
      {
      vtkPVDataSetAttributesInformation* fieldinfo = 
        dataInfo->GetFieldDataInformation();
      info = fieldinfo->GetArrayInformation(arrayname);
      }
    return info;
    }
};

//-----------------------------------------------------------------------------
pqScatterPlotRepresentation::pqScatterPlotRepresentation(
  const QString& group,
  const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  Superclass(group, name, display, server, p)
{
  this->Internal = new pqScatterPlotRepresentation::pqInternal();
  this->Internal->RepresentationProxy
    = vtkSMScatterPlotRepresentationProxy::SafeDownCast(display);

  if (!this->Internal->RepresentationProxy)
    {
    qFatal("Display given is not a vtkSMScatterPlotRepresentationProxy.");
    }

  // If any of these properties change, we know that the coloring for the
  // representation has been affected.
  const char* properties[] = {
    "LookupTable",
    "ColorArrayName",
    0};

  for (int cc=0; properties[cc]; cc++)
    {
    this->Internal->VTKConnect->Connect(
      display->GetProperty(properties[cc]), vtkCommand::ModifiedEvent,
      this, SIGNAL(colorChanged()));
    }

  this->Internal->VTKConnect->Connect(
    display->GetProperty("ColorArrayName"), vtkCommand::ModifiedEvent,
    this, SLOT(onColorArrayNameChanged()), 0, 0, Qt::QueuedConnection);

  QObject::connect(this, SIGNAL(visibilityChanged(bool)),
    this, SLOT(updateScalarBarVisibility(bool)));
}

//-----------------------------------------------------------------------------
pqScatterPlotRepresentation::~pqScatterPlotRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMScatterPlotRepresentationProxy* pqScatterPlotRepresentation::getRepresentationProxy() const
{
  return this->Internal->RepresentationProxy;
}

//-----------------------------------------------------------------------------
/*
vtkSMProxy* pqScatterPlotRepresentation::getScalarOpacityFunctionProxy()
{
  // Something for the volume rendering
  // We may want to create a new proxy is none exists.
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("ScalarOpacityFunction"));
  
  return 0;
}
*/
//-----------------------------------------------------------------------------
/*
pqScalarOpacityFunction* pqScatterPlotRepresentation::getScalarOpacityFunction()
{

  if(this->getRepresentationType() == vtkSMPVRepresentationProxy::VOLUME)
    {
    pqServerManagerModel* smmodel = 
      pqApplicationCore::instance()->getServerManagerModel();
    vtkSMProxy* opf = this->getScalarOpacityFunctionProxy();

    return (opf? smmodel->findItem<pqScalarOpacityFunction*>(opf): 0);
    }
  return 0;
}
*/
//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::setDefaultPropertyValues()
{
  // We deliberately don;t call superclass. For somereason,
  // its messing up with the scalar coloring.
  // this->Superclass::setDefaultPropertyValues();

  if (!this->isVisible() &&
      !pqApplicationCore::instance()->getDisplayPolicy()->getHideByDefault()
      )
    {
    // don't worry about invisible displays.
    return;
    }

  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  pqSMAdaptor::setEnumerationProperty(repr->GetProperty("SelectionRepresentation"),
    "Points");
  pqSMAdaptor::setElementProperty(repr->GetProperty("SelectionPointSize"), 2);

  // Set up some global property links by default.
  vtkSMGlobalPropertiesManager* globalPropertiesManager =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  // Note that the representation created for the 2D view doesn't even have
  // these properties.
  globalPropertiesManager->SetGlobalPropertyLink(
    "SelectionColor", repr, "SelectionColor");
  globalPropertiesManager->SetGlobalPropertyLink(
    "SurfaceColor", repr, "DiffuseColor");
  globalPropertiesManager->SetGlobalPropertyLink(
    "ForegroundColor", repr, "AmbientColor");
  globalPropertiesManager->SetGlobalPropertyLink(
    "EdgeColor", repr, "EdgeColor");
  globalPropertiesManager->SetGlobalPropertyLink(
    "SurfaceColor", repr, "BackfaceDiffuseColor");
  
  // if the source created a new point scalar, use it
  // else if the source created a new cell scalar, use it
  // else if the input color by array exists in this source, use it
  // else color by property
  
  vtkPVDataInformation* inGeomInfo = 0;
  vtkPVDataInformation* geomInfo = 0;
  // Get the time that this representation is going to use.
  vtkPVDataInformation* dataInfo = 0;

  dataInfo = this->getOutputPortFromInput()->getDataInformation();

/*
  if (repr->GetProperty("ScalarOpacityUnitDistance"))
    {
    double bounds[6];
    dataInfo->GetBounds(bounds);
    double unitDistance = 1.0;
    if(vtkMath::AreBoundsInitialized(bounds))
      {
      double diameter =
        sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
          (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
          (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );

      int numCells = dataInfo->GetNumberOfCells();
      double linearNumCells = pow( (double) numCells, (1.0/3.0) );
      unitDistance = diameter;
      if (linearNumCells != 0.0)
        {
        unitDistance = diameter / linearNumCells;
        }
      }
    pqSMAdaptor::setElementProperty(
      repr->GetProperty("ScalarOpacityUnitDistance"),
      unitDistance);
    }
*/
  repr->UpdateVTKObjects();

  geomInfo = repr->GetRepresentedDataInformation(/*update=*/true);

  // The pipeline has been updated, make sure the properties are up to date now
  repr->UpdatePropertyInformation();

  // Locate input display.
  pqScatterPlotRepresentation* upstreamDisplay = 
    qobject_cast<pqScatterPlotRepresentation*>(
      this->getRepresentationForUpstreamSource());
  if (upstreamDisplay)
    {
    inGeomInfo = upstreamDisplay->getRepresentationProxy()->
      GetRepresentedDataInformation();
    }
  // Color by property.
  QString array =  pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();
  this->colorByArray(array.toStdString().c_str());
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::colorByArray(const char* array)
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  if(!array || !array[0])
    {
    pqSMAdaptor::setElementProperty(
      repr->GetProperty("ColorArrayName"), "");
    repr->UpdateVTKObjects();

    // BUG #6818. If user switched to solid color, we need to update the lut
    // visibility.
    pqScalarsToColors* lut = this->getLookupTable();
    if (lut)
      {
      lut->hideUnusedScalarBars();
      }
    return;
    }
  
  //int arrayType = this->GetArrayType(array);
  int number_of_components = 
    //this->getNumberOfComponents(array, fieldtype);
    this->GetArrayNumberOfComponents(array);  
  int component = this->GetArrayComponent(array);;

  std::string arrayName = this->GetArrayName(array).toStdString();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;
  vtkSMProxy* opf = 0;
  if (lut_mgr)
    {
    pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
      this->getServer(), arrayName.c_str(), number_of_components, component);
    lut = (pqlut)? pqlut->getProxy() : 0;
    pqScalarOpacityFunction* pqOPF = lut_mgr->getScalarOpacityFunction(
      this->getServer(), arrayName.c_str(), number_of_components, 0);
    opf = (pqOPF)? pqOPF->getProxy() : 0;
    }
  else
    {
    // When lookup table manager is not available,
    // we simply create new lookup tables for each display.
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      repr->GetProperty("LookupTable"));
    if (pp->GetNumberOfProxies() == 0)
      {
      pqObjectBuilder* builder = core->getObjectBuilder();
      lut = builder->createProxy("lookup_tables", "PVLookupTable", 
        this->getServer(), "lookup_tables");
      // Setup default LUT to go from Blue to Red.
      QList<QVariant> values;
      values << 0.0 << 0.0 << 0.0 << 1.0
        << 1.0 << 1.0 << 0.0 << 0.0;
      pqSMAdaptor::setMultipleElementProperty(
        lut->GetProperty("RGBPoints"), values);
      pqSMAdaptor::setEnumerationProperty(
        lut->GetProperty("ColorSpace"), "HSV");
      pqSMAdaptor::setEnumerationProperty(
        lut->GetProperty("VectorMode"), "Magnitude");
      lut->UpdateVTKObjects();
      }
    else
      {
      lut = pp->GetProxy(0);
      }
      
    //opf = this->createOpacityFunctionProxy(repr);
    }

  if (!lut)
    {
    qDebug() << "Failed to create/locate Lookup Table.";
    pqSMAdaptor::setElementProperty(
      repr->GetProperty("ColorArrayName"), "");
    repr->UpdateVTKObjects();
    return;
    }

  // Locate pqScalarsToColors for the old LUT and update 
  // it's scalar bar visibility.
  pqScalarsToColors* old_stc = this->getLookupTable();
  pqSMAdaptor::setProxyProperty(
    repr->GetProperty("LookupTable"), lut);
    
  // set the opacity function
  /*
  if(opf)
    {
    pqSMAdaptor::setProxyProperty(
      repr->GetProperty("ScalarOpacityFunction"), opf);
    repr->UpdateVTKObjects();
    }
  */
  bool current_scalar_bar_visibility = false;
  // If old LUT was present update the visibility of the scalar bars
  if (old_stc && old_stc->getProxy() != lut)
      {
      pqScalarBarRepresentation* scalar_bar = old_stc->getScalarBar(
        qobject_cast<pqRenderViewBase*>(this->getView()));
      if (scalar_bar)
        {
        current_scalar_bar_visibility = scalar_bar->isVisible();
        }
      old_stc->hideUnusedScalarBars();
      }
  lut->UpdateVTKObjects();
  repr->UpdateVTKObjects();

  this->updateLookupTableScalarRange();

  
  if (current_scalar_bar_visibility && lut_mgr && this->getLookupTable())
    {
    lut_mgr->setScalarBarVisibility(this,
      current_scalar_bar_visibility);
    }
}

//-----------------------------------------------------------------------------
/*
double pqScatterPlotRepresentation::getOpacity() const
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("Opacity");
  return (prop? pqSMAdaptor::getElementProperty(prop).toDouble() : 1.0);
}
*/
//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::resetLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField != "")
    {
    QPair<double,double> range = this->getColorFieldRange();
    lut->setScalarRange(range.first, range.second);
/*
    // scalar opacity is treated as slave to the lookup table.
    pqScalarOpacityFunction* opacity = this->getScalarOpacityFunction();
    if(opacity)
      {
      opacity->setScalarRange(range.first, range.second);
      }
*/
    }
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::updateLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  if (!lut || lut->getScalarRangeLock())
    {
    return;
    }

  QString colorField = this->getColorField();
  if (colorField == "")
    {
    return;
    }

  QPair<double, double> range = this->getColorFieldRange();
  lut->setWholeScalarRange(range.first, range.second);
/*
  // Adjust opacity function range.
  pqScalarOpacityFunction* opacityFunction = this->getScalarOpacityFunction();
  if (opacityFunction && !lut->getScalarRangeLock())
    {
    QPair<double, double> adjusted_range = lut->getScalarRange();

    // Opacity function always follows the LUT scalar range.
    // scalar opacity is treated as slave to the lookup table.
    opacityFunction->setScalarRange(adjusted_range.first, adjusted_range.second);
    }
*/
}

//-----------------------------------------------------------------------------
bool pqScatterPlotRepresentation::isPartial(const QString& array) const
{
  QString arrayName = this->GetArrayName(array);
  int fieldType = this->GetArrayType(array);
  // the coord_data can't be partial...
  if( fieldType == pqScatterPlotRepresentation::COORD_DATA)
    {
    return false;
    }
  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    arrayName.toAscii().data(), fieldType, this->getInputDataInformation());
  return (info? (info->GetIsPartial()==1) : false);
}

//-----------------------------------------------------------------------------
QPair<double, double> 
pqScatterPlotRepresentation::getColorFieldRange(const QString& array)const
{
  QPair<double,double>ret(0.0, 1.0);

  int arrayType = this->GetArrayType(array);
  int component = this->GetArrayComponent(array);
  QString arrayName = this->GetArrayName(array);

  vtkPVArrayInformation* representedInfo = 
    this->Internal->getArrayInformation(arrayName.toAscii().data(), arrayType);

  vtkPVDataInformation* inputInformation = this->getInputDataInformation();
  vtkPVArrayInformation* inputInfo = this->Internal->getArrayInformation(
    arrayName.toAscii().data(), arrayType, inputInformation);

  // Try to use full input data range is possible. Sometimes, the data array is
  // only provided by some pre-processing filter added by the representation
  // (and is not present in the original input), in that case we use the range
  // provided by the representation.
  int inputInfoNumberOfComponents = 
    inputInfo ? inputInfo->GetNumberOfComponents() : 0;
  int representedInfoNumberOfComponents = 
    representedInfo ? representedInfo->GetNumberOfComponents() : 0;
  if (inputInfo && 
      component < inputInfoNumberOfComponents && 
      inputInfoNumberOfComponents > 0)
    {
    double range[2];
    inputInfo->GetComponentRange(component, range);
    ret = QPair<double,double>(range[0], range[1]);
    }
  else if (representedInfo &&
           component < representedInfoNumberOfComponents && 
           representedInfoNumberOfComponents > 0)
    {
    double range[2];
    representedInfo->GetComponentRange(component, range);
    ret = QPair<double,double>(range[0], range[1]);
    }
  return ret;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqScatterPlotRepresentation::getColorFieldRange()const
{
  //pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (colorField != "")
    {
    /*int component = -1;
    QRegExp rx("(.+)\\((\\d+)\\)( \\((cell|point)\\))?$");
    if(rx.exactMatch(colorField))
      {
      component = rx.cap(2).toInt();
      }
    else
      {
      component = pqSMAdaptor::getElementProperty(
        lut->getProxy()->GetProperty("VectorComponent")).toInt();
      if (pqSMAdaptor::getEnumerationProperty(
            lut->getProxy()->GetProperty("VectorMode")) == "Magnitude")
        {
        component = -1;
        }
      }
      */
    return this->getColorFieldRange(colorField);
    }

  return QPair<double, double>(0.0, 1.0);
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::setColorField(const QString& value)
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();

  if(!repr)
    {
    return;
    }
  if(this->GetArrayType(value) != -1)
    {
    this->colorByArray(value.toStdString().c_str());
    }
  else
    {
    this->colorByArray("");
    }
}


//-----------------------------------------------------------------------------
QString pqScatterPlotRepresentation::getColorField()const
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return "";
    }
  QString scalarArray = pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();
  return scalarArray;
}

//-----------------------------------------------------------------------------
bool pqScatterPlotRepresentation::getDataBounds(double bounds[6])const
{
  vtkSMScatterPlotRepresentationProxy* repr = 
    this->getRepresentationProxy();

  vtkPVDataInformation* info = repr? 
    repr->GetRepresentedDataInformation() : 0;
  if(!info)
    {
    return false;
    }
  info->GetBounds(bounds);
  return true;
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::onColorArrayNameChanged()
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }
  QString array = pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();
  this->colorByArray(array.toAscii().data()); 
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::updateScalarBarVisibility(bool visible)
{
  pqView* view = this->getView();
  if (!view)
    {
    return;
    }

  pqScalarsToColors* lut = this->getLookupTable();
  if (!lut)
    {
    return;
    }

  // Is this lut used by any other visible repr in this view?
  QList<pqRepresentation*> reprs = view->getRepresentations();
  foreach (pqRepresentation* repr, reprs)
    {
    pqDataRepresentation* dataRepr=qobject_cast<pqDataRepresentation*>(repr);
    if (dataRepr && dataRepr != this &&
      dataRepr->isVisible() && dataRepr->getLookupTable() == lut)
      {
      // lut is used by another visible repr. Don't change lut visibility.
      return;
      }
    }

  pqScalarBarRepresentation* sbRepr = lut->getScalarBar(
    qobject_cast<pqRenderView*>(view));
  if (sbRepr)
    {
    if (!visible && sbRepr->isVisible())
      {
      sbRepr->setVisible(false);
      sbRepr->setAutoHidden(true);
      }
    else if (visible && sbRepr->getAutoHidden() && !sbRepr->isVisible())
      {
      sbRepr->setAutoHidden(false);
      sbRepr->setVisible(true);
      }
    }
}

//-----------------------------------------------------------------------------
QString pqScatterPlotRepresentation::GetArrayName(const QString& array)const
{
  QStringList attributes = array.split(',');
  if (!attributes.count())
    {
    return QString();
    }
  if (attributes[0] == "coord" ||
      attributes[0] == "point" || 
      attributes[0] == "cell" ||
      attributes[0] == "field")
    {
    return attributes[1];
    }
  return attributes[0];
}


//-----------------------------------------------------------------------------
int pqScatterPlotRepresentation::GetArrayType(const QString& array)const
{
  QStringList attributes = array.split(',');
  if (!attributes.count())
    {
    return -1;
    }
  
  if (attributes[0] == "coord")
    {
    return pqScatterPlotRepresentation::COORD_DATA;
    }
  if (attributes[0] == "point")
    {
    return pqScatterPlotRepresentation::POINT_DATA;
    }
  if (attributes[0] == "cell")
    {
    return pqScatterPlotRepresentation::CELL_DATA;
    }
  if (attributes[0] == "field")
    {
    return pqScatterPlotRepresentation::FIELD_DATA;
    }
  return -1;
}

//-----------------------------------------------------------------------------
int pqScatterPlotRepresentation::GetArrayComponent(const QString& array)const
{
  QStringList attributes = array.split(',');
  QString arrayName = this->GetArrayName(array);
  int indexOfArrayName = attributes.indexOf(arrayName);
  if (indexOfArrayName == -1 || 
      indexOfArrayName + 1 >= attributes.count())
    {
    return -1;
    }
  bool ok = false;
  int component = attributes[indexOfArrayName + 1].toInt(&ok);
  if(!ok)
    {
    return -1;
    }
  return component;
}

int pqScatterPlotRepresentation::GetArrayNumberOfComponents(
  const QString& array) const
{
  QString arrayName = this->GetArrayName(array);
  int arrayType = this->GetArrayType(array);
  vtkPVArrayInformation* info = 
    this->Internal->getArrayInformation(arrayName.toAscii().data(), 
                                        arrayType);
  return (info? info->GetNumberOfComponents() : 0);
}


/*
//-----------------------------------------------------------------------------
vtkSMProxy* pqScatterPlotRepresentation::createOpacityFunctionProxy(
  vtkSMScatterPlotRepresentationProxy* repr)
{
  if (!repr || !repr->GetProperty("ScalarOpacityFunction"))
    {
    return NULL;
    }

  vtkSMProxy* opacityFunction = 0;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    repr->GetProperty("ScalarOpacityFunction"));
  if (pp->GetNumberOfProxies() == 0)
    {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    opacityFunction = builder->createProxy(
      "piecewise_functions", "PiecewiseFunction", 
      this->getServer(), "piecewise_functions");
    // Setup default opactiy function to go from 0 to 1.
    QList<QVariant> values;
    values << 0.0 << 0.0 << 1.0 << 1.0;
    
    pqSMAdaptor::setMultipleElementProperty(
      opacityFunction->GetProperty("Points"), values);
    opacityFunction->UpdateVTKObjects();
    }
  else
    {
    opacityFunction = pp->GetProxy(0);
    }

  return opacityFunction;
}
*/
