/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLODPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLODPartDisplay.h"

#include "vtkClientServerStream.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkPolyData.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkQuadricClustering.h"
#include "vtkRectilinearGrid.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLODPartDisplay);
vtkCxxRevisionMacro(vtkSMLODPartDisplay, "1.9");


//----------------------------------------------------------------------------
vtkSMLODPartDisplay::vtkSMLODPartDisplay()
{
  this->LODDeciProxy = 0;
  this->LODMapperProxy = 0;
  this->LODUpdateSuppressorProxy = 0;
  this->LODVolumeMapperProxy = 0;

  this->LODInformationIsValid = 0;
  this->LODInformation = vtkPVLODPartDisplayInformation::New();
  this->LODResolution = 50;

  this->LODGeometryIsValid = 0;
}

//----------------------------------------------------------------------------
vtkSMLODPartDisplay::~vtkSMLODPartDisplay()
{
  if(this->LODMapperProxy)
    {
    this->LODMapperProxy->Delete();
    this->LODMapperProxy = 0;
    }
  if(this->LODDeciProxy)
    {
    this->LODDeciProxy->Delete();
    this->LODDeciProxy = 0;
    }
  if(this->LODUpdateSuppressorProxy)
    {
    this->LODUpdateSuppressorProxy->Delete();
    this->LODUpdateSuppressorProxy = 0;
    }
  if (this->LODVolumeMapperProxy)
    {
    this->LODVolumeMapperProxy->Delete();
    this->LODVolumeMapperProxy = 0;
    }

  this->LODInformation->Delete();
  this->LODInformation = NULL;
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkSMLODPartDisplay::GetLODInformation()
{
  if (this->LODInformationIsValid)
    {
    return this->LODInformation;
    }
  if ( ! this->GetProcessModule() )
    {
    return 0;
    }
  // Fixme: loop over IDs
  this->LODInformation->CopyFromObject(0); // Clear information.
  if (this->LODDeciProxy->GetNumberOfIDs() > 0)
    {
    this->GetProcessModule()->GatherInformation(
                   this->LODInformation, this->LODDeciProxy->GetID(0));
    }
  this->LODInformationIsValid = 1;

  return this->LODInformation;  
}


//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::CreateVTKObjects(int num)
{
  int i;
  // hack until input uses proxy input.
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  this->Superclass::CreateVTKObjects(num);
  
  // Create the decimation filter which branches the LOD pipeline.
  this->LODDeciProxy = vtkSMProxy::New();
  this->LODDeciProxy->SetVTKClassName("vtkQuadricClustering");
  this->LODDeciProxy->SetServersSelf(vtkProcessModule::DATA_SERVER);

  // ===== LOD branch:
  this->LODUpdateSuppressorProxy = vtkSMProxy::New();
  this->LODUpdateSuppressorProxy->SetVTKClassName("vtkPVUpdateSuppressor");
  this->LODUpdateSuppressorProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  // ===== LOD branch:
  this->LODMapperProxy = vtkSMProxy::New();
  this->LODMapperProxy->SetVTKClassName("vtkPolyDataMapper");
  this->LODMapperProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->LODMapperProxy->AddProperty("DirectColorFlag", this->DirectColorFlagProperty);
  this->LODMapperProxy->AddProperty("InterpolateColorsFlag", this->InterpolateColorsFlagProperty);

  this->LODDeciProxy->CreateVTKObjects(num);
  this->LODUpdateSuppressorProxy->CreateVTKObjects(num);
  this->LODMapperProxy->CreateVTKObjects(num);

  // ===== Volume rendering LOD branch (use a surface for low LOD):
  this->LODVolumeMapperProxy = vtkSMProxy::New();
  this->LODVolumeMapperProxy->SetVTKClassName("vtkPolyDataMapper");
  this->LODVolumeMapperProxy->SetServersSelf(  vtkProcessModule::CLIENT
                                             | vtkProcessModule::RENDER_SERVER);
  this->LODVolumeMapperProxy->AddProperty("InterpolateColorsFlag",
                                          this->InterpolateColorsFlagProperty);

  this->LODVolumeMapperProxy->CreateVTKObjects(num);

  for (i = 0; i < num; ++i)
    {
    // Keep track of how long each decimation filter takes to execute.
    vtkClientServerStream cmd;
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Decimate"
        << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Decimate"
        << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << pm->GetProcessModuleID() << "RegisterProgressEvent"
      << this->LODDeciProxy->GetID(i) << this->LODDeciProxy->GetID(i).ID
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "CopyCellDataOn"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "UseInputPointsOn"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "UseInternalTrianglesOff"
      << vtkClientServerStream::End;
    // These options reduce seams, but makes the decimation too slow.
    //pm->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
    //pm->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
    // This should be changed to origin and spacing determined globally.
    int res[3] = {this->LODResolution,this->LODResolution,this->LODResolution};
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "SetNumberOfDivisions"
      << vtkClientServerStream::InsertArray(res, 3)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);

    pm->GetStream()
      << vtkClientServerStream::Invoke << this->LODDeciProxy->GetID(i) << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke << this->LODUpdateSuppressorProxy->GetID(i)
      << "SetInput" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);

    pm->GetStream()
      << vtkClientServerStream::Invoke 
      << this->LODMapperProxy->GetID(i) << "InterpolateScalarsBeforeMappingOn" 
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperProxy->GetID(i) << "UseLookupTableScalarRangeOn"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "GetPolyDataOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODMapperProxy->GetID(i) << "SetInput" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperProxy->GetID(i) << "SetImmediateModeRendering"
      << pm->GetUseImmediateMode()
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->PropProxy->GetID(i) << "SetLODMapper" << this->LODMapperProxy->GetID(i)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

    pm->GetStream()
      << vtkClientServerStream::Invoke 
      << this->LODVolumeMapperProxy->GetID(i)
      << "InterpolateScalarsBeforeMappingOn" 
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODVolumeMapperProxy->GetID(i) << "UseLookupTableScalarRangeOn"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "GetPolyDataOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODVolumeMapperProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODVolumeMapperProxy->GetID(i) << "SetImmediateModeRendering"
      << pm->GetUseImmediateMode()
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->VolumeProxy->GetID(i) << "SetLODMapper"
      << this->LODVolumeMapperProxy->GetID(i)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

    // Broadcast for subclasses.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

    // Now that geometry in in this object, we must
    // connect LOD filter to geometry as part of initialization.
    pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryProxy->GetID(i)
      << "GetOutput" << vtkClientServerStream::End;
    pm->GetStream() 
      << vtkClientServerStream::Invoke << this->LODDeciProxy->GetID(i) << "SetInput" 
      << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::ColorByArray(vtkSMProxy* colorMap,
                                       int field)
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  this->SetScalarVisibility(1);

  this->ColorField = field;
  if (this->ColorMap)
    {
    this->ColorMap->Delete();
    this->ColorMap = NULL;
    }
  this->ColorMap = colorMap;
  if (this->ColorMap)
    {
    this->ColorMap->Register(this);
    }

  int num, i;
  num = this->MapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Turn off the specular so it does not interfere with data.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->PropertyProxy->GetID(i) << "SetSpecular" << 0.0
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->MapperProxy->GetID(i) << "SetLookupTable" 
      << colorMap->GetID(0)
      << vtkClientServerStream::End;
    if (field == vtkDataSet::CELL_DATA_FIELD)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->MapperProxy->GetID(i) << "SetScalarModeToUseCellFieldData"
        << vtkClientServerStream::End;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->LODMapperProxy->GetID(i) << "SetScalarModeToUseCellFieldData"
        << vtkClientServerStream::End;
      }
    else if (field == vtkDataSet::POINT_DATA_FIELD)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->MapperProxy->GetID(i) << "SetScalarModeToUsePointFieldData"
        << vtkClientServerStream::End;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->LODMapperProxy->GetID(i) << "SetScalarModeToUsePointFieldData"
        << vtkClientServerStream::End;
      }
    else
      {
      vtkErrorMacro("Only point or cell field please.");
      }
    
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      colorMap->GetProperty("ArrayName"));
    if (svp)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->MapperProxy->GetID(i) << "SelectColorArray" << svp->GetElement(0)
        << vtkClientServerStream::End;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->LODMapperProxy->GetID(i) << "SetLookupTable" 
        << colorMap->GetID(0)
        << vtkClientServerStream::End;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->LODMapperProxy->GetID(i) << "SelectColorArray" << svp->GetElement(0)
        << vtkClientServerStream::End;
      }
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}


//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::SetScalarVisibility(int val)
{
  this->ScalarVisibilityProperty->SetElement(0, val);
  if (this->MapperProxy)
    {
    this->MapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  this->Superclass::SetUseImmediateMode(val);
  if (this->LODMapperProxy)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperProxy->GetID(0) << "SetImmediateModeRendering" << val
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  if (this->LODVolumeMapperProxy)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODVolumeMapperProxy->GetID(0) << "SetImmediateModeRendering"
      << val << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }
}

//-----------------------------------------------------------------------------
void vtkSMLODPartDisplay::VolumeRenderPointField(const char *name)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  this->Superclass::VolumeRenderPointField(name);

  vtkClientServerStream &stream = pm->GetStream();

  int i, num;
  num = this->LODVolumeMapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->LODVolumeMapperProxy->GetID(i)
           << "SetScalarModeToUsePointFieldData"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODVolumeMapperProxy->GetID(i)
           << "SelectColorArray" << name << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

//-----------------------------------------------------------------------------
void vtkSMLODPartDisplay::VolumeRenderCellField(const char *name)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  this->Superclass::VolumeRenderCellField(name);

  vtkClientServerStream &stream = pm->GetStream();

  int i, num;
  num = this->LODVolumeMapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->LODVolumeMapperProxy->GetID(i)
           << "SetScalarModeToUseCellFieldData"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODVolumeMapperProxy->GetID(i)
           << "SelectColorArray" << name << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::SetLODResolution(int res)
{
  if (res == this->LODResolution)
    {
    return;
    }
  this->LODResolution = res;

  vtkPVProcessModule* pm = this->GetProcessModule();
  if (this->LODDeciProxy)
    {
    int r[3] = {res, res, res};
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(0) << "SetNumberOfDivisions" 
      << vtkClientServerStream::InsertArray(r, 3)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  this->InvalidateGeometry();
}



//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::Update()
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  if ( ! this->GeometryIsValid && this->UpdateSuppressorProxy )
    {
    int i, num;
    num = this->UpdateSuppressorProxy->GetNumberOfIDs();
    for (i = 0; i < num; ++i)
      {
      this->GeometryInformationIsValid = 0;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      }
    this->SendForceUpdate();
    this->GeometryIsValid = 1;
    }

  // Try delaying the update of the LOD path if it is not necessary.
  if ( ! this->LODGeometryIsValid && pm->GetGlobalLODFlag() && 
       this->LODUpdateSuppressorProxy )
    {
    int i, num;
    num = this->UpdateSuppressorProxy->GetNumberOfIDs();
    for (i = 0; i < num; ++i)
      {
      this->LODInformationIsValid = 0;
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      }
    this->SendForceUpdate();
    this->LODGeometryIsValid = 1;
    }
}


//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::InvalidateGeometry()
{
  this->LODGeometryIsValid = 0;
  this->GeometryIsValid = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::RemoveAllCaches()
{
  if (this->UpdateSuppressorProxy)
    {
    vtkPVProcessModule* pm = this->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(0) << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(0) << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMLODPartDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID(0) << "CacheUpdate" << idx << total
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorProxy->GetID(0) << "CacheUpdate" << idx << total
    << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperProxy->GetID(0) << "Modified"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperProxy->GetID(0) << "Modified"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::SetDirectColorFlag(int val)
{
  this->Superclass::SetDirectColorFlag(val);
  if (this->LODMapperProxy)
    {
    this->LODMapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::SetInterpolateColorsFlag(int val)
{
  this->Superclass::SetInterpolateColorsFlag(val);
  if (this->LODMapperProxy)
    {
    this->LODMapperProxy->UpdateVTKObjects();
    }
  if (this->LODVolumeMapperProxy)
    {
    this->LODVolumeMapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMLODPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODMapperProxy: " << this->LODMapperProxy << endl;
  os << indent << "LODDeciProxy: " << this->LODDeciProxy << endl;

  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "LODUpdateSuppressorProxy: " << this->LODUpdateSuppressorProxy
     << endl;
}
