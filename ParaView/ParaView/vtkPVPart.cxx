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
#include "vtkPVClassNameInformation.h"
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
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPart);
vtkCxxRevisionMacro(vtkPVPart, "1.28.2.10");


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
  this->VTKDataID.ID = 0;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->GeometryID.ID = 0;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->ClassNameInformation = vtkPVClassNameInformation::New();

  this->VTKSourceIndex = -1;
  this->VTKOutputIndex = -1;
}

//----------------------------------------------------------------------------
vtkPVPart::~vtkPVPart()
{  
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = 0;
  if(pvApp)
    {
    pm = pvApp->GetProcessModule();
    }

  this->SetPartDisplay(NULL);
  // Get rid of the circular reference created by the extent translator.
  // We have a problem with ExtractPolyDataPiece also.
  if (this->VTKDataID.ID != 0)
    {
    vtkClientServerStream& stream = pm->GetStream();
    stream.Reset();
    stream << vtkClientServerStream::Invoke << this->VTKDataID << "SetExtentTranslator" << 0 
           << vtkClientServerStream::End;
    pm->SendStreamToServer();
    }  
    
  this->SetName(NULL);

  this->DataInformation->Delete();
  this->DataInformation = NULL;

  // Used to be in vtkPVActorComposite........

  if (this->GeometryID.ID != 0)
    {
    if ( pm )
      {
      pm->DeleteStreamObject(this->GeometryID);
      pm->SendStreamToClientAndServer();
      }
    }
  
  this->ClassNameInformation->Delete();
  this->ClassNameInformation = NULL;
}


//----------------------------------------------------------------------------
void vtkPVPart::SetPVApplication(vtkPVApplication *pvApp)
{
  if ( pvApp )
    {
    this->CreateParallelTclObjects(pvApp);
    }
  this->vtkKWObject::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
void vtkPVPart::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  if ( !pvApp )
    {
    return;
    }
  this->vtkKWObject::SetApplication(pvApp);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  this->GeometryID = pm->NewStreamObject("vtkPVGeometryFilter");
  stream << vtkClientServerStream::Invoke << this->GeometryID << "SetUseStrips"
         << pvApp->GetMainView()->GetTriangleStripsCheck()->GetState()
         << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  // Keep track of how long each geometry filter takes to execute.
  vtkClientServerStream start;
  start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
        << "LogStartEvent" << "Execute Geometry" 
        << vtkClientServerStream::End;
  vtkClientServerStream end;
  end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
      << "LogEndEvent" << "Execute Geometry" 
      << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryID 
                  << "AddObserver"
                  << "StartEvent"
                  << start
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->GeometryID 
                  << "AddObserver"
                  << "EndEvent"
                  << end
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
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
    this->PartDisplay->ConnectToGeometry(this->GeometryID);
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
  if ( !pvApp )
    {
    return;
    }
  vtkPVProcessModule *pm = pvApp->GetProcessModule();

  // This does nothing if the geometry is already up to date.
  if (this->PartDisplay)
    {
    this->PartDisplay->Update();
    }

  pm->GatherInformation(this->DataInformation, this->GetVTKDataID());

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
      long nc = this->DataInformation->GetNumberOfCells();
      sprintf(str, "Polygonal: %ld cells", nc);
      }
    else if (this->DataInformation->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
      {
      long nc = this->DataInformation->GetNumberOfCells();
      sprintf(str, "Unstructured Grid: %ld cells", nc);
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
void vtkPVPart::SetVTKDataID(vtkClientServerID id)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( !pvApp )
    {
    vtkErrorMacro("Set the application before you set the VTKDataID.");
    return;
    }
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->VTKDataID = id;
  vtkClientServerStream& stream = pm->GetStream();
  stream 
    << vtkClientServerStream::Invoke << this->GeometryID <<  "SetInput" 
    << this->VTKDataID << vtkClientServerStream::End;
  pm->SendStreamToServer();
}



//----------------------------------------------------------------------------
void vtkPVPart::InsertExtractPiecesIfNecessary()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  // We are going to create the piece filter with a dummy tcl name,
  // setup the pipeline, and remove tcl's reference to the objects.
  // The vtkData object will be moved to the output of the piece filter.
  vtkClientServerStream& stream = pm->GetStream();
  stream.Reset();
  stream << vtkClientServerStream::Invoke 
         << this->VTKDataID << "IsA" << "vtkPolyData" 
         << vtkClientServerStream::End;
  pm->SendStreamToServer();
  int ispolydata = 0;
  if(!pm->GetLastServerResult().GetArgument(0, 0, &ispolydata))
    {
    vtkErrorMacro("bad return from IsA call");
    }
  return;
  pm->GatherInformation(this->ClassNameInformation, this->VTKDataID);
  char *className = this->ClassNameInformation->GetVTKClassName();
  vtkClientServerID tempDataPiece = {0};
  if (!strcmp(className, "vtkPolyData"))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions() == 1)
      {
      return;
      }  
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->VTKDataID << "UpdateInformation"
                    << vtkClientServerStream::End;
    pm->SendStreamToServer();
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->VTKDataID << "GetMaximumNumberOfPieces"
                    << vtkClientServerStream::End;
    pm->SendStreamToServer(); // Was a RootScript
    int num =0;
    pm->GetLastServerResult().GetArgument(0,0,&num);
    if (num != 1)
      { // The source can already produce pieces.
      return;
      }
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      tempDataPiece = pm->NewStreamObject("vtkExtractPolyDataPiece");
      }
    else
      {
      tempDataPiece = pm->NewStreamObject("vtkTransmitPolyDataPiece");
      vtkClientServerStream start;
      start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
            << "LogStartEvent" << "Execute TransmitPData" 
            << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                      << "AddObserver"
                      << "StartEvent"
                      << start
                      << vtkClientServerStream::End;
      vtkClientServerStream end;
      end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
          << "LogEndEvent" << "Execute TransmitPData" 
          << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                      << "AddObserver"
                      << "EndEvent"
                      << start
                      << vtkClientServerStream::End;
      }
    }
  else if (!strcmp(className, "vtkUnstructuredGrid"))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions() == 1)
      {
      return;
      }
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->VTKDataID << "UpdateInformation"
                    << vtkClientServerStream::End;
    pm->SendStreamToServer();
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->VTKDataID << "GetMaximumNumberOfPieces"
                    << vtkClientServerStream::End;
    pm->SendStreamToServer(); // Was a RootScript
    int num =0;
    pm->GetLastServerResult().GetArgument(0,0,&num);
    if (num != 1)
      { // The source can already produce pieces.
      return;
      }

    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      { 
      tempDataPiece = pm->NewStreamObject("vtkExtractUnstructuredGridPiece");
      }
    else
      {
      tempDataPiece = pm->NewStreamObject("vtkTransmitUnstructuredGridPiece");
      vtkClientServerStream start;
      start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
            << "LogStartEvent" << "Execute TransmitPData" 
            << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                      << "AddObserver"
                      << "StartEvent"
                      << start
                      << vtkClientServerStream::End;
      vtkClientServerStream end;
      end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
          << "LogEndEvent" << "Execute TransmitPData" 
          << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                      << "AddObserver"
                      << "EndEvent"
                      << start
                      << vtkClientServerStream::End;
      }
    }
  else if (!strcmp(className, "vtkImageData"))
    {
    if (getenv("PV_LOCK_SAFE") == NULL)
      { 
      tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
      }
    }
  else if (!strcmp(className, "vtkStructuredGrid"))
    {
    if (getenv("PV_LOCK_SAFE") == NULL)
      {
      tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
      }
    }
  else if (!strcmp(className, "vtkRectilinearGrid"))
    {
    if (getenv("PV_LOCK_SAFE") == NULL)
      {
      tempDataPiece = pm->NewStreamObject("vtkStructuredCacheFilter");
      }
    }
  else
    {
    return;
    }
  // Connect the filter to the pipeline.  
  pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                  << "SetInput"
                  << this->VTKDataID
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << tempDataPiece 
                  << "GetOutput"
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();
  pm->GetLastServerResult().GetArgument(0, 0, &this->VTKDataID);
  pm->DeleteStreamObject(tempDataPiece);
  pm->SendStreamToServer();
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
//  os << indent << "GeometryID: " << (this->GeometryTclName?this->GeometryTclName:"none") << endl;
//  os << indent << "VTKDataTclName: " << (this->VTKDataTclName?this->VTKDataTclName:"none") << endl;
  os << indent << "ClassNameInformation: " << this->ClassNameInformation << endl;
  os << indent << "VTKSourceIndex: " << this->VTKSourceIndex << endl;
  os << indent << "VTKOutputIndex: " << this->VTKOutputIndex << endl;
}
