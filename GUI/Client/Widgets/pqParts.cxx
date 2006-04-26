/*=========================================================================

   Program:   ParaQ
   Module:    pqParts.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqParts.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVGeometryInformation.h>
#include <vtkPVArrayInformation.h>
#include <vtkSMLookupTableProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkProcessModule.h>

#include "pqNameCount.h"
#include "pqPipelineData.h"
#include "pqSMAdaptor.h"
#include "pqPipelineObject.h"

#include <QString>

vtkSMDisplayProxy* pqPart::Add(vtkSMRenderModuleProxy* rm, vtkSMSourceProxy* Part)
{
  // without this, you will get runtime errors from the part display
  // (connected below). this should be fixed
  Part->CreateParts();

  // Create part display.
  vtkSMDisplayProxy *partdisplay = rm->CreateDisplayProxy();

  // Set the part as input to the part display.
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(partdisplay->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(Part);

  vtkSMDataObjectDisplayProxy *dod
    = vtkSMDataObjectDisplayProxy::SafeDownCast(partdisplay);
  if (dod)
    {
    dod->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
    }

  partdisplay->UpdateVTKObjects();

  // Add the part display to the render module.
  pp = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->AddProxy(partdisplay);
  
  rm->UpdateVTKObjects();

  // set default colors for display
  pqPart::Color(partdisplay);

  // Allow the render module proxy to maintain the part display.
//  partdisplay->Delete();

  return partdisplay;
}

static void pqGetColorArray(
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


/// color the part to its default color
void pqPart::Color(vtkSMDisplayProxy* Part)
{
  // if the source created a new point scalar, use it
  // else if the source created a new cell scalar, use it
  // else if the input color by array exists in this source, use it
  // else color by property
  
  vtkPVDataInformation* inGeomInfo = 0;
  vtkPVDataInformation* geomInfo = 0;
  vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;
  vtkSMDisplayProxy* dproxy;

  dproxy = Part;
  if (dproxy)
    {
    geomInfo = dproxy->GetGeometryInformation();
    }
    
#if 0
  pqPipelineObject* input = 0;
  pqPipelineData* pipeline = pqPipelineData::instance();
  pqPipelineObject* pqpart = pipeline->getObjectFor(Part);  // this doesn't work with compound proxies
  input = pqpart->GetInput(0);

  if (input)
    {
    dproxy = input->GetDisplayProxy();
    if (dproxy)
      {
      inGeomInfo = dproxy->GetGeometryInformation();
      }
    }
#endif
  
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
  pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
  if(arrayInfo)
    {
    pqPart::Color(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
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
  pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
  if(arrayInfo)
    {
    pqPart::Color(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
    return;
    }
    
#if 0
  // Inherit property color from input.
  if (input)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      input->GetDisplayProxy()->GetProperty("Color"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to find property Color on input->DisplayProxy.");
      return;
      }
    double rgb[3] = { 1, 1, 1};
    input->GetDisplayProxy()->GetColorCM(rgb);
    this->DisplayProxy->SetColorCM(rgb);
    this->DisplayProxy->UpdateVTKObjects();
    } 
#endif

  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetPointDataInformation();
    pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      pqPart::Color(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
      return;
      }
    }

  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetCellDataInformation();
    pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      pqPart::Color(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      return;
      }
    }

#if 0
  // Try to use the same array selected by the input.
  if (input)
    {
    colorMap = input->GetPVColorMap();
    int colorField = -1;
    if (colorMap)
      {
      colorField = input->GetDisplayProxy()->GetScalarModeCM();

      // Find the array in our info.
      switch (colorField)
        {
      case vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA:
          attrInfo = geomInfo->GetPointDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->ColorByArray(
              colorMap, vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
            return;
            }
          break;
        case vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA:
          attrInfo = geomInfo->GetCellDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->ColorByArray(
              colorMap, vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
            return;
            }
          break;
        default:
          vtkErrorMacro("Bad attribute.");
          return;
        }

      }
    }
#endif

  // Color by property.
  pqPart::Color(Part, NULL, 0);
}

/// color the part by a specific field, if fieldname is NULL, colors by actor color
void pqPart::Color(vtkSMDisplayProxy* Part, const char* fieldname, int fieldtype)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("LookupTable"));
  pp->RemoveAllProxies();
  
  if(fieldname == 0)
    {
    pqSMAdaptor::setElementProperty(Part, Part->GetProperty("ScalarVisibility"), 0);
    }
  else
    {
    // create lut
    vtkSMLookupTableProxy* lut = vtkSMLookupTableProxy::SafeDownCast(
      vtkSMObject::GetProxyManager()->NewProxy("lookup_tables", "LookupTable"));
    lut->SetConnectionID(Part->GetConnectionID());
    lut->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("LookupTable"));
    pp->AddProxy(lut);

    // Register the lookup table with the server manager so save/restore
    // works correctly.
    QString name;
    pqPipelineData *pipeline = pqPipelineData::instance();
    name.setNum(pipeline->getNameCount()->GetCountAndIncrement("LookupTable"));
    name.prepend("LookupTable");
    vtkSMObject::GetProxyManager()->RegisterProxy("lookup_tables",
        name.toAscii().data(), lut);
    lut->Delete();

    pqSMAdaptor::setElementProperty(Part, Part->GetProperty("ScalarVisibility"), 1);

    vtkPVArrayInformation* ai;

    if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
      ai = geomInfo->GetCellDataInformation()->GetArrayInformation(fieldname);
      pqSMAdaptor::setEnumerationProperty(Part, Part->GetProperty("ScalarMode"), "UseCellFieldData");
      }
    else
      {
      vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
      ai = geomInfo->GetPointDataInformation()->GetArrayInformation(fieldname);
      pqSMAdaptor::setEnumerationProperty(Part, Part->GetProperty("ScalarMode"), "UsePointFieldData");
      }
    
    // array couldn't be found, look for it on the reader
    // TODO: this support should be moved into the server manager and/or VTK
    if(!ai)
      {
      pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("Input"));
      vtkSMProxy* reader = pp->GetProxy(0);
      while((pp = vtkSMProxyProperty::SafeDownCast(reader->GetProperty("Input"))))
        reader = pp->GetProxy(0);
      QList<QVariant> prop;
      prop += fieldname;
      prop += 1;
      QList<QList<QVariant> > property;
      property.push_back(prop);
      if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
        {
        pqSMAdaptor::setSelectionProperty(reader, reader->GetProperty("CellArrayStatus"), property);
        reader->UpdateVTKObjects();
        vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
        ai = geomInfo->GetCellDataInformation()->GetArrayInformation(fieldname);
        }
      else
        {
        pqSMAdaptor::setSelectionProperty(reader, reader->GetProperty("PointArrayStatus"), property);
        reader->UpdateVTKObjects();
        vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
        ai = geomInfo->GetPointDataInformation()->GetArrayInformation(fieldname);
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
    pqSMAdaptor::setMultipleElementProperty(lut, lut->GetProperty("ScalarRange"), tmp);
    pqSMAdaptor::setElementProperty(Part, Part->GetProperty("ColorArray"), fieldname);
    lut->UpdateVTKObjects();
    }

  Part->UpdateVTKObjects();
}

QList<QString> pqPart::GetColorFields(vtkSMDisplayProxy* display)
{
  QList<QString> ret;
  if(!display)
    {
    return ret;
    }

  // Actor color is one way to color this part
  ret.append("Property");

  vtkPVDataInformation* geomInfo = display->GetGeometryInformation();
  if(!geomInfo)
    {
    return ret;
    }

  // get cell arrays
  vtkPVDataSetAttributesInformation* cellinfo = geomInfo->GetCellDataInformation();
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
  vtkPVDataSetAttributesInformation* pointinfo = geomInfo->GetPointDataInformation();
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

void pqPart::SetColorField(vtkSMDisplayProxy* Part, const QString& value)
{
  if(!Part)
    {
    return;
    }

  QString field = value;

  if(field == "Property")
    {
    pqPart::Color(Part);
    }
  else
    {
    if(field.right(strlen(" (cell)")) == " (cell)")
      {
      field.chop(strlen(" (cell)"));
      pqPart::Color(Part, field.toAscii().data(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      }
    else if(field.right(strlen(" (point)")) == " (point)")
      {
      field.chop(strlen(" (point)"));
      pqPart::Color(Part, field.toAscii().data(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
      }
    }
}


QString pqPart::GetColorField(vtkSMDisplayProxy* Part)
{
  QVariant scalarColor = pqSMAdaptor::getElementProperty(Part, Part->GetProperty("ScalarVisibility"));
  if(scalarColor.toBool())
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(Part, Part->GetProperty("ScalarMode"));
    QString scalarArray = pqSMAdaptor::getElementProperty(Part, Part->GetProperty("ColorArray")).toString();
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
    return "Property";
    }
  return QString();
}

