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
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
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

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
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

  // before changing coloring mode, get the visibility status of the scalar bar
  // for current array, if any.
  bool sb_visibility = false;
  QPointer<pqScalarsToColors> lut = this->getLookupTable();
  pqView* view = this->getView();
  vtkSMProxy* sbProxy = NULL;
  if (lut && view)
    {
    sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
      lut->getProxy(), this->getView()->getProxy());
    if (sbProxy && vtkSMPropertyHelper(sbProxy, "Visibility").GetAsInt() == 1)
      {
      sb_visibility = true;
      }
    }

  vtkSMPVRepresentationProxy::SetScalarColoring(repr, arrayname, fieldtype);
  if (arrayname && arrayname[0])
    {
    this->resetLookupTableScalarRange();
    if (sb_visibility && view)
      {
      vtkSMPVRepresentationProxy::SetScalarBarVisibility(repr, view->getProxy(), true);
      }
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
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRangeOverTime()
{
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(proxy);
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

  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    // if scale range was initialized, we extend it, other we simply reset it.
    vtkSMPropertyHelper helper(lut->getProxy(), "ScalarRangeInitialized", true);
    bool extend_lut = helper.GetAsInt() == 1;
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy,
      extend_lut);
    // mark the scalar range as initialized.
    helper.Set(0, 1);
    lut->getProxy()->UpdateVTKObjects();
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
  if(field.right((int)strlen(" (cell)")) == " (cell)")
    {
    field.chop((int)strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right((int)strlen(" (point)")) == " (point)")
    {
    field.chop((int)strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  return this->getNumberOfComponents(field.toLatin1().data(),
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
  if(field.right((int)strlen(" (cell)")) == " (cell)")
    {
    field.chop((int)strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right((int)strlen(" (point)")) == " (point)")
    {
    field.chop((int)strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

   return this->getComponentName(field.toLatin1().data(), fieldType, component );
}

//-----------------------------------------------------------------------------
bool pqPipelineRepresentation::isPartial(const QString& array, int fieldType) const
{
  vtkPVArrayInformation* info = pqInternal::getArrayInformation(this,
    array.toLatin1().data(), fieldType);
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
  if(field.right((int)strlen(" (cell)")) == " (cell)")
    {
    field.chop((int)strlen(" (cell)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if(field.right((int)strlen(" (point)")) == " (point)")
    {
    field.chop((int)strlen(" (point)"));
    fieldType = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  vtkPVArrayInformation* arrayInfo = pqInternal::getArrayInformation(this,
    field.toLatin1().data(), fieldType);
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
    if(field.right((int)strlen(" (cell)")) == " (cell)")
      {
      field.chop((int)strlen(" (cell)"));
      this->colorByArray(field.toLatin1().data(), 
                         vtkDataObject::FIELD_ASSOCIATION_CELLS);
      }
    else if(field.right((int)strlen(" (point)")) == " (point)")
      {
      field.chop((int)strlen(" (point)"));
      this->colorByArray(field.toLatin1().data(), 
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
  vtkSMPropertyHelper(repr, "Representation").Set(representation.toLatin1().data());
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
  qDebug("FIXME");
}
//-----------------------------------------------------------------------------
const char* pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()
{
  // BUG #13564. I am renaming this key since we want to change the default
  // value for this key. Renaming the key makes it possible to override the
  // value without forcing users to reset their .config files.
  return "/representation/UnstructuredGridOutlineThreshold2";
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

  return 250; //  250 million cells.
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
