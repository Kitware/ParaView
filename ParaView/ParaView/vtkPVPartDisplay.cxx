/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPartDisplay.h"

#include "vtkPVRenderModule.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkPVColorMap.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkClientServerStream.h"
#include "vtkPVRenderView.h"
#include "vtkKWCheckButton.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPartDisplay);
vtkCxxRevisionMacro(vtkPVPartDisplay, "1.16");


//----------------------------------------------------------------------------
vtkPVPartDisplay::vtkPVPartDisplay()
{
  this->PVApplication = NULL;

  this->DirectColorFlag = 1;
  this->Visibility = 1;
  this->Part = NULL;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->Mapper = NULL;
  this->Property = NULL;
  this->Prop = NULL;
  this->PropID.ID =0;
  this->PropertyID.ID =0;
  this->MapperID.ID = 0;
  this->UpdateSuppressorID.ID = 0;
  this->GeometryIsValid = 0;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
}

//----------------------------------------------------------------------------
vtkPVPartDisplay::~vtkPVPartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = 0;
  if(pvApp)
    {
    pm = pvApp->GetProcessModule();
    }
  
  if ( pm && this->MapperID.ID != 0)
    {
    pm->DeleteStreamObject(this->MapperID);
    }
  this->Mapper = NULL;
    
  if ( pm && this->PropID.ID != 0)
    {
    pm->DeleteStreamObject(this->PropID);
    }
  this->Prop = NULL;
  
  if ( pm && this->PropertyID.ID !=0)
    {  
    pm->DeleteStreamObject(this->PropertyID);
    }
  this->Property = NULL;
  
  if (pm && this->UpdateSuppressorID.ID)
    {
    pm->DeleteStreamObject(this->UpdateSuppressorID);
    }
  if(pm)
    {
    pm->SendStreamToClientAndServer();
    }

  this->SetPart(NULL);
  this->SetPVApplication( NULL);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ConnectToGeometry(vtkClientServerID geometryID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << geometryID
         << "GetOutput" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");

  // Now create the mapper.
  this->MapperID = pm->NewStreamObject("vtkPolyDataMapper");
  stream << vtkClientServerStream::Invoke << this->MapperID << "UseLookupTableScalarRangeOn" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "GetOutput" 
         <<  vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetImmediateModeRendering" 
         << pvApp->GetMainView()->GetImmediateModeCheck()->GetState() << vtkClientServerStream::End;
  // Create a LOD Actor for the subclasses.
  // I could use just a plain actor for this class.
  
  this->PropID = pm->NewStreamObject("vtkPVLODActor");
  this->PropertyID = pm->NewStreamObject("vtkProperty");
  
  // I used to use ambient 0.15 and diffuse 0.85, but VTK did not
  // handle it correctly.
  stream << vtkClientServerStream::Invoke << this->PropertyID 
         << "SetAmbient" << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID 
         << "SetDiffuse" << 1.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropID 
         << "SetProperty" << this->PropertyID  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropID 
         << "SetMapper" << this->MapperID  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  // now we can get pointers to the client vtk objects, this
  // must be after the streams are sent
  this->Property = 
    vtkProperty::SafeDownCast(
      pm->GetObjectFromID(this->PropertyID));
  this->Mapper = 
    vtkPolyDataMapper::SafeDownCast(
      pm->GetObjectFromID(this->MapperID));

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetImmediateModeRendering" << val << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetVisibility(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  if ( !pvApp || !pvApp->GetRenderModule() )
    {
    return;
    }

  if (this->PropID.ID != 0)
    {
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->PropID
           << "SetVisibility" << v << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
  this->Visibility = v;
  // Recompute total visibile memory size.
  pvApp->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetColor" << r << g << b << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecular" << 0.1 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecularPower" << 100.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecularColor" << 1.0 << 1.0 << 1.0 << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}  


//----------------------------------------------------------------------------
void vtkPVPartDisplay::Update()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetPVApplication(vtkPVApplication *pvApp)
{
  if (pvApp == NULL)
    {
    if (this->PVApplication)
      {
      this->PVApplication->Delete();
      this->PVApplication = NULL;
      }
    return;
    }

  if (this->PVApplication)
    {
    vtkErrorMacro("PVApplication already set and part has been initialized.");
    return;
    }

  this->CreateParallelTclObjects(pvApp);
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);
}



//----------------------------------------------------------------------------
void vtkPVPartDisplay::RemoveAllCaches()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStreamToClientAndServer();
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPartDisplay::CacheUpdate(int idx, int total)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End; 
  pm->SendStreamToClientAndServer();
}






//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetScalarVisibility(int val)
{  
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetScalarVisibility" << val << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetDirectColorFlag(int val)
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
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  this->DirectColorFlag = val;
  if (val)
    {
    stream << vtkClientServerStream::Invoke << this->MapperID
           << "SetColorModeToDefault" << vtkClientServerStream::End;
    }
  else
    {
    stream << vtkClientServerStream::Invoke << this->MapperID
           << "SetColorModeToMapScalars" << vtkClientServerStream::End;
    }
  pm->SendStreamToClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ColorByArray(vtkPVColorMap *colorMap,
                                    int field)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  
  // Turn off the specualr so it does not interfere with data.
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecular" << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetLookupTable" << colorMap->GetLookupTableID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "ScalarVisibilityOn" << vtkClientServerStream::End;

  if (field == VTK_CELL_DATA_FIELD)
    { 
    stream << vtkClientServerStream::Invoke << this->MapperID
           << "SetScalarModeToUseCellFieldData" << vtkClientServerStream::End;
    }
  else if (field == VTK_POINT_DATA_FIELD)
    {
    stream << vtkClientServerStream::Invoke << this->MapperID
           << "SetScalarModeToUsePointFieldData" << vtkClientServerStream::End;
    }
  else
    {
    vtkErrorMacro("Only point or cell field please.");
    }
//   stream << vtkClientServerStream::Invoke << this->MapperID
//          << "SelectColorArray" << colorMap->GetArrayName() << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "Part: " << this->Part << endl;
  os << indent << "Mapper: " << this->GetMapper() << endl;
  os << indent << "MapperID: " << this->MapperID.ID << endl;
  os << indent << "PropID: " << this->PropID.ID << endl;
  os << indent << "PropertyID: " << this->PropertyID.ID << endl;
  os << indent << "PVApplication: " << this->PVApplication << endl;
  os << indent << "DirectColorFlag: " << this->DirectColorFlag << endl;
  os << indent << "UpdateSuppressor: " << this->UpdateSuppressorID.ID << endl;
}


  



