/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLODPartDisplay.h"

#include "vtkClientServerStream.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRMScalarBarWidget.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkSMPart.h"
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
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODPartDisplay);
vtkCxxRevisionMacro(vtkPVLODPartDisplay, "1.2");


//----------------------------------------------------------------------------
vtkPVLODPartDisplay::vtkPVLODPartDisplay()
{
  this->LODDeciID.ID = 0;
  this->LODMapperID.ID = 0;
  this->LODUpdateSuppressorID.ID = 0;

  this->PointLabelMapperID.ID = 0;
  this->PointLabelActorID.ID = 0;
  
  this->Information = vtkPVLODPartDisplayInformation::New();
  this->InformationIsValid = 0;
  this->LODResolution = 50;
}

//----------------------------------------------------------------------------
vtkPVLODPartDisplay::~vtkPVLODPartDisplay()
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  if ( pm )
    {
    if(this->LODMapperID.ID)
      {
      pm->DeleteStreamObject(this->LODMapperID);
      this->LODMapperID.ID = 0;
      }
    if(this->LODDeciID.ID)
      {
      pm->DeleteStreamObject(this->LODDeciID);
      this->LODDeciID.ID = 0;
      }
    if(this->LODUpdateSuppressorID.ID)
      {
      pm->DeleteStreamObject(this->LODUpdateSuppressorID);
      this->LODUpdateSuppressorID.ID = 0;
      }
    if (this->PointLabelMapperID.ID)
      {
      pm->DeleteStreamObject(this->PointLabelMapperID);
      this->PointLabelMapperID.ID = 0;
      }
    if (this->PointLabelActorID.ID)
      {
      pm->DeleteStreamObject(this->PointLabelActorID);
      this->PointLabelActorID.ID = 0;
      }
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  this->Information->Delete();
  this->Information = NULL;
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkPVLODPartDisplay::GetInformation()
{
  if (this->InformationIsValid)
    {
    return this->Information;
    }
  if ( ! this->GetProcessModule() )
    {
    return 0;
    }
  this->GetProcessModule()->GatherInformation(
                 this->Information, this->LODDeciID);

  this->InformationIsValid = 1;

  return this->Information;  
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  this->Superclass::CreateParallelTclObjects(pm);

  // Create the decimation filter which branches the LOD pipeline.
  this->LODDeciID = pm->NewStreamObject("vtkQuadricClustering");

  // Keep track of how long each decimation filter takes to execute.
  vtkClientServerStream cmd;
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Decimate"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Decimate"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "AddObserver" << "EndEvent" << cmd
    << vtkClientServerStream::End;
  pm->GetStream() 
    << vtkClientServerStream::Invoke 
    << pm->GetProcessModuleID() << "RegisterProgressEvent"
    << this->LODDeciID << this->LODDeciID.ID
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "CopyCellDataOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "UseInputPointsOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "UseInternalTrianglesOff"
    << vtkClientServerStream::End;
  // These options reduce seams, but makes the decimation too slow.
  //pm->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pm->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  // This should be changed to origin and spacing determined globally.
  int res[3] = {this->LODResolution,this->LODResolution,this->LODResolution};
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "SetNumberOfDivisions"
    << vtkClientServerStream::InsertArray(res, 3)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);


  // ===== LOD branch:
  this->LODUpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  pm->GetStream()
    << vtkClientServerStream::Invoke << this->LODDeciID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke << this->LODUpdateSuppressorID
    << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // ===== LOD branch:
  this->LODMapperID = pm->NewStreamObject("vtkPolyDataMapper");
  pm->GetStream()
    << vtkClientServerStream::Invoke 
    << this->LODMapperID << "InterpolateScalarsBeforeMappingOn" 
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "UseLookupTableScalarRangeOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "GetPolyDataOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetImmediateModeRendering"
    << pm->GetUseImmediateMode()
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->PropID << "SetLODMapper" << this->LODMapperID
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  // Broadcast for subclasses.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  if ( pm->GetNumberOfPartitions() == 1 && !pm->GetOptions()->GetClientMode() )
    {
    this->PointLabelMapperID = pm->NewStreamObject("vtkLabeledDataMapper");
    this->PointLabelActorID = pm->NewStreamObject("vtkActor2D");
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->PointLabelActorID << "SetMapper" << this->PointLabelMapperID
      << vtkClientServerStream::End;
    // Sending to client is enough here since this works only for single
    // node.
    pm->SendStream(vtkProcessModule::CLIENT);
    }

  // Now that geometry in in this object, we must
  // connect LOD filter to geometry as part of initialization.
  pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryID
    << "GetOutput" << vtkClientServerStream::End;
  pm->GetStream() 
    << vtkClientServerStream::Invoke << this->LODDeciID << "SetInput" 
    << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::ColorByArray(vtkRMScalarBarWidget *colorMap,
                                       int field)
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  // Turn off the specular so it does not interfere with data.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->PropertyID << "SetSpecular" << 0.0
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "SetLookupTable" << colorMap->GetLookupTableID()
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "ScalarVisibilityOn"
    << vtkClientServerStream::End;

  if (field == VTK_CELL_DATA_FIELD)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->MapperID << "SetScalarModeToUseCellFieldData"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperID << "SetScalarModeToUseCellFieldData"
      << vtkClientServerStream::End;
    }
  else if (field == VTK_POINT_DATA_FIELD)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->MapperID << "SetScalarModeToUsePointFieldData"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperID << "SetScalarModeToUsePointFieldData"
      << vtkClientServerStream::End;
    }
  else
    {
    vtkErrorMacro("Only point or cell field please.");
    }

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "SelectColorArray" << colorMap->GetArrayName()
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetLookupTable" << colorMap->GetLookupTableID()
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "ScalarVisibilityOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SelectColorArray" << colorMap->GetArrayName()
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetScalarVisibility(int val)
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "SetScalarVisibility" << val
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetScalarVisibility" << val
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  this->Superclass::SetUseImmediateMode(val);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetImmediateModeRendering" << val
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetLODResolution(int res)
{
  if (res == this->LODResolution)
    {
    return;
    }
  this->LODResolution = res;

  vtkPVProcessModule* pm = this->GetProcessModule();
  int r[3] = {res, res, res};
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "SetNumberOfDivisions" 
    << vtkClientServerStream::InsertArray(r, 3)
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
  this->InvalidateGeometry();
}



//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::Update()
{
  vtkPVProcessModule* pm = this->GetProcessModule();

  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID )
    {
    this->InformationIsValid = 0;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    this->SendForceUpdate();
    this->GeometryIsValid = 1;
    }
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::RemoveAllCaches()
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "RemoveAllCaches"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "RemoveAllCaches"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVLODPartDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "CacheUpdate" << idx << total
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "CacheUpdate" << idx << total
    << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "Modified"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "Modified"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetDirectColorFlag(int val)
{
  if (val)
    {
    val = 1;
    }
  if (val == this->DirectColorFlag)
    {
    return;
    }

  vtkPVProcessModule* pm = this->GetProcessModule();
  this->DirectColorFlag = val;
  if (val)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->MapperID << "SetColorModeToDefault"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperID << "SetColorModeToDefault"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->MapperID << "SetColorModeToMapScalars"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODMapperID << "SetColorModeToMapScalars"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetInterpolateColorsFlag(int val)
{
  if (val)
    {
    val = 1;
    }
  if (val == this->InterpolateColorsFlag)
    {
    return;
    }

  vtkPVProcessModule* pm = this->GetProcessModule();
  this->InterpolateColorsFlag = val;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID
    << "SetInterpolateScalarsBeforeMapping" << (!val) 
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID 
    << "SetInterpolateScalarsBeforeMapping" << (!val) 
    << vtkClientServerStream::End;
pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetPointLabelVisibility(int val)
{
  if (!this->PointLabelMapperID.ID || !this->PointLabelActorID.ID)
    {
    return;
    }
    
  vtkPVProcessModule *pm = this->GetProcessModule();

  vtkPVDataInformation* di = this->GetPart()->GetDataInformation();
  if (di->DataSetTypeIsA("vtkDataSet"))
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->PointLabelMapperID << "SetInput"
      << this->Part->GetID(0)
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke << this->PointLabelMapperID
      << "GetLabelTextProperty" << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult << "SetFontSize" << 24
      << vtkClientServerStream::End;
    
    if (val)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << pm->GetRenderModule()->GetRendererID() << "AddProp"
        << this->PointLabelActorID << vtkClientServerStream::End;
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << pm->GetRenderModule()->GetRendererID() << "RemoveProp"
        << this->PointLabelActorID << vtkClientServerStream::End;
      }
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODMapperID: " << this->LODMapperID.ID << endl;
  os << indent << "LODDeciID: " << this->LODDeciID.ID << endl;

  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "LODUpdateSuppressor: " << this->LODUpdateSuppressorID.ID
     << endl;
}
