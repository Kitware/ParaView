/*=========================================================================Interpolate

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
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkPVClassNameInformation.h"
#include "vtkRMScalarBarWidget.h"
#include "vtkPVConfig.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkSMPart.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPartDisplay);
vtkCxxRevisionMacro(vtkPVPartDisplay, "1.1");


//----------------------------------------------------------------------------
vtkPVPartDisplay::vtkPVPartDisplay()
{
  this->ProcessModule = NULL;

  this->DirectColorFlag = 1;
  this->InterpolateColorsFlag = 0;
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

  this->VolumeID.ID            = 0;
  this->VolumeTetraFilterID.ID = 0;
  this->VolumeMapperID.ID      = 0;
  this->VolumePropertyID.ID    = 0;
  this->VolumeColorID.ID       = 0;
  this->VolumeOpacityID.ID     = 0; 
  this->VolumeFieldFilterID.ID = 0;
  this->Volume                 = NULL;
  this->VolumeColor            = NULL;
  this->VolumeOpacity          = NULL;
  
  this->VolumeRenderMode       = 0;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
}

//----------------------------------------------------------------------------
vtkPVPartDisplay::~vtkPVPartDisplay()
{
  vtkPVProcessModule* pm = this->ProcessModule;
  
  if ( pm && this->VolumeID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeID );
    }
  if ( pm && this->VolumeTetraFilterID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeTetraFilterID );
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
  if ( pm && this->VolumeFieldFilterID.ID != 0 )
    {
    pm->DeleteStreamObject( this->VolumeFieldFilterID );
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
  
  if(pm)
    {
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  if (pm && this->GeometryID.ID != 0)
    {
    pm->DeleteStreamObject(this->GeometryID);
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }

  if (pm && this->UpdateSuppressorID.ID)
    {
    pm->DeleteStreamObject(this->UpdateSuppressorID);
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  
  this->SetPart(NULL);
  this->SetProcessModule( NULL);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetInput(vtkSMPart* input)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
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
      << input->GetID(0) << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
  
  if ( input != NULL )
    {
    vtkPVClassNameInformation *info = vtkPVClassNameInformation::New();
    pm->GatherInformation(info, input->GetID(0));
    char *className = info->GetVTKClassName();
  
    if (strcmp(className, "vtkUnstructuredGrid") == 0 )
      {
      stream << vtkClientServerStream::Invoke << this->VolumeFieldFilterID
             << "SetInput" << input->GetID(0)
             << vtkClientServerStream::End;
    
      stream << vtkClientServerStream::Invoke << this->VolumeFieldFilterID
             << "GetOutput" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << this->VolumeTetraFilterID
             << "SetInput" <<  vtkClientServerStream::LastResult 
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << this->VolumeTetraFilterID
             << "GetOutput" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << this->VolumeMapperID
             << "SetInput" <<  vtkClientServerStream::LastResult 
             << vtkClientServerStream::End;

      pm->SendStream(vtkProcessModule::DATA_SERVER);
      }
    info->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  vtkClientServerStream& stream = pm->GetStream();

  // Create the geometry filter.
  this->GeometryID = pm->NewStreamObject("vtkPVGeometryFilter");
  stream << vtkClientServerStream::Invoke << this->GeometryID << "SetUseStrips"
         << pm->GetUseTriangleStrips()
         << vtkClientServerStream::End;
  // fixme
  // I see no reason why this needs to be created on the client.
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Keep track of how long each geometry filter takes to execute.
  vtkClientServerStream start;
  start << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
        << "LogStartEvent" << "Execute Geometry" 
        << vtkClientServerStream::End;
  vtkClientServerStream end;
  end << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
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
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  // fixme:  get rid of the extra sends.
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Connect the geometry to the update suppressor.
  stream << vtkClientServerStream::Invoke << this->GeometryID
         << "GetOutput" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;

  // Now create the mapper.
  this->MapperID = pm->NewStreamObject("vtkPolyDataMapper");
  stream << vtkClientServerStream::Invoke << this->MapperID << "UseLookupTableScalarRangeOn" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID << "InterpolateScalarsBeforeMappingOn" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "GetPolyDataOutput" 
         <<  vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetImmediateModeRendering" 
         << pm->GetUseImmediateMode() << vtkClientServerStream::End;
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
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  
  
  // Now create the object for volume rendering if applicable
  this->VolumeID            = pm->NewStreamObject("vtkVolume");
  this->VolumeTetraFilterID = pm->NewStreamObject("vtkDataSetTriangleFilter");
  this->VolumeMapperID      = pm->NewStreamObject("vtkUnstructuredGridVolumeRayCastMapper");
  this->VolumePropertyID    = pm->NewStreamObject("vtkVolumeProperty");
  this->VolumeOpacityID     = pm->NewStreamObject("vtkPiecewiseFunction");
  this->VolumeColorID       = pm->NewStreamObject("vtkColorTransferFunction");
  this->VolumeFieldFilterID = pm->NewStreamObject("vtkFieldDataToAttributeDataFilter");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  
  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "VisibilityOff" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "SetMapper" << this->VolumeMapperID  
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeID 
         << "SetProperty" << this->VolumePropertyID  
         << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke << this->VolumePropertyID 
         << "SetScalarOpacity" << this->VolumeOpacityID  
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumePropertyID 
         << "SetColor" << this->VolumeColorID  
         << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "RemoveAllPoints"  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "SetColorSpaceToHSVNoWrap"  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "RemoveAllPoints" << vtkClientServerStream::End;
  
  pm->SendStream(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  
  this->Volume = 
    vtkVolume::SafeDownCast(
      pm->GetObjectFromID(this->VolumeID));

  this->VolumeOpacity = 
    vtkPiecewiseFunction::SafeDownCast(
      pm->GetObjectFromID(this->VolumeOpacityID));

  this->VolumeColor = 
    vtkColorTransferFunction::SafeDownCast(
      pm->GetObjectFromID(this->VolumeColorID));

}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetImmediateModeRendering" << val << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetVisibility(int v)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();

  this->Visibility = v;
  
  if ( v )
    {
    if ( this->VolumeRenderMode )
      {
      this->VolumeRenderModeOn();
      }
    else
      {
      this->VolumeRenderModeOff();
      }
    }
  else
    {
    if ( this->PropID.ID ) 
      {
      stream << vtkClientServerStream::Invoke << this->PropID
             << "SetVisibility" << v << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if ( this->VolumeID.ID )
      {
      stream << vtkClientServerStream::Invoke << this->VolumeID
             << "SetVisibility" << v << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }
  
  // Recompute total visibile memory size.
  if (pm->GetRenderModule())
    {
    pm->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);
    }
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetColor(float r, float g, float b)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetColor" << r << g << b << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecular" << 0.1 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecularPower" << 100.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->PropertyID
         << "SetSpecularColor" << 1.0 << 1.0 << 1.0 << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}  


//----------------------------------------------------------------------------
void vtkPVPartDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = this->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
           << "ForceUpdate" << vtkClientServerStream::End;
    this->SendForceUpdate();
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetProcessModule(vtkPVProcessModule* pm)
{
  if (pm == 0)
    {
    if (this->ProcessModule)
      {
      this->ProcessModule->Delete();
      this->ProcessModule = 0;
      }
    return;
    }

  if (this->ProcessModule)
    {
    vtkErrorMacro("ProcessModule already set and part has been initialized.");
    return;
    }

  this->CreateParallelTclObjects(pm);
  this->ProcessModule = pm;
  this->ProcessModule->Register(this);
}



//----------------------------------------------------------------------------
void vtkPVPartDisplay::RemoveAllCaches()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPartDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  stream << vtkClientServerStream::Invoke << this->MapperID << "Modified"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}






//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetScalarVisibility(int val)
{  
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetScalarVisibility" << val << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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

  vtkPVProcessModule *pm = this->GetProcessModule();
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
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetInterpolateColorsFlag(int val)
{
  if (val)
    {
    val = 1;
    }
  if (val == this->InterpolateColorsFlag)
    {
    return;
    }

  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  this->InterpolateColorsFlag = val;
  stream << vtkClientServerStream::Invoke << this->MapperID
         << "SetInterpolateScalarsBeforeMapping" << (!val) 
         << vtkClientServerStream::End;

  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ColorByArray(vtkRMScalarBarWidget *colorMap,
                                    int field)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
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
void vtkPVPartDisplay::VolumeRenderModeOn()
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  
  if ( this->Visibility )
    {
    if ( this->PropID.ID )
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke
        << this->PropID << "VisibilityOff" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if ( this->VolumeID.ID )
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke
        << this->VolumeID << "VisibilityOn" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }
  
  this->VolumeRenderMode = 1;
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::VolumeRenderModeOff()
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  
  if ( this->Visibility )
    {
    if ( this->PropID.ID )
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke
        << this->PropID << "VisibilityOn" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    if ( this->VolumeID.ID )
      {
      pm->GetStream() 
        << vtkClientServerStream::Invoke
        << this->VolumeID << "VisibilityOff" << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    }
  
  this->VolumeRenderMode = 0;
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::InitializeTransferFunctions(vtkPVArrayInformation *arrayInfo,
                                                   vtkPVDataInformation *dataInfo )
{
  // need to initialize only if there are no points in the function
  if ( this->VolumeOpacity->GetSize() == 0 )
    {
    this->ResetTransferFunctions(arrayInfo, dataInfo);
    }
  
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ResetTransferFunctions(vtkPVArrayInformation *arrayInfo, 
                                              vtkPVDataInformation *dataInfo)
{
  double range[2];
  arrayInfo->GetComponentRange(0, range);
  
    
  double bounds[6];
  dataInfo->GetBounds(bounds);
  double diameter = 
    sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
          (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
          (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );
  
  int numCells = dataInfo->GetNumberOfCells();
  double linearNumCells = pow( (double) numCells, (1.0/3.0) );
  double unitDistance = diameter / linearNumCells;
  
  vtkPVProcessModule* pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "RemoveAllPoints" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << range[0] << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityID 
         << "AddPoint" << range[1] << 1.0 << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "RemoveAllPoints" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "AddHSVPoint" << range[0] << 0.0 << 1.0 << 1.0 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "AddHSVPoint" << range[1] << 0.8 << 1.0 << 1.0 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorID 
         << "SetColorSpaceToHSVNoWrap" << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumePropertyID
         << "SetScalarOpacityUnitDistance" << unitDistance << vtkClientServerStream::End;
  
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::VolumeRenderPointField(const char *name)
{
  vtkPVProcessModule* pm = this->GetProcessModule();
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  vtkClientServerStream& stream = pm->GetStream();

  stream << vtkClientServerStream::Invoke << this->VolumeFieldFilterID
         << "SetInputFieldToPointDataField" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeFieldFilterID
         << "SetOutputAttributeDataToPointData" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeFieldFilterID
         << "SetScalarComponent" << 0 << name << 0 << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER);

}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SendForceUpdate()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  //cout << "vtkPVLODPartDisplay::ForceUpdate" << endl;

  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  //cout << "vtkPVLODPartDisplay::ForceUpdate - done" << endl;
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: "   << this->Visibility    << endl;
  os << indent << "Part: "         << this->Part          << endl;
  os << indent << "Mapper: "       << this->GetMapper()   << endl;
  os << indent << "MapperID: "     << this->MapperID.ID   << endl;
  os << indent << "PropID: "       << this->PropID.ID     << endl;
  os << indent << "PropertyID: "   << this->PropertyID.ID << endl;
  
  os << indent << "ProcessModule: "    << this->ProcessModule         << endl;
  os << indent << "DirectColorFlag: "  << this->DirectColorFlag       << endl;
  os << indent << "InterpolateColorsFlag: " << this->InterpolateColorsFlag << endl;
  os << indent << "UpdateSuppressor: " << this->UpdateSuppressorID.ID << endl;
  
  os << indent << "VolumeID: "            << this->VolumeID.ID            << endl;
  os << indent << "VolumeMapperID: "      << this->VolumeMapperID.ID      << endl;
  os << indent << "VolumePropertyID: "    << this->VolumePropertyID.ID    << endl;
  os << indent << "VolumeOpacityID: "     << this->VolumeOpacityID.ID     << endl;
  os << indent << "VolumeColorID: "       << this->VolumeColorID.ID       << endl;
  os << indent << "VolumeTetraFilterID: " << this->VolumeTetraFilterID.ID << endl;
  os << indent << "VolumeFieldFilterID: " << this->VolumeFieldFilterID.ID << endl;
  os << indent << "Mapper: ";
  if (this->Mapper)
    {
    this->Mapper->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)";
    }
  os << endl;

  os << indent << "Prop: ";
  if (this->Prop)
    {
    this->Prop->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)";
    }
  os << endl;
  
}

