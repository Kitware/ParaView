/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositePartDisplay.cxx
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
#include "vtkPVCompositePartDisplay.h"

#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositePartDisplay);
vtkCxxRevisionMacro(vtkPVCompositePartDisplay, "1.1");


//----------------------------------------------------------------------------
vtkPVCompositePartDisplay::vtkPVCompositePartDisplay()
{
  this->CollectionDecision = 1;
  this->LODCollectionDecision = 1;

  this->CollectTclName = NULL;
  this->LODCollectTclName = NULL;
}

//----------------------------------------------------------------------------
vtkPVCompositePartDisplay::~vtkPVCompositePartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if (this->CollectTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->CollectTclName);
      }
    this->SetCollectTclName(NULL);
    }
  if (this->LODCollectTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODCollectTclName);
      }
    this->SetLODCollectTclName(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::ConnectToGeometry(char* geometryTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // The input of course is the geometry filter.
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODDeciTclName, geometryTclName);

  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->CollectTclName, geometryTclName);
}


//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  char tclName[100];
  
  this->Superclass::CreateParallelTclObjects(pvApp);
  
  // Create the collection filters which allow small models to render locally.  
  // They also redistributed data for SGI pipes option.
  // ===== Primary branch:
  sprintf(tclName, "Collect%d", this->InstanceCount);

  // Different filter for  pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    pvApp->BroadcastScript("vtkAllToNRedistributePolyData %s", tclName);
    pvApp->BroadcastScript("%s SetNumberOfProcesses %d", tclName,
                           pvApp->GetNumberOfPipes());
    }
  else if (pvApp->GetUseTiledDisplay())
    {
    int numProcs = pvApp->GetController()->GetNumberOfProcesses();
    int* dims = pvApp->GetTileDimensions();
    pvApp->BroadcastScript("vtkPVDuplicatePolyData %s", tclName);
    pvApp->BroadcastScript("%s InitializeSchedule %d %d", tclName,
                           numProcs, dims[0]*dims[1]);
    // Initialize collection descision here. (When we have rendering module).
    }
  else
    {
    pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
    }
  this->SetCollectTclName(tclName);
  pvApp->BroadcastScript("%s AddObserver StartEvent {$Application LogStartEvent {Execute Collect}}", 
                         this->CollectTclName);
  pvApp->BroadcastScript("%s AddObserver EndEvent {$Application LogEndEvent {Execute Collect}}", 
                         this->CollectTclName);
  //
  // ===== LOD branch:
  sprintf(tclName, "LODCollect%d", this->InstanceCount);
  // Different filter for pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    pvApp->BroadcastScript("vtkAllToNRedistributePolyData %s", tclName);
    pvApp->BroadcastScript("%s SetNumberOfProcesses %d", tclName,
                           pvApp->GetNumberOfPipes());
    }
  else if (pvApp->GetUseTiledDisplay())
    {
    int numProcs = pvApp->GetController()->GetNumberOfProcesses();
    int* dims = pvApp->GetTileDimensions();
    pvApp->BroadcastScript("vtkPVDuplicatePolyData %s", tclName);
    pvApp->BroadcastScript("%s InitializeSchedule %d %d", tclName,
                           numProcs, dims[0]*dims[1]);
    // Initialize collection descision here. (When we have rendering module).
    }
  else
    {
    pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
    }
  this->SetLODCollectTclName(tclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODCollectTclName, this->LODDeciTclName);
  pvApp->BroadcastScript("%s AddObserver StartEvent {$Application LogStartEvent {Execute LODCollect}}", 
                         this->LODCollectTclName);
  pvApp->BroadcastScript("%s AddObserver EndEvent {$Application LogEndEvent {Execute LODCollect}}", 
                         this->LODCollectTclName);
   // Handle collection setup with client server.
  pvApp->BroadcastScript("%s SetSocketController [ $Application GetSocketController ] ", 
                         this->CollectTclName);
  pvApp->BroadcastScript("%s SetSocketController [ $Application GetSocketController ] ", 
                         this->LODCollectTclName);
  // Special condition to signal the client.
  // Because both processes of the Socket controller think they are 0!!!!
  if (pvApp->GetClientMode())
    {
    pvApp->Script("%s SetController {}", this->CollectTclName);
    pvApp->Script("%s SetController {}", this->LODCollectTclName);
    }

  // Insert collection filters into pipeline.
  if (this->CollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->UpdateSuppressorTclName, 
                           this->CollectTclName);
    }

  if (this->LODCollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->LODUpdateSuppressorTclName, 
                           this->LODCollectTclName);
    }
}



//-----------------------------------------------------------------------------
// Updates if necessary.
vtkPVLODPartDisplayInformation* vtkPVCompositePartDisplay::GetInformation()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    vtkErrorMacro("Missing application.");
    return NULL;
    }

  if ( ! this->GeometryIsValid)
    { // Update but with collection filter off.
    this->CollectionDecision = 0;
    pvApp->BroadcastScript("%s SetPassThrough 1; %s ForceUpdate; " 
                           "%s SetPassThrough 1; %s ForceUpdate", 
                           this->CollectTclName,
                           this->UpdateSuppressorTclName,
                           this->LODCollectTclName,
                           this->LODUpdateSuppressorTclName);
    this->InformationIsValid = 0;
    }

  return this->Superclass::GetInformation();
}


//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::SetCollectionDecision(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (v == this->CollectionDecision)
    {
    return;
    }
  this->CollectionDecision = v;

  if ( this->UpdateSuppressorTclName == NULL )
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }

  if (this->CollectTclName)
    {
    if (this->CollectionDecision)
      {
      pvApp->BroadcastScript("%s SetPassThrough 0; %s RemoveAllCaches; %s ForceUpdate", 
                             this->CollectTclName,
                             this->UpdateSuppressorTclName,
                             this->UpdateSuppressorTclName);
      }
    else
      {
      pvApp->BroadcastScript("%s SetPassThrough 1; %s RemoveAllCaches; %s ForceUpdate", 
                             this->CollectTclName,
                             this->UpdateSuppressorTclName,
                             this->UpdateSuppressorTclName);
      }
    }

  this->GetPVApplication()->BroadcastScript(
             "%s RemoveAllCaches", this->UpdateSuppressorTclName);
}

//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::SetLODCollectionDecision(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (v == this->LODCollectionDecision)
    {
    return;
    }
  this->LODCollectionDecision = v;

  if ( this->UpdateSuppressorTclName == NULL )
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }

  if (this->LODCollectTclName)
    {
    if (this->LODCollectionDecision)
      {
      pvApp->BroadcastScript("%s SetPassThrough 0; %s RemoveAllCaches; %s ForceUpdate", 
                             this->LODCollectTclName,
                             this->LODUpdateSuppressorTclName,
                             this->LODUpdateSuppressorTclName);
      }
    else
      {
      pvApp->BroadcastScript("%s SetPassThrough 1; %s RemoveAllCaches; %s ForceUpdate", 
                             this->LODCollectTclName,
                             this->LODUpdateSuppressorTclName,
                             this->LODUpdateSuppressorTclName);
      }
    }

}




//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CollectTclName: " << (this->CollectTclName?this->CollectTclName:"none") << endl;
  os << indent << "LODCollectTclName: " << (this->LODCollectTclName?this->LODCollectTclName:"none") << endl;

  os << indent << "CollectionDecision: " 
     <<  this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " 
     <<  this->LODCollectionDecision << endl;

}


  



