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

#include "vtkClientServerStream.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkImageData.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkPVApplication.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVColorMap.h"
#include "vtkPVConfig.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkPVColorMap.h"
#include "vtkPVPart.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkPVRenderView.h"

#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPartDisplay);
vtkCxxRevisionMacro(vtkPVPartDisplay, "1.26");


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
  this->GeometryID.ID = 0;

  this->VolumeID.ID         = 0;
  this->VolumeMapperID.ID   = 0;
  this->VolumePropertyID.ID = 0;
  this->VolumeColorID.ID    = 0;
  this->VolumeOpacityID.ID  = 0; 
  this->Volume              = NULL;
  
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
  
  if ( pm && this->VolumeID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeID );
    }
  if ( pm && this->VolumeMapperID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeMapperID );
    }
  if ( pm && this->VolumePropertyID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumePropertyID );
    }
  if ( pm && this->VolumeColorID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeColorID );
    }
  if ( pm && this->VolumeOpacityID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeOpacityID );
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
  if (pm && this->GeometryID.ID != 0)
    {
    pm->DeleteStreamObject(this->GeometryID);
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
void vtkPVPartDisplay::SetInput(vtkPVPart* input)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( !pvApp )
    {
    vtkErrorMacro("Set the application before you connect.");
    return;
    }
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();

  if (input == NULL)
    {
    vtkClientServerID nullID;
    nullID.ID = 0;
    stream 
      << vtkClientServerStream::Invoke << this->GeometryID <<  "SetInput" 
      << nullID << vtkClientServerStream::End;
    }
  else
    {
    stream 
      << vtkClientServerStream::Invoke << this->GeometryID <<  "SetInput" 
      << input->GetVTKDataID() << vtkClientServerStream::End;
    }
  pm->SendStreamToServer();
  
  if ( input != NULL )
  {
  vtkPVClassNameInformation *info = vtkPVClassNameInformation::New();
  pm->GatherInformation(info, input->GetVTKDataID());
  char *className = info->GetVTKClassName();
  
  if (strcmp(className, "vtkUnstructuredGrid") == 0 )
    {
    stream << vtkClientServerStream::Invoke << this->VolumeMapperID
           << "SetInput" << input->GetVTKDataID() << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
  info->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();

  // Create the geometry filter.
  this->GeometryID = pm->NewStreamObject("vtkPVGeometryFilter");
  stream << vtkClientServerStream::Invoke << this->GeometryID << "SetUseStrips"
         << pvApp->GetMainView()->GetTriangleStripsCheck()->GetState()
         << vtkClientServerStream::End;
  // fixme
  // I see no reason why this needs to be created on the client.
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

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  // fixme:  get rid of the extra sends.
  pm->SendStreamToClientAndServer();

  // Connect the geometry to the update suppressor.
  stream << vtkClientServerStream::Invoke << this->GeometryID
         << "GetOutput" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;

  // Now create the mapper.
  if (pvApp->GetUseSoftwareRendering())
    {
    // vtkPVPolyDataMapper does not have a mangled mesa version. 
    // Therefore, in case of software rendering, we use vtkPolyDataMapper.
    // This goes through the factory and creates the right mesa classes.
    this->MapperID = pm->NewStreamObject("vtkPolyDataMapper");
    }
  else
    {
    this->MapperID = pm->NewStreamObject("vtkPVPolyDataMapper");
    }
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
  this->Prop = 
    vtkProp::SafeDownCast(
      pm->GetObjectFromID(this->PropID));
  this->Mapper = 
    vtkPolyDataMapper::SafeDownCast(pm->GetObjectFromID(this->MapperID));

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
  
  
  // Now create the object for volume rendering if applicable
  this->VolumeID         = pm->NewStreamObject("vtkVolume");
  this->VolumeMapperID   = pm->NewStreamObject("vtkUnstructuredGridVolumeRayCastMapper");
  this->VolumePropertyID = pm->NewStreamObject("vtkVolumeProperty");
  this->VolumeOpacityID  = pm->NewStreamObject("vtkPiecewiseFunction");
  this->VolumeColorID    = pm->NewStreamObject("vtkColorTransferFunction");

  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "VisibilityOff" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "SetMapper" << this->VolumeMapperID  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "SetProperty" << this->VolumePropertyID  << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke << this->VolumePropertyID 
         << "SetScalarOpacity" << this->VolumeOpacityID  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumePropertyID 
         << "SetColor" << this->VolumeColorID  << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "AddHSVPoint" << 0.0 << 0.01 << 1.0 << 1.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "AddHSVPoint" << 128.0 << 0.5 << 1.0 << 1.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "AddHSVPoint" << 255.0 << 0.99 << 1.0 << 1.0 << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << 0.0 << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << 60.0 << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << 100.0 << 1.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << 255.0 << 1.0 << vtkClientServerStream::End;
  
  pm->SendStreamToClientAndServer();
  
  this->Volume = 
    vtkVolume::SafeDownCast(
      pm->GetObjectFromID(this->VolumeID));

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
    this->SendForceUpdate();
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
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  stream << vtkClientServerStream::Invoke << this->MapperID << "Modified"
         << vtkClientServerStream::End;
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
void vtkPVPartDisplay::SendForceUpdate()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  //cout << "vtkPVLODPartDisplay::ForceUpdate" << endl;

  pm->SendStreamToClientAndServer();
  //cout << "vtkPVLODPartDisplay::ForceUpdate - done" << endl;
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
  
  os << indent << "VolumeID: "         << this->VolumeID.ID         << endl;
  os << indent << "VolumeMapperID: "   << this->VolumeMapperID.ID   << endl;
  os << indent << "VolumePropertyID: " << this->VolumePropertyID.ID << endl;
  os << indent << "VolumeOpacityID: "  << this->VolumeOpacityID.ID  << endl;
  os << indent << "VolumeColorID: "    << this->VolumeColorID.ID    << endl;
  
}

