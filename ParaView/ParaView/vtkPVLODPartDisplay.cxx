/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplay.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.
 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVLODPartDisplay.h"

#include "vtkPVLODPartDisplayInformation.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkQuadricClustering.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkKWCheckButton.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkPVColorMap.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODPartDisplay);
vtkCxxRevisionMacro(vtkPVLODPartDisplay, "1.9");


//----------------------------------------------------------------------------
vtkPVLODPartDisplay::vtkPVLODPartDisplay()
{
  this->LODDeciID.ID = 0;
  this->LODMapperID.ID = 0;
  this->LODUpdateSuppressorID.ID = 0;

  this->Information = vtkPVLODPartDisplayInformation::New();
  this->InformationIsValid = 0;
  this->LODResolution = 50;
}

//----------------------------------------------------------------------------
vtkPVLODPartDisplay::~vtkPVLODPartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
    if ( pvApp )
      {
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
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
    pm->SendStreamToClientAndServer();
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
  if ( !this->GetPVApplication() || !this->GetPVApplication()->GetProcessModule() )
    {
    return 0;
    }
  this->GetPVApplication()->GetProcessModule()->GatherInformation(
                 this->Information, this->LODDeciID);

  this->InformationIsValid = 1;

  return this->Information;  
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::ConnectToGeometry(vtkClientServerID geometryID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  // Superclass connects the full res pipeline.
  this->Superclass::ConnectToGeometry(geometryID);
  
  stream << vtkClientServerStream::Invoke << geometryID
         << "GetOutput" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->LODDeciID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->Superclass::CreateParallelTclObjects(pvApp);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  // Create the decimation filter which branches the LOD pipeline.
  this->LODDeciID = pm->NewStreamObject("vtkQuadricClustering");

  // Keep track of how long each decimation filter takes to execute.
  vtkClientServerStream cmd;
  cmd << vtkClientServerStream::Invoke
      << pm->GetApplicationID() << "LogStartEvent" << "Execute Decimate"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetApplicationID() << "LogEndEvent" << "Execute Decimate"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "AddObserver" << "EndEvent" << cmd
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
  //pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  // This should be changed to origin and spacing determined globally.
  int res[3] = {this->LODResolution,this->LODResolution,this->LODResolution};
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "SetNumberOfDivisions"
    << vtkClientServerStream::InsertArray(res, 3)
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();


  // ===== LOD branch:
  this->LODUpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  pm->GetStream()
    << vtkClientServerStream::Invoke << this->LODDeciID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke << this->LODUpdateSuppressorID
    << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

  // ===== LOD branch:
  this->LODMapperID = pm->NewStreamObject("vtkPolyDataMapper");
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "UseLookupTableScalarRangeOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetImmediateModeRendering"
    << pvApp->GetMainView()->GetImmediateModeCheck()->GetState()
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->PropID << "SetLODMapper" << this->LODMapperID
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

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
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::ColorByArray(vtkPVColorMap *colorMap,
                                       int field)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

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
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetScalarVisibility(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->MapperID << "SetScalarVisibility" << val
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetScalarVisibility" << val
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  this->Superclass::SetUseImmediateMode(val);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODMapperID << "SetImmediateModeRendering" << val
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetLODResolution(int res)
{
  if (res == this->LODResolution)
    {
    return;
    }
  this->LODResolution = res;

  vtkPVApplication* pvApp = this->GetPVApplication();
  if (pvApp)
    {
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    int r[3] = {res, res, res};
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODDeciID << "SetNumberOfDivisions" 
      << vtkClientServerStream::InsertArray(r, 3)
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
  this->InvalidateGeometry();
}



//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::Update()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

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
    pm->SendStreamToClientAndServer();
    this->GeometryIsValid = 1;
    }
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::RemoveAllCaches()
{
  if (this->PVApplication)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVLODPartDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "CacheUpdate" << idx << total
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "CacheUpdate" << idx << total
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
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

  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
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
    pm->SendStreamToClientAndServer();
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
    pm->SendStreamToClientAndServer();
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
