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
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSmartPointer.h" 
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMInputProperty.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"
#include "pqScalarsToColors.h"


//-----------------------------------------------------------------------------
class pqPipelineDisplayInternal
{
public:
  vtkSmartPointer<vtkSMDataObjectDisplayProxy> DisplayProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqPipelineSource> Input;
};

//-----------------------------------------------------------------------------
pqPipelineDisplay::pqPipelineDisplay(const QString& name,
  vtkSMDataObjectDisplayProxy* display,
  pqServer* server, QObject* p/*=null*/):
  pqDisplay("displays", name, display, server, p)
{
  this->Internal = new pqPipelineDisplayInternal();
  this->Internal->DisplayProxy = display;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->Input = 0;
  if (display)
    {
    this->Internal->VTKConnect->Connect(display->GetProperty("Input"),
      vtkCommand::ModifiedEvent, this, SLOT(onInputChanged()));
    this->Internal->VTKConnect->Connect(display->GetProperty("Visibility"),
      vtkCommand::ModifiedEvent, this, SLOT(onVisibilityChanged()));
    }
  // This will make sure that if the input is already set.
  this->onInputChanged();
}

//-----------------------------------------------------------------------------
pqPipelineDisplay::~pqPipelineDisplay()
{
  if (this->Internal->DisplayProxy)
    {
    this->Internal->VTKConnect->Disconnect(
      this->Internal->DisplayProxy->GetProperty("Input"));
    }
  if (this->Internal->Input)
    {
    this->Internal->Input->removeDisplay(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy* pqPipelineDisplay::getDisplayProxy() const
{
  return this->Internal->DisplayProxy;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineDisplay::getInput() const
{
  return this->Internal->Input;
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::onInputChanged()
{
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    this->Internal->DisplayProxy->GetProperty("Input"));
  if (!ivp)
    {
    qDebug() << "Display proxy has no input property!";
    return;
    }
  pqPipelineSource* added = 0;
  pqPipelineSource* removed = 0;

  int new_proxes_count = ivp->GetNumberOfProxies();
  if (new_proxes_count == 0)
    {
    removed = this->Internal->Input;
    this->Internal->Input = 0;
    }
  else if (new_proxes_count == 1)
    {
    pqServerManagerModel* model = pqServerManagerModel::instance();
    removed = this->Internal->Input;
    this->Internal->Input = model->getPQSource(ivp->GetProxy(0));
    added = this->Internal->Input;
    if (ivp->GetProxy(0) && !this->Internal->Input)
      {
      qDebug() << "Display could not locate the pqPipelineSource object "
        << "for the input proxy.";
      }
    }
  else if (new_proxes_count > 1)
    {
    qDebug() << "Displays with more than 1 input are not handled.";
    return;
    }

  // Now tell the pqPipelineSource about the changes in the displays.
  if (removed)
    {
    removed->removeDisplay(this);
    }
  if (added)
    {
    added->addDisplay(this);
    }

}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::setDefaultColorParametes()
{
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

  geomInfo = displayProxy->GetGeometryInformation();
    
  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".
  if(geomInfo)
    {
    attrInfo = geomInfo->GetPointDataInformation();
    if (inGeomInfo)
      {
      inAttrInfo = inGeomInfo->GetPointDataInformation();
      }
    else
      {
      inAttrInfo = 0;
      }
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
    if (inGeomInfo)
      {
      inAttrInfo = inGeomInfo->GetCellDataInformation();
      }
    else
      {
      inAttrInfo = 0;
      }
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

  // Color by property.
  this->colorByArray(NULL, 0);
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
    pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
      this->getServer(), arrayname, 0);
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
      pqPipelineBuilder* builder = core->getPipelineBuilder();
      lut = builder->createLookupTable(this);
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
vtkSMProxy* pqPipelineDisplay::getLookupTableProxy()
{
  return pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("LookupTable"));
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqPipelineDisplay::getLookupTable()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxy* lut = this->getLookupTableProxy();

  return (lut? qobject_cast<pqScalarsToColors*>(smmodel->getPQProxy(lut)): 0);
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::resetLookupTableScalarRange()
{

  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField!= "" && colorField != "Solid Color")
    {
    QList<QPair<double,double> > ranges = this->getColorFieldRanges(colorField);
    if (ranges.size() > 0)
      {
      // TODO: use the correct array component.
      QPair<double, double> range = ranges[0];
      lut->setScalarRange(range.first, range.second);
      }
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

  QList<QPair<double, double> >ranges = this->getColorFieldRanges(colorField);
  if (ranges.size() > 0)
    {
    QPair<double, double> range = ranges[0];
    lut->setWholeScalarRange(range.first, range.second);
    }
}

//-----------------------------------------------------------------------------
bool pqPipelineDisplay::isVisible() const
{
  return this->getDisplayProxy()->GetVisibilityCM();
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::setVisible(bool visible)
{
  this->getDisplayProxy()->SetVisibilityCM((visible? 1 : 0));
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
  vtkSMDisplayProxy* displayProxy = this->getDisplayProxy();

  QList<QString> ret;
  if(!displayProxy)
    {
    return ret;
    }

  // Actor color is one way to color this part
  ret.append("Solid Color");

  vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
  if(!geomInfo)
    {
    return ret;
    }

  // get cell arrays
  vtkPVDataSetAttributesInformation* cellinfo = 
    geomInfo->GetCellDataInformation();
  if(cellinfo)
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

QList<QPair<double, double> >
pqPipelineDisplay::getColorFieldRanges(const QString& array)
{
  QList<QPair<double,double> > ret;
  
  vtkSMDisplayProxy* displayProxy = this->getDisplayProxy();

  if(!displayProxy)
    {
    return ret;
    }

  vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
  if(!geomInfo)
    {
    return ret;
    }

  QString field = array;
  vtkPVArrayInformation* info = NULL;

  if(field == "Solid Color")
    {
    return ret;
    }
  else
    {
    if(field.right(strlen(" (cell)")) == " (cell)")
      {
      field.chop(strlen(" (cell)"));
      vtkPVDataSetAttributesInformation* cellinfo = 
        geomInfo->GetCellDataInformation();
      info = cellinfo->GetArrayInformation(field.toAscii().data());
      }
    else if(field.right(strlen(" (point)")) == " (point)")
      {
      field.chop(strlen(" (point)"));
      vtkPVDataSetAttributesInformation* pointinfo = 
        geomInfo->GetPointDataInformation();
      info = pointinfo->GetArrayInformation(field.toAscii().data());
      }
    if(info)
      {
      double range[2];
      for(int i=0; i<info->GetNumberOfComponents(); i++)
        {
        info->GetComponentRange(i, range);
        ret.append(QPair<double,double>(range[0], range[1]));
        }
      }
    }
  return ret;
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

