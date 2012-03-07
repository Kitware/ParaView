/*=========================================================================

   Program: ParaView
   Module:    pqPipelineRepresentation.cxx

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

/// \file pqPipelineRepresentation.cxx
/// \date 4/24/2006

#include "pqPipelineRepresentation.h"


// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkGeometryRepresentation.h"
#include "vtkMath.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
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
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqPipelineRepresentation::pqInternal
{
public:
  vtkSmartPointer<vtkSMRepresentationProxy> RepresentationProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
//  pqScalarOpacityFunction *Opacity;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
//    this->Opacity = 0;
    }

  static vtkPVArrayInformation* getArrayInformation(const pqPipelineRepresentation* repr,
    const char* arrayname, int fieldType)
    {
    if (!arrayname || !arrayname[0] || !repr)
      {
      return NULL;
      }

    vtkPVDataInformation* dataInformation = repr->getInputDataInformation();
    vtkPVArrayInformation* arrayInfo = NULL;
    if (dataInformation)
      {
      arrayInfo = dataInformation->GetAttributeInformation(fieldType)->
        GetArrayInformation(arrayname);
      }
    if (!arrayInfo)
      {
      dataInformation = repr->getRepresentedDataInformation();
      if (dataInformation)
        {
        arrayInfo = dataInformation->GetAttributeInformation(fieldType)->
          GetArrayInformation(arrayname);
        }
      }
    return arrayInfo;
    }

};

//-----------------------------------------------------------------------------
pqPipelineRepresentation::pqPipelineRepresentation(
  const QString& group,
  const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  Superclass(group, name, display, server, p)
{
  this->Internal = new pqPipelineRepresentation::pqInternal();
  this->Internal->RepresentationProxy
    = vtkSMRepresentationProxy::SafeDownCast(display);

  if (!this->Internal->RepresentationProxy)
    {
    qFatal("Display given is not a vtkSMRepresentationProxy.");
    }

  // If any of these properties change, we know that the coloring for the
  // representation has been affected.
  const char* properties[] = {
    "LookupTable",
    "ColorArrayName",
    "ColorAttributeType",
    0};

  for (int cc=0; properties[cc]; cc++)
    {
    this->Internal->VTKConnect->Connect(
      display->GetProperty(properties[cc]), vtkCommand::ModifiedEvent,
      this, SIGNAL(colorChanged()));
    }

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

  // Whenever the pipeline gets be updated, it's possible that the scalar ranges
  // change. If that happens, we try to ensure that the lookuptable range is big
  // enough to show the entire data (unless of course, the user locked the
  // lookuptable ranges).
  this->Internal->VTKConnect->Connect(
    display, vtkCommand::UpdateDataEvent,
    this, SLOT(onDataUpdated()));
  this->UpdateLUTRangesOnDataUpdate = true;
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation::~pqPipelineRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* pqPipelineRepresentation::getRepresentationProxy() const
{
  return this->Internal->RepresentationProxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineRepresentation::getScalarOpacityFunctionProxy()
{
  // We may want to create a new proxy is none exists.
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("ScalarOpacityFunction"));
}

//-----------------------------------------------------------------------------
pqScalarOpacityFunction* pqPipelineRepresentation::getScalarOpacityFunction()
{
  if (this->getRepresentationType().compare("Volume", Qt::CaseInsensitive) == 0)
    {
    pqServerManagerModel* smmodel = 
      pqApplicationCore::instance()->getServerManagerModel();
    vtkSMProxy* opf = this->getScalarOpacityFunctionProxy();

    return (opf? smmodel->findItem<pqScalarOpacityFunction*>(opf): 0);
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::createHelperProxies()
{
  vtkSMProxy* proxy = this->getProxy();

  if (proxy->GetProperty("ScalarOpacityFunction"))
    {
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    vtkSMProxy* opacityFunction = 
      pxm->NewProxy("piecewise_functions", "PiecewiseFunction");
    opacityFunction->UpdateVTKObjects();

    this->addHelperProxy("ScalarOpacityFunction", opacityFunction);
    opacityFunction->Delete();

    pqSMAdaptor::setProxyProperty(
      proxy->GetProperty("ScalarOpacityFunction"), opacityFunction);
    proxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onInputChanged()
{
  if (this->getInput())
    {
    QObject::disconnect(this->getInput(),
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(onInputAccepted()));
    }

  this->Superclass::onInputChanged();

  if (this->getInput())
    {
    /// We need to try to update the LUT ranges only when the user manually
    /// changed the input source object (not necessarily ever pipeline update
    /// which can happen when time changes -- for example). So we listen to this
    /// signal. BUG #10062.
    QObject::connect(this->getInput(),
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(onInputAccepted()));
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setDefaultPropertyValues()
{
  // We deliberately don;t call superclass. For somereason,
  // its messing up with the scalar coloring.
  this->Superclass::setDefaultPropertyValues();

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

  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  pqSMAdaptor::setEnumerationProperty(repr->GetProperty("SelectionRepresentation"),
    "Wireframe");
  pqSMAdaptor::setElementProperty(repr->GetProperty("SelectionLineWidth"), 2);
  pqSMAdaptor::setElementProperty(repr->GetProperty("SelectionPointSize"), 5);

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
  globalPropertiesManager->SetGlobalPropertyLink(
    "ForegroundColor", repr, "CubeAxesColor");

  // if the source created a new point scalar, use it
  // else if the source created a new cell scalar, use it
  // else if the input color by array exists in this source, use it
  // else color by property
  
  vtkPVDataInformation* inGeomInfo = 0;
  vtkPVDataInformation* geomInfo = 0;
  vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;

  // Get the time that this representation is going to use.
  vtkPVDataInformation* dataInfo = 0;

  dataInfo = this->getOutputPortFromInput()->getDataInformation();

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
        pqPipelineRepresentation::getUnstructuredGridOutlineThreshold()*1000000.0)
        {
        pqSMAdaptor::setEnumerationProperty(repr->GetProperty("Representation"),
          "Outline");
        }
      }
    else if (dataSetType == VTK_IMAGE_DATA)
      {
      // Use slice representation by default for 2D image data.
      int* ext = dataInfo->GetExtent();
      if (
        (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5]) &&
        /* slice not supported for composite datasets */
        (dataInfo->GetCompositeDataSetType()==-1 ))
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
  // Locate input display.
  pqPipelineRepresentation* upstreamDisplay =
    qobject_cast<pqPipelineRepresentation*>(
      this->getRepresentationForUpstreamSource());
  if (upstreamDisplay &&
    this->getRepresentationType().compare("Outline", Qt::CaseInsensitive) == 0)
    {
    // try to preserve the upstream representation type (except for volume
    // rendering).
    if (upstreamDisplay->getRepresentationType().compare("Volume",
        Qt::CaseInsensitive) != 0)
      {
      pqSMAdaptor::setElementProperty(repr->GetProperty("Representation"),
        upstreamDisplay->getRepresentationType());
      }
    }


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
  repr->UpdateVTKObjects();

  if (pqSMAdaptor::getEnumerationProperty(repr->GetProperty("Representation"))
    == "Outline")
    {
    // no need to determine scalar coloring for outline representation.
    // just ensure that the color is empty (BUG #12180).
    vtkSMPropertyHelper(repr, "ColorArrayName", true).Set((char*)NULL);
    return;
    }

  // update the input using the current application time.
  this->getInput()->updatePipeline();
  geomInfo = this->getInputDataInformation();
  if (upstreamDisplay)
    {
    inGeomInfo = upstreamDisplay->getInputDataInformation();
    }

  vtkPVArrayInformation* chosenArrayInfo = 0;
  int chosenFieldType = 0;

  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".
  if (geomInfo)
    {
    attrInfo = geomInfo->GetPointDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetPointDataInformation() : 0;
    pqPipelineRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if (arrayInfo)
      {
      chosenFieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
      chosenArrayInfo = arrayInfo;
      }
    }

  // Check for new cell scalars.
  if (!chosenArrayInfo && geomInfo)
    {
    attrInfo = geomInfo->GetCellDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetCellDataInformation() : 0;
    pqPipelineRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if (arrayInfo)
      {
      chosenFieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
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
      chosenFieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
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
      chosenFieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
      }
    }

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

  // Try to inherit the same array selected by the input.
  if (upstreamDisplay)
    {
    vtkSMPropertyHelper colorAttrType(upstreamDisplay->getProxy(),
      "ColorAttributeType");
    vtkSMPropertyHelper colorArrayName(upstreamDisplay->getProxy(),
      "ColorArrayName");
    if (pqInternal::getArrayInformation(
        this, colorArrayName.GetAsString(), colorAttrType.GetAsInt()))
      {
      this->colorByArray(
        colorArrayName.GetAsString(), colorAttrType.GetAsInt());
      return;
      }
    }

  QList<QString> myColorFields = this->getColorFields();

  // We are going to set the default color mode to use solid color i.e. not use
  // scalar coloring at all. However, for some representations (eg. slice/volume)
  // this is an error, we have to color by some array. Since no active scalar
  // were choosen, we simply use the first color array available. (If no arrays
  // are available, then error will be raised anyways).
  if (!myColorFields.contains(pqPipelineRepresentation::solidColor()))
    {
    if (myColorFields.size() > 0)
      {
      this->setColorField(myColorFields[0]);
      return;
      }
    }

  // Color by property.
  this->colorByArray(NULL, 0);

  pqSettings *settings = pqApplicationCore::instance()->settings();
  if(repr->GetProperty("AllowSpecularHighlightingWithScalarColoring"))
    {
    vtkSMPropertyHelper(repr, "AllowSpecularHighlightingWithScalarColoring").Set(
      settings->value("allowSpecularHighlightingWithScalarColoring").toBool());
    }
}

//-----------------------------------------------------------------------------
int pqPipelineRepresentation::getNumberOfComponents(
  const char* arrayname, int fieldtype)
{
  vtkPVArrayInformation* info = pqInternal::getArrayInformation(this,
    arrayname, fieldtype);
  return (info? info->GetNumberOfComponents() : 0);
}
//-----------------------------------------------------------------------------
QString pqPipelineRepresentation::getComponentName(
  const char* arrayname, int fieldtype, int component)
{
  vtkPVArrayInformation* info = pqInternal::getArrayInformation(this,
    arrayname, fieldtype);

   if ( info )
     {
     return QString (info->GetComponentName( component ) );     
     }  

   //failed to find info, return empty string
  return QString();
}
//-----------------------------------------------------------------------------
void pqPipelineRepresentation::colorByArray(const char* arrayname, int fieldtype)
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
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

  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;
  vtkSMProxy* opf = 0;
  if (lut_mgr)
    {
    int number_of_components = this->getNumberOfComponents(
      arrayname, fieldtype);
    pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
      this->getServer(), arrayname, number_of_components, 0);
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
      
    opf = this->createOpacityFunctionProxy(repr);
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
  if(opf)
    {
    pqSMAdaptor::setProxyProperty(
      repr->GetProperty("ScalarOpacityFunction"), opf);
    repr->UpdateVTKObjects();
    }

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

  if(fieldtype == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    pqSMAdaptor::setEnumerationProperty(
      repr->GetProperty("ColorAttributeType"), "CELL_DATA");
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(
      repr->GetProperty("ColorAttributeType"), "POINT_DATA");
    }
  pqSMAdaptor::setElementProperty(
    repr->GetProperty("ColorArrayName"), arrayname);
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
QString pqPipelineRepresentation::getRepresentationType() const
{
  vtkSMProxy* repr = this->getRepresentationProxy();
  if (repr && repr->GetProperty("Representation"))
    {
    // this handles enumeration domains as well.
    return vtkSMPropertyHelper(repr, "Representation").GetAsString(0);
    }

  const char* xmlname = repr->GetXMLName();
  if (strcmp(xmlname, "OutlineRepresentation") == 0)
    {
    return "Outline";
    }

  if (strcmp(xmlname, "UnstructuredGridVolumeRepresentation") == 0 ||
    strcmp(xmlname, "UniformGridVolumeRepresentation") == 0)
    {
    return "Volume";
    }

  if (strcmp(xmlname, "ImageSliceRepresentation") == 0)
    {
    return "Slice";
    }

  qCritical() << "pqPipelineRepresentation created for a incorrect proxy : " << xmlname;
  return 0;
}

//-----------------------------------------------------------------------------
double pqPipelineRepresentation::getOpacity() const
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("Opacity");
  return (prop? pqSMAdaptor::getElementProperty(prop).toDouble() : 1.0);
}
//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setColor(double R,double G,double B)
{
  pqSMAdaptor::setMultipleElementProperty(this->getProxy()->GetProperty("Color"),0,R);
  pqSMAdaptor::setMultipleElementProperty(this->getProxy()->GetProperty("Color"),1,G);
  pqSMAdaptor::setMultipleElementProperty(this->getProxy()->GetProperty("Color"),2,B);
  this->getProxy()->UpdateVTKObjects();

}
//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField != "" && 
    colorField != pqPipelineRepresentation::solidColor())
    {
    QPair<double,double> range = this->getColorFieldRange();
    lut->setScalarRange(range.first, range.second);

    // scalar opacity is treated as slave to the lookup table.
    pqScalarOpacityFunction* opacity = this->getScalarOpacityFunction();
    if(opacity)
      {
      opacity->setScalarRange(range.first, range.second);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRangeOverTime()
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField(true);

  if (lut && colorField != "" && 
    colorField != pqPipelineRepresentation::solidColor())
    {
    int attribute_type = vtkSMPropertyHelper(repr,
      "ColorAttributeType").GetAsInt();
    vtkPVTemporalDataInformation* dataInfo = 
      this->getInputTemporalDataInformation();
    vtkPVArrayInformation* arrayInfo = dataInfo->GetAttributeInformation(
      attribute_type)->GetArrayInformation(colorField.toAscii().data());
    if (arrayInfo)
      {
      int component = vtkSMPropertyHelper(lut->getProxy(),
        "VectorComponent").GetAsInt();
        if (vtkSMPropertyHelper(
            lut->getProxy(), "VectorMode").GetAsInt() ==
          vtkScalarsToColors::MAGNITUDE)
          {
          component = -1;
          }
      double range[2];
      arrayInfo->GetComponentRange(component, range);
      lut->setScalarRange(range[0], range[1]);

      // scalar opacity is treated as slave to the lookup table.
      pqScalarOpacityFunction* opacity = this->getScalarOpacityFunction();
      if (opacity)
        {
        opacity->setScalarRange(range[0], range[1]);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onInputAccepted()
{
  // BUG #10062
  // This slot gets called when the input to the representation is "accepted".
  // We mark this representation's LUT ranges dirty so that when the pipeline
  // finally updates, we can reset the LUT ranges.
  if (this->getInput()->modifiedState() == pqProxy::MODIFIED)
    {
    this->UpdateLUTRangesOnDataUpdate = true;
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onDataUpdated()
{
  if (this->UpdateLUTRangesOnDataUpdate ||
    pqScalarsToColors::colorRangeScalingMode() ==
    pqScalarsToColors::GROW_ON_UPDATED)
    {
    // BUG #10062
    // Since this part of the code happens every time the pipeline is updated, we
    // don't need to record it on the undo stack. It will happen automatically
    // each time.
    BEGIN_UNDO_EXCLUDE();
    this->UpdateLUTRangesOnDataUpdate = false;
    this->updateLookupTableScalarRange();
    END_UNDO_EXCLUDE();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::updateLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  if (!lut || lut->getScalarRangeLock())
    {
    return;
    }

  QString colorField = this->getColorField();
  if (colorField == "" || colorField == pqPipelineRepresentation::solidColor())
    {
    return;
    }

  QPair<double, double> range = this->getColorFieldRange();
  lut->setWholeScalarRange(range.first, range.second);

  // Adjust opacity function range.
  pqScalarOpacityFunction* opacityFunction = this->getScalarOpacityFunction();
  if (opacityFunction && !lut->getScalarRangeLock())
    {
    QPair<double, double> adjusted_range = lut->getScalarRange();

    // Opacity function always follows the LUT scalar range.
    // scalar opacity is treated as slave to the lookup table.
    opacityFunction->setScalarRange(adjusted_range.first, adjusted_range.second);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::getColorArray(
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
QList<QString> pqPipelineRepresentation::getColorFields()
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();

  QList<QString> ret;
  if(!repr)
    {
    return ret;
    }

  QString representation = this->getRepresentationType();

  if (representation.compare("Volume", Qt::CaseInsensitive) != 0 &&
    representation.compare("Slice", Qt::CaseInsensitive) != 0)
    {
    // Actor color is one way to color this part.
    // Not applicable when volume rendering.
    ret.append(pqPipelineRepresentation::solidColor());
    }

  vtkPVDataInformation* geomInfo = NULL;
  geomInfo = repr->GetRepresentedDataInformation();
  if(!geomInfo)
    {
    return ret;
    }

  // get cell arrays (only when not in certain data types).
  vtkPVDataSetAttributesInformation* cellinfo = 
    geomInfo->GetCellDataInformation();
  if (cellinfo)// && representation != vtkSMPVRepresentationProxy::VOLUME)
    {
    int dataSetType = -1;
    int compositeDataType = -1;
    vtkPVDataInformation* dataInfo = NULL;
    if(this->getInput())
      {
      dataInfo = this->getOutputPortFromInput()->getDataInformation();
      }
    if(dataInfo)
      {
      dataSetType = dataInfo->GetDataSetType();// get data set type
      compositeDataType = dataInfo->GetCompositeDataSetType();
      }

    if ((compositeDataType == VTK_OVERLAPPING_AMR) ||
        (compositeDataType == VTK_HIERARCHICAL_BOX_DATA_SET) ||
        (representation.compare("Volume", Qt::CaseInsensitive) != 0) ||
        (dataSetType != VTK_UNIFORM_GRID &&
         dataSetType != VTK_STRUCTURED_POINTS &&
         dataSetType != VTK_IMAGE_DATA ))
      {
      for(int i=0; i<cellinfo->GetNumberOfArrays(); i++)
        {
        vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
        if (representation.compare("Volume", Qt::CaseInsensitive) == 0 &&
            !( info->GetNumberOfComponents() == 1 ||
              info->GetNumberOfComponents() == 4))
          {
          // Skip vectors when volumerendering.
          continue;
          }

        QString name = info->GetName();
        name += " (cell)";
        ret.append(name);
        }
      }
    }
  
  // get point arrays (only when not in outline mode.
  vtkPVDataSetAttributesInformation* pointinfo = 
     geomInfo->GetPointDataInformation();
  if (pointinfo && representation.compare("Outline", Qt::CaseInsensitive) != 0)
    {
    for(int i=0; i<pointinfo->GetNumberOfArrays(); i++)
      {
      vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
      if (representation.compare("Volume", Qt::CaseInsensitive) == 0 &&
          !( info->GetNumberOfComponents() == 1 ||
            info->GetNumberOfComponents() == 4))
        {
        // Skip vectors when volumerendering.
        continue;
        }
      QString name = info->GetName();
      name += " (point)";
      ret.append(name);
      }
    }
  return ret;
}

//-----------------------------------------------------------------------------
int pqPipelineRepresentation::getColorFieldNumberOfComponents(const QString& array)
{
  QString field = array;
  int fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  if(field == pqPipelineRepresentation::solidColor())
    {
    return 0;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  return this->getNumberOfComponents(field.toAscii().data(),
    fieldType);
}

//-----------------------------------------------------------------------------
QString pqPipelineRepresentation::getColorFieldComponentName( const QString& array, const int &component )
{
  QString field = array;
  int fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  if(field == pqPipelineRepresentation::solidColor())
    {
    return 0;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

   return this->getComponentName(field.toAscii().data(), fieldType, component );
}

//-----------------------------------------------------------------------------
bool pqPipelineRepresentation::isPartial(const QString& array, int fieldType) const
{
  vtkPVArrayInformation* info = pqInternal::getArrayInformation(this,
    array.toAscii().data(), fieldType);
  return (info? (info->GetIsPartial()==1) : false);
}

//-----------------------------------------------------------------------------
QPair<double, double> 
pqPipelineRepresentation::getColorFieldRange(const QString& array, int component)
{
  QPair<double,double>ret(0.0, 1.0);

  QString field = array;
  int fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  if(field == pqPipelineRepresentation::solidColor())
    {
    return ret;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  vtkPVArrayInformation* arrayInfo = pqInternal::getArrayInformation(this,
    field.toAscii().data(), fieldType);
  if (arrayInfo)
    {
    if (component < arrayInfo->GetNumberOfComponents())
      {
      double range[2];
      arrayInfo->GetComponentRange(component, range);
      return QPair<double,double>(range[0], range[1]);
      }
    }
  return ret;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqPipelineRepresentation::getColorFieldRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField!= "" && 
    colorField != pqPipelineRepresentation::solidColor())
    {
    int component = pqSMAdaptor::getElementProperty(
      lut->getProxy()->GetProperty("VectorComponent")).toInt();
    if (pqSMAdaptor::getEnumerationProperty(
        lut->getProxy()->GetProperty("VectorMode")) == "Magnitude")
      {
      component = -1;
      }

    return this->getColorFieldRange(colorField, component);
    }

  return QPair<double, double>(0.0, 1.0);
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setColorField(const QString& value)
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();

  if(!repr)
    {
    return;
    }

  QString field = value;

  if(field == pqPipelineRepresentation::solidColor())
    {
    this->colorByArray(0, 0);
    }
  else
    {
    if(field.right(strlen(" (cell)")) == " (cell)")
      {
      field.chop(strlen(" (cell)"));
      this->colorByArray(field.toAscii().data(), 
                         vtkDataObject::FIELD_ASSOCIATION_CELLS);
      }
    else if(field.right(strlen(" (point)")) == " (point)")
      {
      field.chop(strlen(" (point)"));
      this->colorByArray(field.toAscii().data(), 
                         vtkDataObject::FIELD_ASSOCIATION_POINTS);
      }
    }
}


//-----------------------------------------------------------------------------
QString pqPipelineRepresentation::getColorField(bool raw)
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return pqPipelineRepresentation::solidColor();
    }

  QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
    repr->GetProperty("ColorAttributeType"));
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
    }

  return pqPipelineRepresentation::solidColor();
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setRepresentation(const QString& representation)
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
  vtkSMPropertyHelper(repr, "Representation").Set(representation.toAscii().data());
  repr->UpdateVTKObjects();
  this->onRepresentationChanged();
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onRepresentationChanged()
{
  vtkSMRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  QString reprType = this->getRepresentationType();
  if (reprType.compare("Volume", Qt::CaseInsensitive) != 0 &&
    reprType.compare("Slice", Qt::CaseInsensitive) != 0)
    {
    // Nothing to do here.
    return;
    }

  // Representation is Volume, is color array set?
  QList<QString> colorFields = this->getColorFields();
  if (colorFields.size() == 0)
    {
    qCritical() << 
      "Cannot volume render since no point (or cell) data available.";
    this->setRepresentation("Outline");
    return;
    }

  QString colorField = this->getColorField(false);
  if(!colorFields.contains(colorField))
    {
    // Current color field is not suitable for Volume rendering.
    // Change it.
    this->setColorField(colorFields[0]);
    }

  this->updateLookupTableScalarRange();
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::updateScalarBarVisibility(bool visible)
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
const char* pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()
{
  return "/representation/UnstructuredGridOutlineThreshold";
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setUnstructuredGridOutlineThreshold(double numcells)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
    {
    settings->setValue(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD(), 
      QVariant(numcells));
    }
}

//-----------------------------------------------------------------------------
double pqPipelineRepresentation::getUnstructuredGridOutlineThreshold()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings && settings->contains(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()))
    {
    bool ok;
    double numcells = settings->value(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()).toDouble(&ok);
    if (ok)
      {
      return numcells;
      }
    }

  return 0.5; //  1/2 million cells.
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineRepresentation::createOpacityFunctionProxy(
  vtkSMRepresentationProxy* repr)
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
    // Setup default opacity function to go from (0.0,0.0) to (1.0,1.0).
    // We are new setting defaults for midPoint (0.5) and sharpness(0.0) 
    QList<QVariant> values;
    values << 0.0 << 0.0 << 0.5 << 0.0 ;
    values << 1.0 << 1.0 << 0.5 << 0.0 ;
    
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
