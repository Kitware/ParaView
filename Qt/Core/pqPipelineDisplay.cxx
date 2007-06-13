/*=========================================================================

   Program: ParaView
   Module:    pqPipelineDisplay.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqPipelineDisplay.cxx
/// \date 4/24/2006

#include "pqPipelineDisplay.h"


// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSmartPointer.h" 
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"


//-----------------------------------------------------------------------------
class pqPipelineDisplayInternal
{
public:
  vtkSmartPointer<vtkSMDataObjectDisplayProxy> DisplayProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqPipelineDisplayInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }

  // Convenience method to get array information.
  vtkPVArrayInformation* getArrayInformation(
    const char* arrayname, int fieldType)
    {
    if (!arrayname || !arrayname[0] || !this->DisplayProxy)
      {
      return 0; 
      }
    vtkSMDataObjectDisplayProxy* displayProxy = this->DisplayProxy;
    vtkPVDataInformation* geomInfo = displayProxy->GetDisplayedDataInformation();
    if(!geomInfo)
      {
      return 0;
      }

    vtkPVArrayInformation* info = NULL;
    if(fieldType == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      vtkPVDataSetAttributesInformation* cellinfo = 
        geomInfo->GetCellDataInformation();
      info = cellinfo->GetArrayInformation(arrayname);
      }
    else
      {
      vtkPVDataSetAttributesInformation* pointinfo = 
        geomInfo->GetPointDataInformation();
      info = pointinfo->GetArrayInformation(arrayname);
      }
    return info;
    }
};

//-----------------------------------------------------------------------------
pqPipelineDisplay::pqPipelineDisplay(const QString& name,
  vtkSMDataObjectDisplayProxy* display,
  pqServer* server, QObject* p/*=null*/):
  pqConsumerDisplay("displays", name, display, server, p)
{
  this->Internal = new pqPipelineDisplayInternal();
  this->Internal->DisplayProxy = display;

  const char* properties[] = {
    "ScalarVisibility",
    "LookupTable",
    "ScalarMode",
    "ColorArray", 
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
}

//-----------------------------------------------------------------------------
pqPipelineDisplay::~pqPipelineDisplay()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy* pqPipelineDisplay::getDisplayProxy() const
{
  return this->Internal->DisplayProxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineDisplay::getScalarOpacityFunctionProxy() const
{
  // We may want to create a new proxy is none exists.
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("ScalarOpacityFunction"));
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::createHelperProxies()
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
    this->getProxy()->GetProperty("ScalarOpacityFunction"),
    opacityFunction);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::setDefaultPropertyValues()
{
  // We deliberately don;t call superclass. For somereason,
  // its messing up with the scalar coloring.
  // this->Superclass::setDefaultPropertyValues();

  if (!this->isVisible())
    {
    // don't worry about invisible displays.
    return;
    }

  this->createHelperProxies();

  vtkSMDataObjectDisplayProxy* displayProxy = this->getDisplayProxy();
  if (!displayProxy)
    {
    return;
    }

  // if the source created a new point scalar, use it
  // else if the source created a new cell scalar, use it
  // else if the input color by array exists in this source, use it
  // else color by property
  
  vtkPVDataInformation* inGeomInfo = 0;
  vtkPVDataInformation* geomInfo = 0;
  vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;

  vtkPVDataInformation* dataInfo = 0;
  dataInfo = this->getInput()->getDataInformation();

  // get data set type
  // and set the default representation
  if(dataInfo)
    {
    int dataSetType = dataInfo->GetDataSetType();
    if(dataSetType == VTK_POLY_DATA ||
       dataSetType == VTK_UNSTRUCTURED_GRID ||
       dataSetType == VTK_HYPER_OCTREE ||
       dataSetType == VTK_GENERIC_DATA_SET)
      {
      displayProxy->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
      }
    else if(dataSetType == VTK_RECTILINEAR_GRID ||
       dataSetType == VTK_STRUCTURED_GRID ||
       dataSetType == VTK_IMAGE_DATA)
      {
      int* ext = dataInfo->GetExtent();
      if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
        {
        displayProxy->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
        }
      else
        {
        displayProxy->SetRepresentationCM(vtkSMDataObjectDisplayProxy::OUTLINE);
        }
      }
    else
      {
      displayProxy->SetRepresentationCM(vtkSMDataObjectDisplayProxy::OUTLINE);
      }
    }
  
  geomInfo = displayProxy->GetDisplayedDataInformation();

  // Locate input display.
  pqPipelineFilter* myInput = qobject_cast<pqPipelineFilter*>(this->getInput());
  pqPipelineDisplay* upstreamDisplay = 0;
  if (myInput && myInput->getInputCount() > 0)
    {
    pqPipelineSource* myInputsInput = myInput->getInput(0);
    /*
    upstreamDisplay = qobject_cast<pqPipelineDisplay*>(
      myInputsInput->getDisplay(this->getViewModule(0)));
      */
    if (upstreamDisplay)
      {
      inGeomInfo = upstreamDisplay->getDisplayProxy()->GetDisplayedDataInformation();
      }
    }

  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".
  if(geomInfo)
    {
    attrInfo = geomInfo->GetPointDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetPointDataInformation() : 0;
    pqPipelineDisplay::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
      return;
      }
    }
    
  // Check for new cell scalars.
  if(geomInfo)
    {
    attrInfo = geomInfo->GetCellDataInformation();
    inAttrInfo = inGeomInfo? inGeomInfo->GetCellDataInformation() : 0;
    pqPipelineDisplay::getColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      this->colorByArray(arrayInfo->GetName(), 
                         vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
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
                         vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
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
                         vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      return;
      }
    }

  // Try to inherit the same array selected by the input.
  if (upstreamDisplay)
    {
    // propagate solid color.
    double rgb[3];
    upstreamDisplay->getDisplayProxy()->GetColorCM(rgb);
    displayProxy->SetColorCM(rgb);

    this->setColorField(upstreamDisplay->getColorField(false));
    return;
    }

  // Color by property.
  this->colorByArray(NULL, 0);
}

//-----------------------------------------------------------------------------
int pqPipelineDisplay::getNumberOfComponents(
  const char* arrayname, int fieldtype)
{
  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    arrayname, fieldtype);
  return (info? info->GetNumberOfComponents() : 0);
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::colorByArray(const char* arrayname, int fieldtype)
{
  vtkSMDataObjectDisplayProxy* displayProxy = this->getDisplayProxy();
  if (!displayProxy)
    {
    return;
    }

  if(arrayname == 0)
    {
    pqSMAdaptor::setElementProperty(
      displayProxy->GetProperty("ScalarVisibility"), 0);
    displayProxy->UpdateVTKObjects();
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
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      displayProxy->GetProperty("LookupTable"));
    // When lookup table manager is not available,
    // we simply create new lookup tables for each display.
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
      displayProxy->GetProperty("ScalarVisibility"), 0);
    displayProxy->UpdateVTKObjects();
    return;
    }

  vtkSMProxy* oldlutProxy = 
    pqSMAdaptor::getProxyProperty(displayProxy->GetProperty("LookupTable"));
  pqScalarsToColors* old_stc = 0;
  if (oldlutProxy != lut)
    {
    // Localte pqScalarsToColors for the old LUT and update 
    // it's scalar bar visibility.
    pqServerManagerModel* smmodel = core->getServerManagerModel();
    old_stc = qobject_cast<pqScalarsToColors*>(
      smmodel->getPQProxy(oldlutProxy));
    }
  
  pqSMAdaptor::setProxyProperty(
    displayProxy->GetProperty("LookupTable"), lut);

  // If old LUT was present update the visibility of the scalar bars
  if (old_stc)
      {
      old_stc->hideUnusedScalarBars();
      }
  pqSMAdaptor::setElementProperty(
    displayProxy->GetProperty("ScalarVisibility"), 1);


  if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    pqSMAdaptor::setEnumerationProperty(
      displayProxy->GetProperty("ScalarMode"), "UseCellFieldData");
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(
      displayProxy->GetProperty("ScalarMode"), "UsePointFieldData");
    }
  pqSMAdaptor::setElementProperty(
    displayProxy->GetProperty("ColorArray"), arrayname);
  lut->UpdateVTKObjects();
  displayProxy->UpdateVTKObjects();

  this->updateLookupTableScalarRange();
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::resetLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField!= "" && colorField != "Solid Color")
    {
    QPair<double,double> range = this->getColorFieldRange();
    lut->setScalarRange(range.first, range.second);

    // scalar opacity is treated as slave to the lookup table.
    this->setScalarOpacityRange(range.first, range.second);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::updateLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  if (!lut || lut->getScalarRangeLock())
    {
    return;
    }

  QString colorField = this->getColorField();
  if (colorField == "" || colorField == "Solid Color")
    {
    return;
    }

  QPair<double, double> range = this->getColorFieldRange();
  lut->setWholeScalarRange(range.first, range.second);

  // Adjust opacity function range.
  vtkSMProxy* opacityFunction = this->getScalarOpacityFunctionProxy();
  if (opacityFunction && !lut->getScalarRangeLock() &&
    this->getDisplayProxy()->GetRepresentationCM() == 
    vtkSMDataObjectDisplayProxy::VOLUME)
    {
    QPair<double, double> adjusted_range = lut->getScalarRange();

    // Opacity function always follows the LUT scalar range.
    // scalar opacity is treated as slave to the lookup table.
    this->setScalarOpacityRange(adjusted_range.first, adjusted_range.second);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::setScalarOpacityRange(double min, double max)
{
  // A far more better way would be to create a new pqProxy subclass for
  // the piecewise function.
  vtkSMProxy* opacityFunction = this->getScalarOpacityFunctionProxy();
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
  dnew = (dnew > 0) ? dnew : 1;

  double scale = dnew/dold;
  for (int cc=0; cc < controlPoints.size(); 
    cc+= dvp->GetNumberOfElementsPerCommand())
    {
    controlPoints[cc] = 
      scale * (controlPoints[cc].toDouble()-current_range.first) + min;
    }

  pqSMAdaptor::setMultipleElementProperty(dvp, controlPoints);
  opacityFunction->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::getColorArray(
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
QList<QString> pqPipelineDisplay::getColorFields()
{
  vtkSMDataObjectDisplayProxy* displayProxy = this->getDisplayProxy();

  QList<QString> ret;
  if(!displayProxy)
    {
    return ret;
    }

  int representation = displayProxy->GetRepresentationCM();

  if (representation != vtkSMDataObjectDisplayProxy::VOLUME)
    {
    // Actor color is one way to color this part.
    // Not applicable when volume rendering.
    ret.append("Solid Color");
    }

  vtkPVDataInformation* geomInfo = displayProxy->GetDisplayedDataInformation();
  if(!geomInfo)
    {
    return ret;
    }

  // get cell arrays
  vtkPVDataSetAttributesInformation* cellinfo = 
    geomInfo->GetCellDataInformation();
  if(cellinfo && representation != vtkSMDataObjectDisplayProxy::VOLUME)
    {
    for(int i=0; i<cellinfo->GetNumberOfArrays(); i++)
      {
      vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
      QString name = info->GetName();
      name += " (cell)";
      ret.append(name);
      }
    }
  
  // get point arrays
  vtkPVDataSetAttributesInformation* pointinfo = 
    geomInfo->GetPointDataInformation();
  if(pointinfo)
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
int pqPipelineDisplay::getColorFieldNumberOfComponents(const QString& array)
{
  QString field = array;
  int fieldType = vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA;

  if(field == "Solid Color")
    {
    return 0;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA;
    }

  return this->getNumberOfComponents(field.toAscii().data(),
    fieldType);
}

//-----------------------------------------------------------------------------
QPair<double, double> 
pqPipelineDisplay::getColorFieldRange(const QString& array, int component)
{
  QPair<double,double>ret(0.0, 1.0);

  QString field = array;
  int fieldType = vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA;

  if(field == "Solid Color")
    {
    return ret;
    }
  if(field.right(strlen(" (cell)")) == " (cell)")
    {
    field.chop(strlen(" (cell)"));
    fieldType = vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA;
    }
  else if(field.right(strlen(" (point)")) == " (point)")
    {
    field.chop(strlen(" (point)"));
    fieldType = vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA;
    }

  vtkPVArrayInformation* info = this->Internal->getArrayInformation(
    field.toAscii().data(), fieldType);
  if(info)
    {
    if (component <info->GetNumberOfComponents())
      {
      double range[2];
      info->GetComponentRange(component, range);
      return QPair<double,double>(range[0], range[1]);
      }
    }
  return ret;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqPipelineDisplay::getColorFieldRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField!= "" && colorField != "Solid Color")
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
void pqPipelineDisplay::setColorField(const QString& value)
{
  vtkSMDisplayProxy* displayProxy = this->getDisplayProxy();

  if(!displayProxy)
    {
    return;
    }

  QString field = value;

  if(field == "Solid Color")
    {
    this->colorByArray(0, 0);
    }
  else
    {
    if(field.right(strlen(" (cell)")) == " (cell)")
      {
      field.chop(strlen(" (cell)"));
      this->colorByArray(field.toAscii().data(), 
                         vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      }
    else if(field.right(strlen(" (point)")) == " (point)")
      {
      field.chop(strlen(" (point)"));
      this->colorByArray(field.toAscii().data(), 
                         vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
      }
    }
}


//-----------------------------------------------------------------------------
QString pqPipelineDisplay::getColorField(bool raw)
{
  vtkSMDisplayProxy* displayProxy = this->getDisplayProxy();
  if (!displayProxy)
    {
    return "";
    }

  QVariant scalarColor = pqSMAdaptor::getElementProperty(
    displayProxy->GetProperty("ScalarVisibility"));
  if(scalarColor.toBool())
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
      displayProxy->GetProperty("ScalarMode"));
    QString scalarArray = pqSMAdaptor::getElementProperty(
      displayProxy->GetProperty("ColorArray")).toString();
    if (raw)
      {
      return scalarArray;
      }
    if(scalarMode == "UseCellFieldData")
      {
      return scalarArray + " (cell)";
      }
    else if(scalarMode == "UsePointFieldData")
      {
      return scalarArray + " (point)";
      }
    }
  else
    {
    return "Solid Color";
    }
  return QString();
}

//-----------------------------------------------------------------------------
bool pqPipelineDisplay::getDataBounds(double bounds[6])
{
  vtkSMDataObjectDisplayProxy* display = 
    this->getDisplayProxy();

  if(!display || !display->GetDisplayedDataInformation())
    {
    return false;
    }
  display->GetDisplayedDataInformation()->GetBounds(bounds);
  return true;
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::setRepresentation(int representation)
{
  vtkSMDataObjectDisplayProxy* display = this->getDisplayProxy();
  display->SetRepresentationCM(representation);
  this->onRepresentationChanged();
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::onRepresentationChanged()
{
 vtkSMDataObjectDisplayProxy* display = 
    this->getDisplayProxy();

  if(!display || 
    display->GetRepresentationCM() != vtkSMDataObjectDisplayProxy::VOLUME)
    {
    // Nothing to do here.
    return;
    }

  // This is essential, since otherwise the geometry filter does not execute
  // and we wont get the correct arrays available.
  display->Update();

  // Representation is Volume, is color array set?
  QString colorField = this->getColorField(false);
  QRegExp regExpCell(" \\(cell\\)\\w*$");
  if (colorField == "Solid Color" || regExpCell.indexIn(colorField) != -1)
    {
    // Current color field is not suitable for Volume rendering.
    // Change it.
    QList<QString> colorFields = this->getColorFields();
    if (colorFields.size() == 0)
      {
      qCritical() << "Cannot volume render since no point data available!";
      display->SetRepresentationCM(vtkSMDataObjectDisplayProxy::OUTLINE);
      return;
      }
    this->setColorField(colorFields[0]);
    }
}
