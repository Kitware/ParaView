/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPart.cxx
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
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVRenderModule.h"

#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVConfig.h"
#include "vtkPVRenderView.h"
#include "vtkKWCheckButton.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPart);
vtkCxxRevisionMacro(vtkPVPart, "1.25.2.12");


int vtkPVPartCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVPart::vtkPVPart()
{
  this->CommandFunction = vtkPVPartCommand;

  this->PartDisplay = NULL;

  this->Name = NULL;

  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;
  this->VTKDataTclName = NULL;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->GeometryTclName = NULL;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;

  this->VTKSourceIndex = -1;
  this->VTKOutputIndex = -1;
}

//----------------------------------------------------------------------------
vtkPVPart::~vtkPVPart()
{  
  this->SetPartDisplay(NULL);

  // Get rid of the circular reference created by the extent translator.
  // We have a problem with ExtractPolyDataPiece also.
  if (this->VTKDataTclName)
    {
    this->GetPVApplication()->GetProcessModule()->
      ServerScript("%s SetExtentTranslator {}", this->VTKDataTclName);
    delete [] this->VTKDataTclName;
    this->VTKDataTclName = NULL;
    }  
    
  this->SetName(NULL);

  this->DataInformation->Delete();
  this->DataInformation = NULL;

  // Used to be in vtkPVActorComposite........
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if (this->GeometryTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->GeometryTclName);
      }
    this->SetGeometryTclName(NULL);
    }
}


//----------------------------------------------------------------------------
void vtkPVPart::SetPVApplication(vtkPVApplication *pvApp)
{
  this->CreateParallelTclObjects(pvApp);
  this->vtkKWObject::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
void vtkPVPart::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  char tclName[100];
  
  this->vtkKWObject::SetApplication(pvApp);
  
  // Create the one geometry filter,
  // which creates the poly data from all data sets.
  sprintf(tclName, "Geometry%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVGeometryFilter %s", tclName);
  this->SetGeometryTclName(tclName);

  // Default setting.
  pvApp->BroadcastScript("%s SetUseStrips %d", tclName,
    pvApp->GetMainView()->GetTriangleStripsCheck()->GetState());

  // Keep track of how long each geometry filter takes to execute.
  pvApp->BroadcastScript("%s AddObserver StartEvent {$Application LogStartEvent "
                         "{Execute Geometry}}", this->GeometryTclName);
  pvApp->BroadcastScript("%s AddObserver EndEvent {$Application LogEndEvent "
                         "{Execute Geometry}}", this->GeometryTclName);

}

//----------------------------------------------------------------------------
void vtkPVPart::SetPartDisplay(vtkPVPartDisplay* pDisp)
{
  if (this->PartDisplay)
    {
    this->PartDisplay->UnRegister(this);
    this->PartDisplay = NULL;
    }
  if (pDisp)
    {
    this->PartDisplay = pDisp;
    this->PartDisplay->Register(this);
    // This is special (why we cannot use a macro).
    this->PartDisplay->ConnectToGeometry(this->GeometryTclName);
    }
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVPart::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkPVPart::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
void vtkPVPart::GatherDataInformation()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();

  // This does nothing if the geometry is already up to date.
  if (this->PartDisplay)
    {
    this->PartDisplay->Update();
    }

  pm->GatherInformation(this->DataInformation, 
                        this->GetVTKDataTclName());

  this->DataInformationValid = 1;

  // Recompute total visibile memory size. !!!!!!!
  // This should really be in vtkPVPartDisplay when it gathers its informantion.
  pvApp->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);

  // Look for a name defined in Field data.
  this->SetName(this->DataInformation->GetName());

  // I would like to have all sources generate names and all filters
  // Pass the field data, but until all is working well, this is a default.
  if (this->Name == NULL || this->Name[0] == '\0')
    {
    char str[100];
    if (this->DataInformation->GetDataSetType() == VTK_POLY_DATA)
      {
      sprintf(str, "Polygonal: %d cells", (int)(this->DataInformation->GetNumberOfCells()));
      }
    else if (this->DataInformation->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
      {
      sprintf(str, "Unstructured Grid: %d cells", (int)(this->DataInformation->GetNumberOfCells()));
      }
    else if (this->DataInformation->GetDataSetType() == VTK_IMAGE_DATA)
      {
      int *ext = this->DataInformation->GetExtent();
      sprintf(str, "Uniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (this->DataInformation->GetDataSetType() == VTK_RECTILINEAR_GRID)
      {
      int *ext = this->DataInformation->GetExtent();
      sprintf(str, "Nonuniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (this->DataInformation->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      int *ext = this->DataInformation->GetExtent();
      sprintf(str, "Curvilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else
      {
      sprintf(str, "Part of unknown type");
      }    
    this->SetName(str);
    }
}


//----------------------------------------------------------------------------
void vtkPVPart::SetVTKDataTclName(const char* tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("Set the application before you set the VTKDataTclName.");
    return;
    }
  
  if (this->VTKDataTclName)
    {
    delete [] this->VTKDataTclName;
    this->VTKDataTclName = NULL;
    }
  if (tclName)
    {
    this->VTKDataTclName = new char[strlen(tclName)+1];
    strcpy(this->VTKDataTclName, tclName);
    // Connect the data to the pipeline for displaying the data.
    pm->ServerScript("%s SetInput %s",
                     this->GeometryTclName,
                     this->VTKDataTclName);
    }
}



//----------------------------------------------------------------------------
void vtkPVPart::InsertExtractPiecesIfNecessary()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  // We are going to create the piece filter with a dummy tcl name,
  // setup the pipeline, and remove tcl's reference to the objects.
  // The vtkData object will be moved to the output of the piece filter.
  if((pm->RootScript("%s IsA vtkPolyData", this->VTKDataTclName),
      atoi(pm->GetRootResult())))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions() == 1)
      {
      return;
      }  
    pm->ServerScript("%s UpdateInformation", this->VTKDataTclName);
    pm->RootScript("%s GetMaximumNumberOfPieces", this->VTKDataTclName);
    if (atoi(pm->GetRootResult()) != 1)
      { // The source can already produce pieces.
      return;
      }

    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      pm->ServerSimpleScript("vtkExtractPolyDataPiece pvTemp");
      }
    else
      {
      pm->ServerSimpleScript("vtkTransmitPolyDataPiece pvTemp");
      pm->ServerSimpleScript(
        "pvTemp AddObserver StartEvent {$Application LogStartEvent {Execute TransmitPData}}");
      pm->ServerSimpleScript(
        "pvTemp AddObserver EndEvent {$Application LogEndEvent {Execute TransmitPData}}");
      }
    }
  else if((pm->RootScript("%s IsA vtkUnstructuredGrid", this->VTKDataTclName),
           atoi(pm->GetRootResult())))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions() == 1)
      {
      return;
      }  
    pm->ServerScript("%s UpdateInformation", this->VTKDataTclName);
    pm->RootScript("%s GetMaximumNumberOfPieces", this->VTKDataTclName);
    if (atoi(pm->GetRootResult()) != 1)
      { // The source can already produce pieces.
      return;
      }

    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      pm->ServerSimpleScript("vtkExtractUnstructuredGridPiece pvTemp");
      }
    else
      {
      pm->ServerSimpleScript("vtkTransmitUnstructuredGridPiece pvTemp");
      pm->ServerSimpleScript(
        "pvTemp AddObserver StartEvent {$Application LogStartEvent {Execute TransmitUGrid}}");
      pm->ServerSimpleScript(
        "pvTemp AddObserver EndEvent {$Application LogEndEvent {Execute TransmitUGrid}}");
      }
    }
    else if((pm->RootScript("%s IsA vtkImageData", this->VTKDataTclName),
             atoi(pm->GetRootResult())))
      {
      if (getenv("PV_LOCK_SAFE") == NULL)
        {
        pm->ServerSimpleScript("vtkStructuredCacheFilter pvTemp");
        }
      }
    else if((pm->RootScript("%s IsA vtkStructuredGrid", this->VTKDataTclName),
             atoi(pm->GetRootResult())))
      {
      if (getenv("PV_LOCK_SAFE") == NULL)
        {
        pm->ServerSimpleScript("vtkStructuredCacheFilter pvTemp");
        }
      }
    else if((pm->RootScript("%s IsA vtkRectilinearGrid", this->VTKDataTclName),
             atoi(pm->GetRootResult())))
      {
      if (getenv("PV_LOCK_SAFE") == NULL)
        {
        pm->ServerSimpleScript("vtkStructuredCacheFilter pvTemp");
        }
      }
  else
    {
    return;
    }

  // Connect the filter to the pipeline.
  pm->ServerScript("pvTemp SetInput %s", this->VTKDataTclName);
  // Set the output name to point to the new data object.
  // This is a bit of a hack because we have to strip the '$' off of the current name.
  pm->ServerScript("set %s [pvTemp GetOutput]", this->VTKDataTclName+1);
  
  this->Script("%s SetVTKDataTclName {%s}", this->GetTclName(),
               this->VTKDataTclName);
  
  // Now delete Tcl's reference to the piece filter.
  pm->ServerSimpleScript("pvTemp Delete");
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVPart::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name?this->Name:"none") << endl;
  os << indent << "GeometryTclName: " << (this->GeometryTclName?this->GeometryTclName:"none") << endl;
  os << indent << "VTKDataTclName: " << (this->VTKDataTclName?this->VTKDataTclName:"none") << endl;
  os << indent << "VTKSourceIndex: " << this->VTKSourceIndex << endl;
  os << indent << "VTKOutputIndex: " << this->VTKOutputIndex << endl;

}


  



