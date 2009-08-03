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
    const char* arrayname, int fieldType, vtkPVDataInformation* argInfo=0)
    {
    if (!arrayname || !arrayname[0] || !this->RepresentationProxy)
      {
      return 0; 
      }
    vtkSMScatterPlotRepresentationProxy* repr = this->RepresentationProxy;
    vtkPVDataInformation* dataInfo = argInfo? argInfo: repr->GetRepresentedDataInformation();
    if(!dataInfo)
      {
      return 0;
      }

    vtkPVArrayInformation* info = NULL;
    if(fieldType == vtkSMDataRepresentationProxy::CELL_DATA)
      {
      vtkPVDataSetAttributesInformation* cellinfo = 
        dataInfo->GetCellDataInformation();
      info = cellinfo->GetArrayInformation(arrayname);
      }
    else
      {
      vtkPVDataSetAttributesInformation* pointinfo = 
        dataInfo->GetPointDataInformation();
      info = pointinfo->GetArrayInformation(arrayname);
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
//    "ColorAttributeType",
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

  /*
  // Whenever representation changes to VolumeRendering, we have to
  // ensure that the ColorArray has been initialized to something.
  // Otherwise, the VolumeMapper segfaults.
  this->Internal->VTKConnect->Connect(
    display->GetProperty("Representation"), vtkCommand::ModifiedEvent,
    this, SLOT(onRepresentationChanged()), 0, 0, Qt::QueuedConnection);
    */

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
vtkSMProxy* pqScatterPlotRepresentation::getScalarOpacityFunctionProxy()
{
  /* Something for the volume rendering
  // We may want to create a new proxy is none exists.
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("ScalarOpacityFunction"));
  */
  return 0;
}

//-----------------------------------------------------------------------------
pqScalarOpacityFunction* pqScatterPlotRepresentation::getScalarOpacityFunction()
{
/*
  if(this->getRepresentationType() == vtkSMPVRepresentationProxy::VOLUME)
    {
    pqServerManagerModel* smmodel = 
      pqApplicationCore::instance()->getServerManagerModel();
    vtkSMProxy* opf = this->getScalarOpacityFunctionProxy();

    return (opf? smmodel->findItem<pqScalarOpacityFunction*>(opf): 0);
    }
*/
  return 0;
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::createHelperProxies()
{
  /*vtkSMProxy* proxy = this->getProxy();
  if (proxy->GetProperty("ScalarOpacityFunction"))
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* opacityFunction = 
      pxm->NewProxy("piecewise_functions", "PiecewiseFunction");
    opacityFunction->SetConnectionID(this->getServer()->GetConnectionID());
    opacityFunction->SetServers(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    opacityFunction->UpdateVTKObjects();

    this->addHelperProxy("ScalarOpacityFunction", opacityFunction);
    opacityFunction->Delete();

    pqSMAdaptor::setProxyProperty(
      proxy->GetProperty("ScalarOpacityFunction"), opacityFunction);
    proxy->UpdateVTKObjects();
    }
  */
}

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

  // The HelperProxy is not needed any more since now the OpacityFunction is 
  // created from LookupTableManager (Bug# 0008876)
  // this->createHelperProxies();

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
  //vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  //vtkPVDataSetAttributesInformation* attrInfo;
  //vtkPVArrayInformation* arrayInfo;

  // Get the time that this representation is going to use.
  vtkPVDataInformation* dataInfo = 0;

  dataInfo = this->getOutputPortFromInput()->getDataInformation(true);
/*
  // get data set type
  // and set the default representation
  if (dataInfo && repr->IsA("vtkSMPVRepresentationProxy"))
    {
    int dataSetType = dataInfo->GetDataSetType();
    if(dataSetType == VTK_POLY_DATA ||
       dataSetType == VTK_HYPER_OCTREE ||
       dataSetType == VTK_GENERIC_DATA_SET)
      {
      pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
        "Surface");
      }
    else if (dataSetType == VTK_UNSTRUCTURED_GRID)
      {
      if (static_cast<double>(dataInfo->GetNumberOfCells()) >= 
        pqScatterPlotRepresentation::getUnstructuredGridOutlineThreshold()*1000000.0)
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Outline");
        }
      }
    else if (dataSetType == VTK_IMAGE_DATA)
      {
      // Use slice representation by default for 2D image data.
      int* ext = dataInfo->GetExtent();
      if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Slice");
        }
      else
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Outline");
        }
      }
    else if(dataSetType == VTK_RECTILINEAR_GRID ||
       dataSetType == VTK_STRUCTURED_GRID)
      {
      int* ext = dataInfo->GetExtent();
      if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Surface");
        }
      else
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Outline");
        }
      }
    else
      {
      pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
        "Outline");
      }
    }
*/
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
/*
  vtkPVArrayInformation* chosenArrayInfo = 0;
  int chosenFieldType = 0;

  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".

  if (geomInfo)
    {
    attrInfo = geomInfo->GetPointDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetPointDataInformation() : 0;
    pqScatterPlotRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if (arrayInfo)
      {
      chosenFieldType = vtkSMDataRepresentationProxy::POINT_DATA;
      chosenArrayInfo = arrayInfo;
      }
    }
    
  // Check for new cell scalars.
  if (!chosenArrayInfo && geomInfo)
    {
    attrInfo = geomInfo->GetCellDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetCellDataInformation() : 0;
    pqScatterPlotRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if (arrayInfo)
      {
      chosenFieldType = vtkSMDataRepresentationProxy::CELL_DATA;
      chosenArrayInfo = arrayInfo;
      }
    }
   
  if (!chosenArrayInfo && geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetPointDataInformation();
    this->getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if (arrayInfo)
      {
      chosenArrayInfo = arrayInfo;
      chosenFieldType = vtkSMDataRepresentationProxy::POINT_DATA;
      }
    }

  if (!chosenArrayInfo && geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetCellDataInformation();
    this->getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      chosenArrayInfo = arrayInfo;
      chosenFieldType = vtkSMDataRepresentationProxy::CELL_DATA;
      }
    }
*/
/*
  if (chosenArrayInfo)
    {
    if (chosenArrayInfo->GetDataType() == VTK_UNSIGNED_CHAR &&
        chosenArrayInfo->GetNumberOfComponents() <= 4)
        {
        pqSMAdaptor::setElementProperty(repr->GetProperty("MapScalars"), 0);
        }
    this->colorByArray(chosenArrayInfo->GetName(), chosenFieldType);
    return;
    }
*/
/*
  QList<QString> myColorFields = this->getColorFields();

  // Try to inherit the same array selected by the input.
  if (upstreamDisplay)
    {
    const QString &upstreamColorField = upstreamDisplay->getColorField(false);
    if (myColorFields.contains(upstreamColorField))
      {
      this->setColorField(upstreamColorField);
      return;
      }
    }

  // We are going to set the default color mode to use solid color i.e. not use
  // scalar coloring at all. However, for some representations (eg. slice/volume)
  // this is an error, we have to color by some array. Since no active scalar
  // were choosen, we simply use the first color array available. (If no arrays
  // are available, then error will be raised anyways).
  if (!myColorFields.contains(pqScatterPlotRepresentation::solidColor()))
    {
    if (myColorFields.size() > 0)
      {
      this->setColorField(myColorFields[0]);
      return;
      }
    }
*/
  // Color by property.
  //this->colorByArray(NULL, 0);
  QString array =  pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();
  this->colorByArray(array.toStdString().c_str(),0);
}

//-----------------------------------------------------------------------------
int pqScatterPlotRepresentation::getNumberOfComponents(
  const char* arrayname, int fieldtype)
{
  QString array = arrayname;
  QRegExp rx("(.+)\\((\\d+)\\)$");
  if(rx.exactMatch(array))
    {
    array = rx.cap(1);
    //QString component = rx.cap(2);
    }

  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    array.toAscii().data(), fieldtype);
  return (info? info->GetNumberOfComponents() : 0);
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::colorByArray(const char* arrayname, int fieldtype)
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  if(!arrayname || !arrayname[0])
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

  int number_of_components = this->getNumberOfComponents(
    arrayname, fieldtype);

  vtkstd::string array(arrayname);
  int component = -1;
  QRegExp rx("(.+)\\((\\d+)\\)$");
  if(rx.exactMatch(array.c_str()))
    {
    array = rx.cap(1).toStdString();
    component = rx.cap(2).toInt();
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;
  vtkSMProxy* opf = 0;
  if (lut_mgr)
    {
    pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
      this->getServer(), arrayname, number_of_components, component);
    lut = (pqlut)? pqlut->getProxy() : 0;
    pqScalarOpacityFunction* pqOPF = lut_mgr->getScalarOpacityFunction(
      this->getServer(), arrayname, number_of_components, 0);
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
    //pqSMAdaptor::setElementProperty(
    //  repr->GetProperty("ColorArrayName"), "");
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
/*
  if(fieldtype == vtkSMDataRepresentationProxy::CELL_DATA)
    {
    pqSMAdaptor::setEnumerationProperty(
      repr->GetProperty("ColorAttributeType"), "CELL_DATA");
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(
      repr->GetProperty("ColorAttributeType"), "POINT_DATA");
    }
*/
  //pqSMAdaptor::setElementProperty(
  //  repr->GetProperty("ColorArrayName"), arrayname);
  lut->UpdateVTKObjects();
  repr->UpdateVTKObjects();

  this->updateLookupTableScalarRange();

  
  if (current_scalar_bar_visibility && lut_mgr && this->getLookupTable())
    {
    lut_mgr->setScalarBarVisibility(this->getView(),
      this->getLookupTable(),
      current_scalar_bar_visibility);
    }
}

//-----------------------------------------------------------------------------
int pqScatterPlotRepresentation::getRepresentationType() const
{
  vtkSMProxy* repr = this->getRepresentationProxy();
  vtkSMPVRepresentationProxy* pvRepr = 
    vtkSMPVRepresentationProxy::SafeDownCast(repr);
  if (pvRepr)
    {
    return pvRepr->GetRepresentation();
    }

  const char* xmlname = repr->GetXMLName();
  if (strcmp(xmlname, "ScatterPlotRepresentation") == 0)
    {
    return vtkSMPVRepresentationProxy::POINTS;
    }

  qCritical() << "pqScatterPlotRepresentation created for a incorrect proxy : " << xmlname;
  return 0;
}

//-----------------------------------------------------------------------------
double pqScatterPlotRepresentation::getOpacity() const
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("Opacity");
  return (prop? pqSMAdaptor::getElementProperty(prop).toDouble() : 1.0);
}

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
void pqScatterPlotRepresentation::getColorArray(
  vtkPVDataSetAttributesInformation* attrInfo,
  vtkPVDataSetAttributesInformation* inAttrInfo,
  vtkPVArrayInformation*& arrayInfo)
{  
  arrayInfo = NULL;

  // Check for new point scalars.
  vtkPVArrayInformation* tmp =
    attrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
  vtkPVArrayInformation* inArrayInfo = 0;
  if (tmp)
    {
    if (inAttrInfo)
      {
      inArrayInfo = inAttrInfo->GetAttributeInformation(
        vtkDataSetAttributes::SCALARS);
      }
    if (inArrayInfo == 0 ||
      strcmp(tmp->GetName(),inArrayInfo->GetName()) != 0)
      { 
      // No input or different scalars: use the new scalars.
      arrayInfo = tmp;
      }
    }
}

//-----------------------------------------------------------------------------
int pqScatterPlotRepresentation::getColorFieldNumberOfComponents(const QString& array)
{
  QString field = array;
  int fieldType = vtkSMDataRepresentationProxy::POINT_DATA;

  if(field == "")
    {
    return 0;
    }
  if(field.right(static_cast<int>(strlen(" (cell)"))) == " (cell)")
    {
    field.chop(static_cast<int>(strlen(" (cell)")));
    fieldType = vtkSMDataRepresentationProxy::CELL_DATA;
    }
  else if(field.right(static_cast<int>(strlen(" (point)"))) == " (point)")
    {
    field.chop(static_cast<int>(strlen(" (point)")));
    fieldType = vtkSMDataRepresentationProxy::POINT_DATA;
    }

  return this->getNumberOfComponents(field.toAscii().data(),
    fieldType);
}

//-----------------------------------------------------------------------------
bool pqScatterPlotRepresentation::isPartial(const QString& array, int fieldType) const
{
  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    array.toAscii().data(), fieldType, this->getInputDataInformation());
  return (info? (info->GetIsPartial()==1) : false);
}

//-----------------------------------------------------------------------------
QPair<double, double> 
pqScatterPlotRepresentation::getColorFieldRange(const QString& array, int component)
{
  QPair<double,double>ret(0.0, 1.0);

  QString field = array;
  int fieldType = vtkSMDataRepresentationProxy::POINT_DATA;

  if(field == "")
    {
    return ret;
    }
  if(field.right(static_cast<int>(strlen(" (cell)"))) == " (cell)")
    {
    field.chop(static_cast<int>(strlen(" (cell)")));
    fieldType = vtkSMDataRepresentationProxy::CELL_DATA;
    }
  else if(field.right(static_cast<int>(strlen(" (point)"))) == " (point)")
    {
    field.chop(static_cast<int>(strlen(" (point)")));
    fieldType = vtkSMDataRepresentationProxy::POINT_DATA;
    }

  QRegExp rx("(.+)\\((\\d+)\\)$");
  if(rx.exactMatch(field))
    {
    field = rx.cap(1);
    //QString component = rx.cap(2);
    }

  vtkPVArrayInformation* representedInfo = 
    this->Internal->getArrayInformation(field.toAscii().data(), fieldType);

  vtkPVDataInformation* inputInformation = this->getInputDataInformation();
  vtkPVArrayInformation* inputInfo = this->Internal->getArrayInformation(
    field.toAscii().data(), fieldType, inputInformation);

  // Try to use full input data range is possible. Sometimes, the data array is
  // only provided by some pre-processing filter added by the representation
  // (and is not present in the original input), in that case we use the range
  // provided by the representation.
  if (inputInfo)
    {
    if (component < inputInfo->GetNumberOfComponents())
      {
      double range[2];
      inputInfo->GetComponentRange(component, range);
      return QPair<double,double>(range[0], range[1]);
      }
    }

  if (representedInfo)
    {
    if (component <representedInfo->GetNumberOfComponents())
      {
      double range[2];
      representedInfo->GetComponentRange(component, range);
      return QPair<double,double>(range[0], range[1]);
      }
    }

  return ret;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqScatterPlotRepresentation::getColorFieldRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField != "")
    {
    int component = -1;
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

    return this->getColorFieldRange(colorField, component);
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

  QString field = value;
  if(field.right(static_cast<int>(strlen(" (cell)"))) == " (cell)")
    {
    field.chop(static_cast<int>(strlen(" (cell)")));
    this->colorByArray(field.toAscii().data(), 
                       vtkSMDataRepresentationProxy::CELL_DATA);
    }
  else if(field.right(static_cast<int>(strlen(" (point)"))) == " (point)")
    {
    field.chop(static_cast<int>(strlen(" (point)")));
    this->colorByArray(field.toAscii().data(), 
                       vtkSMDataRepresentationProxy::POINT_DATA);
    }
  else if(field != "")
    {
    this->colorByArray(field.toAscii().data(), 0);
    }
  else
    {
    this->colorByArray(0, 0);
    }
}


//-----------------------------------------------------------------------------
QString pqScatterPlotRepresentation::getColorField(bool raw)
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return "";
    }

  QVariant scalarMode = "";
  vtkSMProperty* colorAttributeProp = 
    repr->GetProperty("ColorAttributeType");
  if(colorAttributeProp)
    {
    scalarMode = pqSMAdaptor::getEnumerationProperty(colorAttributeProp);
    }
  QString scalarArray = pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();

  if (scalarArray != "")
    {
    if (raw)
      {
      return scalarArray;
      }

    if(scalarMode == "CELL_DATA")
      {
      return scalarArray + " (cell)";
      }
    else if(scalarMode == "POINT_DATA")
      {
      return scalarArray + " (point)";
      }
    else
      {//we don't know the type, not a big deal though.
      return scalarArray;
      }
    }

  return "";
}

//-----------------------------------------------------------------------------
bool pqScatterPlotRepresentation::getDataBounds(double bounds[6])
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
void pqScatterPlotRepresentation::setRepresentation(int representation)
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  pqSMAdaptor::setElementProperty(
    repr->GetProperty("Representation"), representation);
  repr->UpdateVTKObjects();
  this->onColorArrayNameChanged();
}

//-----------------------------------------------------------------------------
void pqScatterPlotRepresentation::onColorArrayNameChanged()
{
  vtkSMScatterPlotRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }
  QString array =  pqSMAdaptor::getElementProperty(
    repr->GetProperty("ColorArrayName")).toString();
  
  this->colorByArray(array.toAscii().data(), 
                     vtkSMDataRepresentationProxy::POINT_DATA);
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
