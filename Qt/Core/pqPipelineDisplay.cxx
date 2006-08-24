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
#include <QPointer>
#include <QList>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"


//-----------------------------------------------------------------------------
class pqPipelineDisplayInternal
{
public:
  vtkSmartPointer<vtkSMDataObjectDisplayProxy> DisplayProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqPipelineSource> Input;

  // Set of render modules showing this display. Typically,
  // it will be 1, but theoretically there can be more.
  QList<QPointer<pqRenderModule> > RenderModules;
};

//-----------------------------------------------------------------------------
pqPipelineDisplay::pqPipelineDisplay(const QString& name,
  vtkSMDataObjectDisplayProxy* display,
  pqServer* server, QObject* p/*=null*/):
  pqProxy("displays", name, display, server, p)
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
    
  // Check for new cell scalars.
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
  pqPipelineBuilder* builder = core->getPipelineBuilder();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    displayProxy->GetProperty("LookupTable"));
  vtkSMProxy* lut = 0;
  if (pp->GetNumberOfProxies() == 0)
    {
    lut = builder->createLookupTable(this);
    }
  else
    {
    lut = pp->GetProxy(0);
    }

  if (!lut)
    {
    qDebug() << "Failed to create/locate Lookup Table.";
    pqSMAdaptor::setElementProperty(
      displayProxy->GetProperty("ScalarVisibility"), 0);
    displayProxy->UpdateVTKObjects();
    return;
    }

  pqSMAdaptor::setElementProperty(
    displayProxy->GetProperty("ScalarVisibility"), 1);

  vtkPVArrayInformation* ai;
  if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
    ai = geomInfo->GetCellDataInformation()->GetArrayInformation(arrayname);
    pqSMAdaptor::setEnumerationProperty(
      displayProxy->GetProperty("ScalarMode"), "UseCellFieldData");
    }
  else
    {
    vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
    ai = geomInfo->GetPointDataInformation()->GetArrayInformation(arrayname);
    pqSMAdaptor::setEnumerationProperty(
      displayProxy->GetProperty("ScalarMode"), "UsePointFieldData");
    }

  // array couldn't be found, look for it on the reader
  // TODO: this support should be moved into the server manager and/or VTK
  if(!ai)
    {
    pp = vtkSMProxyProperty::SafeDownCast(displayProxy->GetProperty("Input"));
    vtkSMProxy* reader = pp->GetProxy(0);
    while((pp = vtkSMProxyProperty::SafeDownCast(reader->GetProperty("Input"))))
      reader = pp->GetProxy(0);
    QList<QVariant> prop;
    prop += arrayname;
    prop += 1;
    QList<QList<QVariant> > propertyList;
    propertyList.push_back(prop);
    if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      pqSMAdaptor::setSelectionProperty(
        reader->GetProperty("CellArrayStatus"), propertyList);
      reader->UpdateVTKObjects();
      vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
      ai = geomInfo->GetCellDataInformation()->GetArrayInformation(arrayname);
      }
    else
      {
      pqSMAdaptor::setSelectionProperty(
        reader->GetProperty("PointArrayStatus"), propertyList);
      reader->UpdateVTKObjects();
      vtkPVDataInformation* geomInfo = displayProxy->GetGeometryInformation();
      ai = geomInfo->GetPointDataInformation()->GetArrayInformation(arrayname);
      }
    }
  double range[2] = {0,1};
  if(ai)
    {
    ai->GetComponentRange(0, range);
    }
  QList<QVariant> tmp;
  tmp += range[0];
  tmp += range[1];
  pqSMAdaptor::setMultipleElementProperty(lut->GetProperty("ScalarRange"), tmp);
  pqSMAdaptor::setElementProperty(displayProxy->GetProperty("ColorArray"), 
                                  arrayname);
  lut->UpdateVTKObjects();
  displayProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqPipelineDisplay::shownIn(pqRenderModule* rm) const
{
  return this->Internal->RenderModules.contains(rm);
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::addRenderModule(pqRenderModule* rm)
{
  if (!this->Internal->RenderModules.contains(rm))
    {
    this->Internal->RenderModules.push_back(rm);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::removeRenderModule(pqRenderModule* rm)
{
  if (this->Internal->RenderModules.contains(rm))
    {
    this->Internal->RenderModules.removeAll(rm);
    }
}

//-----------------------------------------------------------------------------
unsigned int pqPipelineDisplay::getNumberOfRenderModules() const
{
  return this->Internal->RenderModules.size();
}

//-----------------------------------------------------------------------------
pqRenderModule* pqPipelineDisplay::getRenderModule(unsigned int index) const
{
  if (index >= this->getNumberOfRenderModules())
    {
    qDebug() << "Invalid index : " << index;
    return NULL;
    }
  return this->Internal->RenderModules[index];
}

//-----------------------------------------------------------------------------
void pqPipelineDisplay::renderAllViews(bool force /*=false*/)
{
  foreach(pqRenderModule* rm, this->Internal->RenderModules)
    {
    if (rm)
      {
      if (force)
        {
        rm->forceRender();
        }
      else
        {
        rm->render();
        }
      }
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
void pqPipelineDisplay::onVisibilityChanged()
{
  emit this->visibilityChanged(this->getDisplayProxy()->GetVisibilityCM());
}

//-----------------------------------------------------------------------------
/*
// This save state must go away. GUI just needs to save which window
// contains rendermodule with what ID.  ProxyManager will manage the display.
void pqPipelineDisplay::SaveState(vtkPVXMLElement *root,
    pqMultiView *multiView)
{
  if(!root || !multiView || !this->Internal)
    {
    return;
    }

  vtkPVXMLElement *element = 0;
  QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if((*iter)->Window && !(*iter)->DisplayName.isEmpty())
      {
      element = vtkPVXMLElement::New();
      element->SetName("Display");
      element->AddAttribute("name", (*iter)->DisplayName.toAscii().data());
      element->AddAttribute("windowID", pqXMLUtil::GetStringFromIntList(
          multiView->indexOf((*iter)->Window->parentWidget())).toAscii().data());
      root->AddNestedElement(element);
      element->Delete();
      }
    }
}*/


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

QList< QPair<double, double> >
pqPipelineDisplay::getColorFieldRanges(const QString& array)
{
  QList< QPair<double,double> > ret;
  
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

