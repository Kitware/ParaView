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


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODPartDisplay);
vtkCxxRevisionMacro(vtkPVLODPartDisplay, "1.6.2.2");


//----------------------------------------------------------------------------
vtkPVLODPartDisplay::vtkPVLODPartDisplay()
{
  this->LODDeciTclName = NULL;
  this->LODDeciTclName = NULL;
  this->LODMapperTclName = NULL;
  this->LODUpdateSuppressorTclName = NULL;

  this->Information = vtkPVLODPartDisplayInformation::New();
  this->InformationIsValid = 0;
  this->LODResolution = 50;
}

//----------------------------------------------------------------------------
vtkPVLODPartDisplay::~vtkPVLODPartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if ( pvApp && this->LODMapperTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->LODMapperTclName);
    }
  this->SetLODMapperTclName(NULL);
  
  if (this->LODDeciTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODDeciTclName);
      }
    this->SetLODDeciTclName(NULL);
    }

  if (this->LODUpdateSuppressorTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODUpdateSuppressorTclName);
      }
    this->SetLODUpdateSuppressorTclName(NULL);
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
                 this->Information, this->LODDeciTclName);

  this->InformationIsValid = 1;

  return this->Information;  
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::ConnectToGeometry(vtkClientServerID geometryID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  
  stream << vtkClientServerStream::Invoke << geometryID
         << "GetOutput" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->LODDeciID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  char tclName[100];
    
  this->Superclass::CreateParallelTclObjects(pvApp);

  // Create the decimation filter which branches the LOD pipeline.
  sprintf(tclName, "LODDeci%d", this->InstanceCount);
  this->LODDeciTclName = NULL;
  this->SetLODDeciTclName(tclName);
  pvApp->BroadcastScript("vtkQuadricClustering %s", tclName);
  // Keep track of how long each decimation filter takes to execute.
  pvApp->BroadcastScript("%s AddObserver StartEvent {$Application LogStartEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  pvApp->BroadcastScript("%s AddObserver EndEvent {$Application LogEndEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  pvApp->BroadcastScript("%s CopyCellDataOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInputPointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInternalTrianglesOff", this->LODDeciTclName);
  // These options reduce seams, but makes the decimation too slow.
  //pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  // This should be changed to origin and spacing determined globally.

  pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                         this->LODDeciTclName, this->LODResolution, 
                         this->LODResolution, this->LODResolution);


  // ===== LOD branch:
  sprintf(tclName, "LODUpdateSuppressor%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVUpdateSuppressor %s", tclName);
  this->SetLODUpdateSuppressorTclName(tclName);
  if (this->LODDeciTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->LODUpdateSuppressorTclName, 
                           this->LODDeciTclName);
    }

  // ===== LOD branch:
  sprintf(tclName, "LODMapper%d", this->InstanceCount);
  pvApp->MakeTclObject("vtkPolyDataMapper", tclName);
  this->LODMapperTclName = NULL;
  this->SetLODMapperTclName(tclName);
  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->LODMapperTclName);
  //pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->LODMapperTclName,
                         this->LODUpdateSuppressorTclName);
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d", this->LODMapperTclName,
                  pvApp->GetMainView()->GetImmediateModeCheck()->GetState());
 
  pvApp->BroadcastScript("%s SetLODMapper %s", this->PropTclName,
                         this->LODMapperTclName);
  
  // Broadcast for subclasses.  
  pvApp->BroadcastScript("%s SetUpdateNumberOfPieces [[$Application GetProcessModule] GetNumberOfPartitions]",
                        this->LODUpdateSuppressorTclName);
  pvApp->BroadcastScript("%s SetUpdatePiece [[$Application GetProcessModule] GetPartitionId]",
                        this->LODUpdateSuppressorTclName);
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::ColorByArray(vtkPVColorMap *colorMap,
                                       int field)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Turn off the specualr so it does not interfere with data.
  pvApp->BroadcastScript("%s SetSpecular 0.0", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetLookupTable %s", 
                         this->MapperTclName,
                         colorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", 
                         this->MapperTclName);
  if (field == VTK_CELL_DATA_FIELD)
    {
    pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                           this->MapperTclName);
    pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                           this->LODMapperTclName);
    }
  else if (field == VTK_POINT_DATA_FIELD)
    {
    pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                           this->MapperTclName);
    pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                           this->LODMapperTclName);
    }
  else
    {
    vtkErrorMacro("Only point or cell field please.");
    }

  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->MapperTclName, colorMap->GetArrayName());

  pvApp->BroadcastScript("%s SetLookupTable %s", 
                         this->LODMapperTclName,
                         colorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", 
                         this->LODMapperTclName);
  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->LODMapperTclName, colorMap->GetArrayName());
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetScalarVisibility(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetScalarVisibility %d", 
                         this->MapperTclName, val);
  pvApp->BroadcastScript("%s SetScalarVisibility %d", 
                         this->LODMapperTclName, val);
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->Superclass::SetUseImmediateMode(val);
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                         this->GetLODMapperTclName(),
                         val);
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
    pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                           this->LODDeciTclName, res, res, res);
    }
  this->InvalidateGeometry();
}



//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::Update()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if ( ! this->GeometryIsValid && this->UpdateSuppressorTclName )
    {
    this->InformationIsValid = 0;
    pvApp->BroadcastScript("%s ForceUpdate", this->UpdateSuppressorTclName);
    pvApp->BroadcastScript("%s ForceUpdate", this->LODUpdateSuppressorTclName);
    this->GeometryIsValid = 1;
    }
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::RemoveAllCaches()
{
  if (this->PVApplication)
    {
    this->GetPVApplication()->BroadcastScript(
             "%s RemoveAllCaches; %s RemoveAllCaches",
             this->UpdateSuppressorTclName, this->LODUpdateSuppressorTclName);
    }
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVLODPartDisplay::CacheUpdate(int idx, int total)
{
  this->GetPVApplication()->BroadcastScript(
             "%s CacheUpdate %d %d; %s CacheUpdate %d %d",
             this->UpdateSuppressorTclName, idx, total,
             this->LODUpdateSuppressorTclName, idx, total);
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
  this->DirectColorFlag = val;
  if (val)
    {
    pvApp->BroadcastScript("%s SetColorModeToDefault", this->MapperTclName);
    pvApp->BroadcastScript("%s SetColorModeToDefault", this->LODMapperTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->MapperTclName);
    pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->LODMapperTclName);
    }
}


//----------------------------------------------------------------------------
void vtkPVLODPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODMapperTclName: " << (this->LODMapperTclName?this->LODMapperTclName:"none") << endl;
  os << indent << "LODDeciTclName: " << (this->LODDeciTclName?this->LODDeciTclName:"none") << endl;

  os << indent << "LODResolution: " << this->LODResolution << endl;

  if (this->LODUpdateSuppressorTclName)
    {
    os << indent << "LODUpdateSuppressor: " << this->LODUpdateSuppressorTclName << endl;
    }
}


  



