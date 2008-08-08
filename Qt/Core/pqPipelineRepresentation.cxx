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
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSmartPointer.h" 
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"

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
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqDisplayPolicy.h"

//-----------------------------------------------------------------------------
class pqPipelineRepresentation::pqInternal
{
public:
  vtkSmartPointer<vtkSMPropRepresentationProxy> RepresentationProxy;
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
    vtkSMDataRepresentationProxy* repr = this->RepresentationProxy;
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
pqPipelineRepresentation::pqPipelineRepresentation(
  const QString& group,
  const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  Superclass(group, name, display, server, p)
{
  this->Internal = new pqPipelineRepresentation::pqInternal();
  this->Internal->RepresentationProxy
    = vtkSMPropRepresentationProxy::SafeDownCast(display);

  if (!this->Internal->RepresentationProxy)
    {
    qFatal("Display given is not a vtkSMPropRepresentationProxy.");
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
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation::~pqPipelineRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMPropRepresentationProxy* pqPipelineRepresentation::getRepresentationProxy() const
{
  return this->Internal->RepresentationProxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineRepresentation::getScalarOpacityFunctionProxy() const
{
  // We may want to create a new proxy is none exists.
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("ScalarOpacityFunction"));
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::createHelperProxies()
{
  vtkSMProxy* proxy = this->getProxy();

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
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setDefaultPropertyValues()
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

  this->createHelperProxies();

  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  pqSMAdaptor::setEnumerationProperty(repr->GetProperty("SelectionRepresentation"),
    "Wireframe");
  pqSMAdaptor::setMultipleElementProperty(repr->GetProperty("SelectionColor"),
    0, 1.0);
  pqSMAdaptor::setMultipleElementProperty(repr->GetProperty("SelectionColor"),
    1, 0.0);
  pqSMAdaptor::setMultipleElementProperty(repr->GetProperty("SelectionColor"),
    2, 1.0);
  pqSMAdaptor::setElementProperty(repr->GetProperty("SelectionLineWidth"), 2);

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

  dataInfo = this->getOutputPortFromInput()->getDataInformation(true);

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

  geomInfo = repr->GetRepresentedDataInformation(/*update=*/true);

  // Locate input display.
  pqPipelineFilter* myInput = qobject_cast<pqPipelineFilter*>(this->getInput());
  pqPipelineRepresentation* upstreamDisplay = 0;
  if (myInput && myInput->getInputCount() > 0)
    {
    pqPipelineSource* myInputsInput = myInput->getInput(0);
    upstreamDisplay = qobject_cast<pqPipelineRepresentation*>(
      myInputsInput->getRepresentation(0, this->getView()));
    if (upstreamDisplay)
      {
      inGeomInfo = upstreamDisplay->getRepresentationProxy()->GetRepresentedDataInformation();
      }
    }

  // Propagate solid color from the upstream representation.
  // In case of slice representation (and similar) there may be no "Color"
  // property, hence we need to have this check.
  if (upstreamDisplay && repr->GetProperty("Color") && 
    upstreamDisplay->getProxy()->GetProperty("Color"))
    {
    repr->GetProperty("Color")->Copy(
      upstreamDisplay->getProxy()->GetProperty("Color"));
    }

  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".
  if(geomInfo)
    {
    attrInfo = geomInfo->GetPointDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetPointDataInformation() : 0;
    pqPipelineRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataRepresentationProxy::POINT_DATA);
      return;
      }
    }
    
  // Check for new cell scalars.
  if(geomInfo)
    {
    attrInfo = geomInfo->GetCellDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetCellDataInformation() : 0;
    pqPipelineRepresentation::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataRepresentationProxy::CELL_DATA);
      return;
      }
    }
   
  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetPointDataInformation();
    this->getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataRepresentationProxy::POINT_DATA);
      return;
      }
    }

  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetCellDataInformation();
    this->getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataRepresentationProxy::CELL_DATA);
      return;
      }
    }

  // Try to inherit the same array selected by the input.
  if (upstreamDisplay)
    {
    QList<QString> myColorFields = this->getColorFields();
    const QString &upstreamColorField = upstreamDisplay->getColorField(false);
    if (myColorFields.contains(upstreamColorField))
      {
      this->setColorField(upstreamColorField);
      return;
      }
    }

  // Color by property.
  this->colorByArray(NULL, 0);
}

//-----------------------------------------------------------------------------
int pqPipelineRepresentation::getNumberOfComponents(
  const char* arrayname, int fieldtype)
{
  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    arrayname, fieldtype);
  return (info? info->GetNumberOfComponents() : 0);
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::colorByArray(const char* arrayname, int fieldtype)
{
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  if(!arrayname || !arrayname[0])
    {
    pqSMAdaptor::setElementProperty(
      repr->GetProperty("ColorArrayName"), "");
    repr->UpdateVTKObjects();
    return;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;

  if (lut_mgr)
    {
    int number_of_components = this->getNumberOfComponents(
      arrayname, fieldtype);
    pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
      this->getServer(), arrayname, number_of_components, 0);
    lut = (pqlut)? pqlut->getProxy() : 0;
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
    }

  if (!lut)
    {
    qDebug() << "Failed to create/locate Lookup Table.";
    pqSMAdaptor::setElementProperty(
      repr->GetProperty("ColorArrayName"), "");
    repr->UpdateVTKObjects();
    return;
    }

  vtkSMProxy* oldlutProxy = 
    pqSMAdaptor::getProxyProperty(repr->GetProperty("LookupTable"));
  pqScalarsToColors* old_stc = 0;
  if (oldlutProxy != lut)
    {
    // Locate pqScalarsToColors for the old LUT and update 
    // it's scalar bar visibility.
    pqServerManagerModel* smmodel = core->getServerManagerModel();
    old_stc = smmodel->findItem<pqScalarsToColors*>(oldlutProxy);
    }
  
  pqSMAdaptor::setProxyProperty(
    repr->GetProperty("LookupTable"), lut);

  // If old LUT was present update the visibility of the scalar bars
  if (old_stc)
      {
      old_stc->hideUnusedScalarBars();
      }

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
  pqSMAdaptor::setElementProperty(
    repr->GetProperty("ColorArrayName"), arrayname);
  lut->UpdateVTKObjects();
  repr->UpdateVTKObjects();

  this->updateLookupTableScalarRange();
}

//-----------------------------------------------------------------------------
int pqPipelineRepresentation::getRepresentationType() const
{
  vtkSMProxy* repr = this->getRepresentationProxy();
  vtkSMPVRepresentationProxy* pvRepr = 
    vtkSMPVRepresentationProxy::SafeDownCast(repr);
  if (pvRepr)
    {
    return pvRepr->GetRepresentation();
    }

  const char* xmlname = repr->GetXMLName();
  if (strcmp(xmlname, "SurfaceRepresentation") == 0)
    {
    int reprType = 
      pqSMAdaptor::getElementProperty(repr->GetProperty("Representation")).toInt();
    switch (reprType)
      {
    case VTK_POINTS:
      return vtkSMPVRepresentationProxy::POINTS;

    case VTK_WIREFRAME:
      return vtkSMPVRepresentationProxy::WIREFRAME;

    case VTK_SURFACE_WITH_EDGES:
      return vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES;

    case VTK_SURFACE:
    default:
      return vtkSMPVRepresentationProxy::SURFACE;
      }
    }
  
  if (strcmp(xmlname, "OutlineRepresentation") == 0)
    {
    return vtkSMPVRepresentationProxy::OUTLINE;
    }

  if (strcmp(xmlname, "UnstructuredGridVolumeRepresentation") == 0 ||
    strcmp(xmlname, "UniformGridVolumeRepresentation") == 0)
    {
    return vtkSMPVRepresentationProxy::VOLUME;
    }

  if (strcmp(xmlname, "ImageSliceRepresentation") == 0)
    {
    return vtkSMPVRepresentationProxy::SLICE;
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
    this->setScalarOpacityRange(range.first, range.second);
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
  vtkSMProxy* opacityFunction = this->getScalarOpacityFunctionProxy();
  if (opacityFunction && !lut->getScalarRangeLock() &&
    this->getRepresentationType() == vtkSMPVRepresentationProxy::VOLUME)
    {
    QPair<double, double> adjusted_range = lut->getScalarRange();

    // Opacity function always follows the LUT scalar range.
    // scalar opacity is treated as slave to the lookup table.
    this->setScalarOpacityRange(adjusted_range.first, adjusted_range.second);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setScalarOpacityRange(double min, double max)
{
  // A far more better way would be to create a new pqProxy subclass for
  // the piecewise function.
  vtkSMProxy* opacityFunction = this->getScalarOpacityFunctionProxy();
  if (!opacityFunction)
    {
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    opacityFunction->GetProperty("Points"));

  QList<QVariant> controlPoints = pqSMAdaptor::getMultipleElementProperty(dvp);
  if (controlPoints.size() == 0)
    {
    return;
    }

  int max_index = dvp->GetNumberOfElementsPerCommand() * (
    (controlPoints.size()-1)/ dvp->GetNumberOfElementsPerCommand());
  QPair<double, double> current_range(controlPoints[0].toDouble(),
    controlPoints[max_index].toDouble());

  // Adjust vtkPiecewiseFunction points to the new range.
  double dold = (current_range.second - current_range.first);
  dold = (dold > 0) ? dold : 1;

  double dnew = (max -min);

  if (dnew > 0)
    {
    double scale = dnew/dold;
    for (int cc=0; cc < controlPoints.size(); 
         cc+= dvp->GetNumberOfElementsPerCommand())
      {
      controlPoints[cc] = 
        scale * (controlPoints[cc].toDouble()-current_range.first) + min;
      }
    }
  else
    {
    // allowing an opacity transfer function with a scalar range of 0.
    // In this case, the piecewise function only contains the endpoints.
    controlPoints.clear();
    controlPoints.push_back(min);
    controlPoints.push_back(0);
    controlPoints.push_back(max);
    controlPoints.push_back(1);
    }

  pqSMAdaptor::setMultipleElementProperty(dvp, controlPoints);
  opacityFunction->UpdateVTKObjects();
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
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();

  QList<QString> ret;
  if(!repr)
    {
    return ret;
    }

  int representation = this->getRepresentationType();

  if (representation != vtkSMPVRepresentationProxy::VOLUME && 
    representation != vtkSMPVRepresentationProxy::SLICE)
    {
    // Actor color is one way to color this part.
    // Not applicable when volume rendering.
    ret.append(pqPipelineRepresentation::solidColor());
    }

  vtkPVDataInformation* geomInfo = repr->GetRepresentedDataInformation();
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
    vtkPVDataInformation* dataInfo = NULL;
    if(this->getInput())
      {
      dataInfo = this->getOutputPortFromInput()->getDataInformation(false);
      }
    if(dataInfo)
      {
      dataSetType = dataInfo->GetDataSetType();// get data set type
      }

    if(representation != vtkSMPVRepresentationProxy::VOLUME || (
       representation == vtkSMPVRepresentationProxy::VOLUME &&
       dataSetType != VTK_UNIFORM_GRID && 
       dataSetType != VTK_STRUCTURED_POINTS &&
       dataSetType != VTK_IMAGE_DATA ))
      {
      for(int i=0; i<cellinfo->GetNumberOfArrays(); i++)
        {
        vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
        QString name = info->GetName();
        name += " (cell)";
        ret.append(name);
        }
      }
    }
  
  // get point arrays (only when not in outline mode.
  vtkPVDataSetAttributesInformation* pointinfo = 
     geomInfo->GetPointDataInformation();
  if (pointinfo && representation != vtkSMPVRepresentationProxy::OUTLINE)
    {
    for(int i=0; i<pointinfo->GetNumberOfArrays(); i++)
      {
      vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
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
  int fieldType = vtkSMDataRepresentationProxy::POINT_DATA;

  if(field == pqPipelineRepresentation::solidColor())
    {
    return 0;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkSMDataRepresentationProxy::CELL_DATA;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkSMDataRepresentationProxy::POINT_DATA;
    }

  return this->getNumberOfComponents(field.toAscii().data(),
    fieldType);
}

//-----------------------------------------------------------------------------
QPair<double, double> 
pqPipelineRepresentation::getColorFieldRange(const QString& array, int component)
{
  QPair<double,double>ret(0.0, 1.0);

  QString field = array;
  int fieldType = vtkSMDataRepresentationProxy::POINT_DATA;

  if(field == pqPipelineRepresentation::solidColor())
    {
    return ret;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkSMDataRepresentationProxy::CELL_DATA;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkSMDataRepresentationProxy::POINT_DATA;
    }

  vtkPVArrayInformation* representedInfo = this->Internal->getArrayInformation(
    field.toAscii().data(), fieldType);

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
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();

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
                         vtkSMDataRepresentationProxy::CELL_DATA);
      }
    else if(field.right(strlen(" (point)")) == " (point)")
      {
      field.chop(strlen(" (point)"));
      this->colorByArray(field.toAscii().data(), 
                         vtkSMDataRepresentationProxy::POINT_DATA);
      }
    }
}


//-----------------------------------------------------------------------------
QString pqPipelineRepresentation::getColorField(bool raw)
{
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();
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
bool pqPipelineRepresentation::getDataBounds(double bounds[6])
{
  vtkSMPropRepresentationProxy* repr = 
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
void pqPipelineRepresentation::setRepresentation(int representation)
{
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();
  pqSMAdaptor::setElementProperty(
    repr->GetProperty("Representation"), representation);
  repr->UpdateVTKObjects();
  this->onRepresentationChanged();
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onRepresentationChanged()
{
  vtkSMPropRepresentationProxy* repr = this->getRepresentationProxy();
  if (!repr)
    {
    return;
    }

  int reprType = this->getRepresentationType();
  if (reprType != vtkSMPVRepresentationProxy::VOLUME  &&
    reprType != vtkSMPVRepresentationProxy::SLICE)
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
    this->setRepresentation(vtkSMPVRepresentationProxy::OUTLINE);
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
