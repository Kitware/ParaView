/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTemporalXYPlotDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTemporalXYPlotDisplayProxy.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTemporalCacheFilter.h"

#include <vtkstd/string>
#include <vtkstd/vector>


class vtkSMTemporalXYPlotDisplayProxyInternal
{
public:
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings PointArrayNames;
  VectorOfStrings CellArrayNames;
  int PointArrayNamesModified;
  int CellArrayNamesModified;

  vtkSMTemporalXYPlotDisplayProxyInternal()
    {
    this->PointArrayNamesModified = 0;
    this->CellArrayNamesModified = 0;
    }
};

vtkStandardNewMacro(vtkSMTemporalXYPlotDisplayProxy);
vtkCxxRevisionMacro(vtkSMTemporalXYPlotDisplayProxy, "1.1");
vtkCxxSetObjectMacro(vtkSMTemporalXYPlotDisplayProxy,
  AnimationCueProxy, vtkSMAnimationCueProxy);

//-----------------------------------------------------------------------------
vtkSMTemporalXYPlotDisplayProxy::vtkSMTemporalXYPlotDisplayProxy()
{
  this->PlotPointData = 1;
  this->TemporalCacheProxy =0 ;
  this->Internal = new vtkSMTemporalXYPlotDisplayProxyInternal;
  this->AnimationCueProxy = 0;
  this->AbortTemporalPlot = 0;
  this->InGenerateTemporalPlot = 0;
  this->LockTemporalCache = 0;
}

//-----------------------------------------------------------------------------
vtkSMTemporalXYPlotDisplayProxy::~vtkSMTemporalXYPlotDisplayProxy()
{
  this->TemporalCacheProxy =0;
  delete this->Internal;
  this->SetAnimationCueProxy(0);
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  
  this->TemporalCacheProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("TemporalCache"));
  if (!this->TemporalCacheProxy)
    {
    vtkErrorMacro("XML description missing required subproxy "
      "TemporalCacheProxy.");
    return;
    }
  this->TemporalCacheProxy->SetServers(vtkProcessModule::DATA_SERVER);
  
  // Create the objects.
  this->Superclass::CreateVTKObjects(numObjects);
  
  if (!this->ObjectsCreated)
    {
    return;
    }

}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::AddInput(vtkSMSourceProxy* input, 
  const char* arg1, int arg2)
{
  this->CreateVTKObjects(1);
  if (!this->ObjectsCreated)
    {
    // Make sure that the proxy has been created fine.
    return;
    }

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->TemporalCacheProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to locate Input on TemporalCacheProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(input);
  this->TemporalCacheProxy->UpdateVTKObjects();

  this->Superclass::AddInput(this->TemporalCacheProxy, arg1, arg2);
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::UpdatePropertyInformation()
{
  this->Superclass::UpdatePropertyInformation();
  this->UpdateArrayInformationProperties();
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::UpdatePropertyInformation(
  vtkSMProperty* prop)
{
  this->Superclass::UpdatePropertyInformation(prop);
  this->UpdateArrayInformationProperties();
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::UpdateArrayInformationProperties()
{
  if(!this->ObjectsCreated)
    {
    return;
    }

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->GetProperty("Input"));
  vtkSMSourceProxy* input = (ip && ip->GetNumberOfProxies() > 0)?
    vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0)) : NULL;
  if (!input)
    {
    return;
    }

  vtkPVDataInformation* info = input->GetDataInformation();
  // Update information in the two fake info properties.
  this->UpdateArrayInformationProperty("CellArrayInfo", info->GetCellDataInformation());
  this->UpdateArrayInformationProperty("PointArrayInfo", info->GetPointDataInformation());
}


//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::UpdateArrayInformationProperty(
  const char* property,  vtkPVDataSetAttributesInformation* info)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty(property));
    
  if (!svp || !info)
    {
    return;
    }

  int numArrays = info->GetNumberOfArrays();
  int e = 0;
  svp->SetNumberOfElements(numArrays);
  for(int i=0; i<numArrays; i++)
    {
    vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(i);
    if( arrayInfo->GetNumberOfComponents() == 1 )
      {
      svp->SetElement(e++, arrayInfo->GetName());
      }
    }
  svp->SetNumberOfElements(e);
  svp->UpdateDependentDomains();
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::RemoveAllPointArrayNames()
{
  this->Internal->PointArrayNames.clear();
  this->Internal->PointArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::AddPointArrayName(const char* name)
{
  this->Internal->PointArrayNames.push_back(name);
  this->Internal->PointArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::RemoveAllCellArrayNames()
{
  this->Internal->CellArrayNames.clear();
  this->Internal->CellArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::AddCellArrayName(const char* name)
{
  this->Internal->CellArrayNames.push_back(name);
  this->Internal->CellArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  int push_array_names;
  // Check if array names need to be pushed.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->TemporalCacheProxy->GetProperty("AttributeToCollect"));
  
  if (this->PlotPointData)
    {
    if (ivp->GetElement(0) != vtkTemporalCacheFilter::POINT_DATA)
      {
      push_array_names = 1;
      ivp->SetElement(0, vtkTemporalCacheFilter::POINT_DATA);
      this->TemporalCacheProxy->UpdateVTKObjects();
      }
    else
      {
      push_array_names = this->Internal->PointArrayNamesModified;
      }
    this->Internal->PointArrayNamesModified = 0;
    }
  else 
    {
    if (ivp->GetElement(0) != vtkTemporalCacheFilter::CELL_DATA)
      {
      push_array_names = 1;
      ivp->SetElement(0, vtkTemporalCacheFilter::CELL_DATA);
      this->TemporalCacheProxy->UpdateVTKObjects();
      }
    else
      {
      push_array_names = this->Internal->CellArrayNamesModified;
      }
    this->Internal->CellArrayNamesModified = 0;
    }
  
  if (!push_array_names)
    {
    return;
    }
  
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->XYPlotActorProxy->GetProperty("ArrayNames"));

  if (!svp)
    {
    vtkErrorMacro("Failed to find property ArrayNames on XYPlotActorProxy.");
    return;
    }
  
  vtkSMTemporalXYPlotDisplayProxyInternal::VectorOfStrings::iterator iter =
    (this->PlotPointData)?
    this->Internal->PointArrayNames.begin() :
      this->Internal->CellArrayNames.begin();
  vtkSMTemporalXYPlotDisplayProxyInternal::VectorOfStrings::iterator end =
    (this->PlotPointData)?
    this->Internal->PointArrayNames.end() :
      this->Internal->CellArrayNames.end();
 
  svp->SetNumberOfElements(0);
 
  for (int i=0; iter != end; ++iter, ++i)
    {
    svp->SetElement(i, iter->c_str());
    }
  this->XYPlotActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::GenerateTemporalPlot()
{
  if (!this->AnimationCueProxy)
    {
    vtkErrorMacro("AnimationCueProxy must be set to generate the temporal plot.");
    return;
    }
 
  this->InGenerateTemporalPlot = 1;
  this->TemporalCacheProxy->GetProperty("ClearCache")->Modified();
  this->TemporalCacheProxy->UpdateVTKObjects();

  vtkSMDoubleVectorProperty* cadProperty = vtkSMDoubleVectorProperty::SafeDownCast(
    this->TemporalCacheProxy->GetProperty("CollectAttributeData"));

  vtkSMProxy* sourceProxy = this->AnimationCueProxy->GetAnimatedProxy();
  vtkSMDoubleVectorProperty* tsvProperty = vtkSMDoubleVectorProperty::SafeDownCast(
    sourceProxy->GetProperty("TimestepValues"));
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->AnimationCueProxy->GetAnimatedProperty());
  
  double starttime = this->AnimationCueProxy->GetStartTime();
  double endtime = this->AnimationCueProxy->GetEndTime();

  vtkAnimationCue::AnimationCueInfo info;
  info.StartTime = starttime;
  info.EndTime = endtime;
  info.DeltaTime = 1.0;
 
  this->AbortTemporalPlot = 0;
  this->AnimationCueProxy->StartCueInternal(&info);
  for (double i = starttime; i <= endtime && !this->AbortTemporalPlot; i+= 1)
    {
    info.AnimationTime = i;
    this->AnimationCueProxy->TickInternal(&info);

    double actual_time = i;
    int actual_index = (ivp)? ivp->GetElement(0) : static_cast<int>(i);
    if (tsvProperty && tsvProperty->GetNumberOfElements() > 
      static_cast<unsigned int>(actual_index))
      {
      actual_time = tsvProperty->GetElement(actual_index);
      }
    cadProperty->SetElement(0, actual_time);
    this->TemporalCacheProxy->UpdateVTKObjects();

    // This event gives GUI the chance to stop the generation
    // mid-way.
    this->InvokeEvent(vtkCommand::AnimationCueTickEvent);
    }
  this->AnimationCueProxy->EndCueInternal(&info);
  this->InGenerateTemporalPlot = 0;
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::InvalidateGeometryInternal(int useCache)
{
  if (!useCache && this->TemporalCacheProxy && !this->InGenerateTemporalPlot
    && !this->LockTemporalCache)
    {
    this->TemporalCacheProxy->GetProperty("ClearCache")->Modified();
    this->TemporalCacheProxy->UpdateVTKObjects();
    }
  this->Superclass::InvalidateGeometryInternal(useCache);
}

//-----------------------------------------------------------------------------
void vtkSMTemporalXYPlotDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LockTemporalCache: " << this->LockTemporalCache << endl;
  os << indent << "PlotPointData: " << this->PlotPointData << endl;
  os << indent << "AnimationCueProxy: " << this->AnimationCueProxy << endl;
}
