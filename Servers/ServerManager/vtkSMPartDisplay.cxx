/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPartDisplay.h"

#include "vtkClientServerStream.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPVConfig.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkSMPart.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkProperty.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVClassNameInformation.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPartDisplay);
vtkCxxRevisionMacro(vtkSMPartDisplay, "1.22");

//----------------------------------------------------------------------------
vtkSMPartDisplay::vtkSMPartDisplay()
{
  this->GeometryInformation = vtkPVGeometryInformation::New();
  this->GeometryInformationIsValid = 0;
  
  this->Visibility = 1;
  
  this->PropVisibilityProperty = vtkSMIntVectorProperty::New();
  this->PropVisibilityProperty->SetCommand("SetVisibility");
  this->PropVisibilityProperty->SetNumberOfElements(1);
  this->PropVisibilityProperty->SetElement(0,1);
  
  this->VolumeVisibilityProperty = vtkSMIntVectorProperty::New();
  this->VolumeVisibilityProperty->SetCommand("SetVisibility");
  this->VolumeVisibilityProperty->SetNumberOfElements(1);
  this->VolumeVisibilityProperty->SetElement(0,0);
  
  this->ScalarVisibilityProperty = vtkSMIntVectorProperty::New();
  this->ScalarVisibilityProperty->SetCommand("SetScalarVisibility");
  this->ScalarVisibilityProperty->SetNumberOfElements(1);
  this->ScalarVisibilityProperty->SetElement(0,0);
    
  this->UseTriangleStripsProperty = vtkSMIntVectorProperty::New();
  this->UseTriangleStripsProperty->SetCommand("SetUseStrips");
  this->UseTriangleStripsProperty->SetNumberOfElements(1);
  this->UseTriangleStripsProperty->SetElement(0,0);
    
  this->UseImmediateModeProperty = vtkSMIntVectorProperty::New();
  this->UseImmediateModeProperty->SetCommand("SetImmediateModeRendering");
  this->UseImmediateModeProperty->SetNumberOfElements(1);
  this->UseImmediateModeProperty->SetElement(0,0);
    
  this->DirectColorFlagProperty = vtkSMIntVectorProperty::New();
  this->DirectColorFlagProperty->SetCommand("SetColorMode");
  this->DirectColorFlagProperty->SetNumberOfElements(1);
  this->DirectColorFlagProperty->SetElement(0, VTK_COLOR_MODE_DEFAULT);
    
  this->InterpolateColorsFlagProperty = vtkSMIntVectorProperty::New();
  this->InterpolateColorsFlagProperty->SetCommand("SetInterpolateScalarsBeforeMapping");
  this->InterpolateColorsFlagProperty->SetNumberOfElements(1);
  this->InterpolateColorsFlagProperty->SetElement(0, 1);

  this->LineWidthProperty = vtkSMDoubleVectorProperty::New();
  this->LineWidthProperty->SetCommand("SetLineWidth");
  this->LineWidthProperty->SetNumberOfElements(1);
  this->LineWidthProperty->SetElement(0, 1.0);
  
  this->PointSizeProperty = vtkSMDoubleVectorProperty::New();
  this->PointSizeProperty->SetCommand("SetPointSize");
  this->PointSizeProperty->SetNumberOfElements(1);
  this->PointSizeProperty->SetElement(0, 1.0);
  
  this->InterpolationProperty = vtkSMIntVectorProperty::New();
  this->InterpolationProperty->SetCommand("SetInterpolation");
  this->InterpolationProperty->SetNumberOfElements(1);
  this->InterpolationProperty->SetElement(0, VTK_GOURAUD);

  this->OpacityProperty = vtkSMDoubleVectorProperty::New();
  this->OpacityProperty->SetCommand("SetOpacity");
  this->OpacityProperty->SetNumberOfElements(1);
  this->OpacityProperty->SetElement(0, 1.0);
  
  this->ScaleProperty = vtkSMDoubleVectorProperty::New();
  this->ScaleProperty->SetCommand("SetScale");
  this->ScaleProperty->SetNumberOfElements(3);
  this->ScaleProperty->SetElement(0, 1.0);
  this->ScaleProperty->SetElement(1, 1.0);
  this->ScaleProperty->SetElement(2, 1.0);
  
  this->TranslateProperty = vtkSMDoubleVectorProperty::New();
  this->TranslateProperty->SetCommand("SetPosition");
  this->TranslateProperty->SetNumberOfElements(3);
  this->TranslateProperty->SetElement(0, 0.0);
  this->TranslateProperty->SetElement(1, 0.0);
  this->TranslateProperty->SetElement(2, 0.0);
  
  this->OrientationProperty = vtkSMDoubleVectorProperty::New();
  this->OrientationProperty->SetCommand("SetOrientation");
  this->OrientationProperty->SetNumberOfElements(1);
  this->OrientationProperty->SetElement(0, 0.0);
  this->OrientationProperty->SetElement(1, 0.0);
  this->OrientationProperty->SetElement(2, 0.0);
  
  this->OriginProperty = vtkSMDoubleVectorProperty::New();
  this->OriginProperty->SetCommand("SetOrigin");
  this->OriginProperty->SetNumberOfElements(1);
  this->OriginProperty->SetElement(0, 0.0);
  this->OriginProperty->SetElement(1, 0.0);
  this->OriginProperty->SetElement(2, 0.0);
  
  this->ColorProperty = vtkSMDoubleVectorProperty::New();
  this->ColorProperty->SetCommand("SetColor");
  this->ColorProperty->SetNumberOfElements(1);
  this->ColorProperty->SetElement(0, 1.0);
  this->ColorProperty->SetElement(1, 1.0);
  this->ColorProperty->SetElement(2, 1.0);

  this->ColorField = vtkDataSet::POINT_DATA_FIELD;

  this->Source = 0;
  this->ColorMap = 0;
  
  this->PropProxy =0;
  this->PropertyProxy =0;
  this->MapperProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->GeometryIsValid = 0;
  this->GeometryProxy = 0;

  this->Representation = VTK_OUTLINE;

  this->VolumeProxy            = 0;
  this->VolumeTetraFilterProxy = 0;
  this->VolumeMapperProxy      = 0;
  this->VolumePropertyProxy    = 0;
  this->VolumeColorProxy       = 0;
  this->VolumeOpacityProxy     = 0; 
  this->Volume                 = 0;
  this->VolumeColor            = 0;
  this->VolumeOpacity          = 0;
  
  this->OpacityUnitDistance    = 0;
  this->VolumeRenderMode       = 0;
  this->VolumeRenderField      = NULL;
 }

//----------------------------------------------------------------------------
vtkSMPartDisplay::~vtkSMPartDisplay()
{
  this->GeometryInformation->Delete();

  this->PropVisibilityProperty->Delete();
  this->PropVisibilityProperty = 0;
  this->VolumeVisibilityProperty->Delete();
  this->VolumeVisibilityProperty = 0;
  this->ScalarVisibilityProperty->Delete();
  this->ScalarVisibilityProperty = 0;
  this->UseTriangleStripsProperty->Delete();
  this->UseTriangleStripsProperty = 0;
  this->UseImmediateModeProperty->Delete();
  this->UseImmediateModeProperty = 0;
  this->DirectColorFlagProperty->Delete();
  this->DirectColorFlagProperty = 0;
  this->InterpolateColorsFlagProperty->Delete();
  this->InterpolateColorsFlagProperty = 0;
  this->LineWidthProperty->Delete();
  this->LineWidthProperty = 0;
  this->PointSizeProperty->Delete();
  this->PointSizeProperty = 0;
  this->InterpolationProperty->Delete();
  this->InterpolationProperty = 0;
  this->OpacityProperty->Delete();
  this->OpacityProperty = 0;
  this->ScaleProperty->Delete();
  this->ScaleProperty = 0;
  this->TranslateProperty->Delete();
  this->TranslateProperty = 0;
  this->OrientationProperty->Delete();
  this->OrientationProperty = 0;
  this->OriginProperty->Delete();
  this->OriginProperty = 0;
  this->ColorProperty->Delete();
  this->ColorProperty = 0;

  if (this->VolumeProxy != 0 )
    {
    this->VolumeProxy->Delete();
    this->VolumeProxy = 0;
    }
  if (this->VolumeTetraFilterProxy != 0 )
    {
    this->VolumeTetraFilterProxy->Delete();
    this->VolumeTetraFilterProxy = 0;
    }
  if (this->VolumeMapperProxy != 0 )
    {
    this->VolumeMapperProxy->Delete();
    this->VolumeMapperProxy = 0;
    }
  if (this->VolumePropertyProxy != 0 )
    {
    this->VolumePropertyProxy->Delete();
    this->VolumePropertyProxy = 0;
    }
  if (this->VolumeColorProxy != 0 )
    {
    this->VolumeColorProxy->Delete();
    this->VolumeColorProxy = 0;
    }
  if (this->VolumeOpacityProxy != 0 )
    {
    this->VolumeOpacityProxy->Delete();
    this->VolumeOpacityProxy = 0;
    }
  if (this->MapperProxy != 0)
    {
    this->MapperProxy->Delete();
    this->MapperProxy = 0;
    }
    
  if (this->PropProxy != 0)
    {
    this->PropProxy->Delete();
    this->PropProxy = 0;
    }
  
  if (this->PropertyProxy !=0)
    {  
    this->PropertyProxy->Delete();
    this->PropertyProxy = 0;
    }
  if (this->GeometryProxy != 0)
    {
    this->GeometryProxy->Delete();
    this->GeometryProxy = 0;
    }
  if (this->UpdateSuppressorProxy)
    {
    this->UpdateSuppressorProxy->Delete();
    this->UpdateSuppressorProxy = 0;
    }
  
  if (this->ColorMap)
    {
    this->ColorMap->UnRegister(this);
    this->ColorMap = 0;
    }
  
  this->SetSource(0);
  this->SetVolumeRenderField(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::CreateVTKObjects(int num)
{
  int i;
  // hack until input uses proxy input.
  vtkPVProcessModule* pm =
    vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  vtkClientServerStream stream;

  // just create the proxys here.

  // Create the geometry filter.
  this->GeometryProxy = vtkSMProxy::New();
  this->GeometryProxy->SetVTKClassName("vtkPVGeometryFilter");
  this->GeometryProxy->SetServersSelf(vtkProcessModule::DATA_SERVER);
  this->GeometryProxy->AddProperty("UseTriangleStrips", 
                                   this->UseTriangleStripsProperty);

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  this->UpdateSuppressorProxy = vtkSMProxy::New();
  this->UpdateSuppressorProxy->SetVTKClassName("vtkPVUpdateSuppressor");
  this->UpdateSuppressorProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  // Now create the mapper.
  this->MapperProxy = vtkSMProxy::New();
  this->MapperProxy->SetVTKClassName("vtkPolyDataMapper");
  this->MapperProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->MapperProxy->AddProperty("ScalarVisibility", this->ScalarVisibilityProperty);
  this->MapperProxy->AddProperty("DirectColorFlag", this->DirectColorFlagProperty);
  this->MapperProxy->AddProperty("InterpolateColorsFlag", this->InterpolateColorsFlagProperty);
  this->MapperProxy->AddProperty("UseImmediateMode", this->UseImmediateModeProperty);

  // Create a LOD Actor for the subclasses.
  // I could use just a plain actor for this class.
  this->PropProxy = vtkSMProxy::New();
  this->PropProxy->SetVTKClassName("vtkPVLODActor");
  this->PropProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->PropProxy->AddProperty("Visibility", this->PropVisibilityProperty);
  this->PropProxy->AddProperty("Translate", this->TranslateProperty);
  this->PropProxy->AddProperty("Scale", this->ScaleProperty);
  this->PropProxy->AddProperty("Orientation", this->OrientationProperty);
  this->PropProxy->AddProperty("Origin", this->OriginProperty);

  // this is a vtk property not SM.
  this->PropertyProxy = vtkSMProxy::New();
  this->PropertyProxy->SetVTKClassName("vtkProperty");
  this->PropertyProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->PropertyProxy->AddProperty("LineWidth", this->LineWidthProperty);
  this->PropertyProxy->AddProperty("PointSize", this->PointSizeProperty);
  this->PropertyProxy->AddProperty("Interpolation", this->InterpolationProperty);
  this->PropertyProxy->AddProperty("Color", this->ColorProperty);
  this->PropertyProxy->AddProperty("Opacity", this->OpacityProperty);

  // Now create the object for volume rendering if applicable
  this->VolumeProxy = vtkSMProxy::New();
  this->VolumeProxy->SetVTKClassName("vtkPVLODVolume");
  this->VolumeProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);
  // Share properties.
  this->VolumeProxy->AddProperty("Visibility", this->VolumeVisibilityProperty);
  this->VolumeProxy->AddProperty("Translate", this->TranslateProperty);
  this->VolumeProxy->AddProperty("Scale", this->ScaleProperty);
  this->VolumeProxy->AddProperty("Orientation", this->OrientationProperty);
  this->VolumeProxy->AddProperty("Origin", this->OriginProperty);

  this->VolumeTetraFilterProxy = vtkSMProxy::New();
  this->VolumeTetraFilterProxy->SetVTKClassName("vtkDataSetTriangleFilter");
  this->VolumeTetraFilterProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->VolumeMapperProxy = vtkSMProxy::New();
  this->VolumeMapperProxy->SetVTKClassName("vtkUnstructuredGridVolumeRayCastMapper");
  this->VolumeMapperProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->VolumePropertyProxy = vtkSMProxy::New();
  this->VolumePropertyProxy->SetVTKClassName("vtkVolumeProperty");
  this->VolumePropertyProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->VolumeOpacityProxy = vtkSMProxy::New();
  this->VolumeOpacityProxy->SetVTKClassName("vtkPiecewiseFunction");
  this->VolumeOpacityProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->VolumeColorProxy = vtkSMProxy::New();
  this->VolumeColorProxy->SetVTKClassName("vtkColorTransferFunction");
  this->VolumeColorProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->GeometryProxy->CreateVTKObjects(num);
  this->UpdateSuppressorProxy->CreateVTKObjects(num);
  this->MapperProxy->CreateVTKObjects(num);
  this->PropProxy->CreateVTKObjects(num);
  this->PropertyProxy->CreateVTKObjects(num);
  this->VolumeProxy->CreateVTKObjects(num);
  this->VolumeTetraFilterProxy->CreateVTKObjects(num);
  this->VolumeMapperProxy->CreateVTKObjects(num);
  this->VolumePropertyProxy->CreateVTKObjects(num);
  this->VolumeOpacityProxy->CreateVTKObjects(num);
  this->VolumeColorProxy->CreateVTKObjects(num);

  // Set the current default.  
  this->UseTriangleStripsProperty->SetElement(0, pm->GetUseTriangleStrips());
  this->UseImmediateModeProperty->SetElement(0, pm->GetUseImmediateMode());
  for (i = 0; i < num; ++i)
    {  
    // Should we use property to send the value to server?
    stream << vtkClientServerStream::Invoke << this->GeometryProxy->GetID(i) 
          << "SetUseStrips" << pm->GetUseTriangleStrips()
          << vtkClientServerStream::End;

    // Keep track of how long each geometry filter takes to execute.
    vtkClientServerStream start;
    start << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
          << "LogStartEvent" << "Execute Geometry" 
          << vtkClientServerStream::End;
    vtkClientServerStream end;
    end << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
        << "LogEndEvent" << "Execute Geometry" 
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->GeometryProxy->GetID(i) 
           << "AddObserver"
           << "StartEvent"
           << start
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->GeometryProxy->GetID(i) 
           << "AddObserver"
           << "EndEvent"
           << end
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

    // Now create the update supressors which keep the renderers/mappers
    // from updating the pipeline.  These are here to ensure that all
    // processes get updated at the same time.
    // ===== Primary branch:

    // Connect the geometry to the update suppressor.
    stream << vtkClientServerStream::Invoke << this->GeometryProxy->GetID(i)
          << "GetOutput" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorProxy->GetID(i) << "SetInput" 
          << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

    // Now create the mapper.
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i) 
          << "UseLookupTableScalarRangeOn" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i) << "InterpolateScalarsBeforeMappingOn" 
          << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorProxy->GetID(i) << "GetPolyDataOutput" 
          <<  vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i) << "SetInput" 
          << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i)
          << "SetImmediateModeRendering" 
          << pm->GetUseImmediateMode() << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
      
    // I used to use ambient 0.15 and diffuse 0.85, but VTK did not
    // handle it correctly.
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
           << "SetAmbient" << 0.0 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
           << "SetDiffuse" << 1.0 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
           << "SetSpecular" << 0.1 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
           << "SetSpecularPower" << 100.0 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
           << "SetSpecularColor" << 1.0 << 1.0 << 1.0 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropProxy->GetID(i)
           << "SetProperty" << this->PropertyProxy->GetID(i) << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropProxy->GetID(i)
           << "SetMapper" << this->MapperProxy->GetID(i)  << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);

    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
    
    stream << vtkClientServerStream::Invoke << this->VolumeProxy->GetID(i) 
          << "VisibilityOff" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeProxy->GetID(i)
          << "SetMapper" << this->VolumeMapperProxy->GetID(i)  
          << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeProxy->GetID(i) 
          << "SetProperty" << this->VolumePropertyProxy->GetID(i)
          << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke << this->VolumePropertyProxy->GetID(i) 
          << "SetScalarOpacity" << this->VolumeOpacityProxy->GetID(i)  
          << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumePropertyProxy->GetID(i)
          << "SetColor" << this->VolumeColorProxy->GetID(i) 
          << vtkClientServerStream::End;
    
    stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(i)
          << "RemoveAllPoints"  << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(i)
          << "SetColorSpaceToHSV"  << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(i)
          << "HSVWrapOff"  << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeOpacityProxy->GetID(i)
          << "RemoveAllPoints" << vtkClientServerStream::End;
    
    pm->SendStream(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER, stream);
    }
    
  //int fixme; //GetRid of these ivars
  if (num == 0)
    {
    vtkErrorMacro("Connecting a display to a source that has no outputs.");
    }
  else
    {
    this->Volume = 
      vtkVolume::SafeDownCast(
        pm->GetObjectFromID(this->VolumeProxy->GetID(0)));
    this->VolumeOpacity = 
      vtkPiecewiseFunction::SafeDownCast(
        pm->GetObjectFromID(this->VolumeOpacityProxy->GetID(0)));
    this->VolumeColor = 
      vtkColorTransferFunction::SafeDownCast(
        pm->GetObjectFromID(this->VolumeColorProxy->GetID(0)));
    }
}
  
//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetInput(vtkSMSourceProxy* input)
{
  if (input == NULL)
    {
    vtkWarningMacro("Trying to set a NULL input.");
    return;
    }

  if (this->Source)
    {
    vtkErrorMacro("Input set already");
    return;
    }  

  input->AddConsumer(0, this);  
  this->SetSource(input);

  // We need some flags (client flag) from process module subclass.
  vtkPVProcessModule* pm = 
    vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  this->SetInputInternal(input, pm);
}

void vtkSMPartDisplay::SetInputInternal(vtkSMSourceProxy *input,
                                        vtkPVProcessModule *pm)
{
  vtkClientServerStream stream;
  // Now that we know how many server objects to create, finish
  // creating them and setting them up.
  int i, num = 0;
  if (input)
    {
    num = input->GetNumberOfParts();
    if (!num)
      {
      input->CreateParts();
      num = input->GetNumberOfParts();
      }
    }
  
  this->CreateVTKObjects(num);

  for ( i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke 
           << this->GeometryProxy->GetID(i) <<  "SetInput" 
           << input->GetPart(i)->GetID(0) << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  for (i = 0; i < num; ++i)
    {
    vtkPVClassNameInformation* cnInfo =
      input->GetPart(i)->GetClassNameInformation();
    if (cnInfo->GetVTKClassName() &&
        strcmp(cnInfo->GetVTKClassName(), "vtkUnstructuredGrid") == 0 )
      {
      // Must loop through inputs .... fixme
      stream << vtkClientServerStream::Invoke << this->VolumeTetraFilterProxy->GetID(i)
             << "SetInput" << input->GetPart(i)->GetID(0)
             << vtkClientServerStream::End;
    
      stream << vtkClientServerStream::Invoke << this->VolumeTetraFilterProxy->GetID(i)
             << "GetOutput" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << this->VolumeMapperProxy->GetID(i)
             << "SetInput" <<  vtkClientServerStream::LastResult 
             << vtkClientServerStream::End;

      }
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetUseImmediateMode(int val)
{
  this->UseImmediateModeProperty->SetElement(0, val);
  if (this->MapperProxy)
    {
    this->MapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetVisibility(int v)
{
  if (v)
    {
    v = 1;
    }
  if (v == this->Visibility)
    {
    return;
    }
  this->Visibility = v;

  if ( ! v)
    {
    this->PropVisibilityProperty->SetElement(0,0);
    this->VolumeVisibilityProperty->SetElement(0,0);
    }
  else if (this->VolumeRenderMode)
    {  
    this->PropVisibilityProperty->SetElement(0,0);
    this->VolumeVisibilityProperty->SetElement(0,1);
    }
  else
    {
    this->PropVisibilityProperty->SetElement(0,1);
    this->VolumeVisibilityProperty->SetElement(0,0);
    }
  if (this->PropProxy)
    {
    this->PropProxy->UpdateVTKObjects();
    this->VolumeProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::GetColor(float *rgb)
{
  rgb[0] = this->ColorProperty->GetElement(0);
  rgb[1] = this->ColorProperty->GetElement(1);
  rgb[2] = this->ColorProperty->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetColor(float r, float g, float b)
{
  this->ColorProperty->SetElement(0, r);
  this->ColorProperty->SetElement(1, g);
  this->ColorProperty->SetElement(2, b);
  if (this->PropertyProxy == 0)
    {
    return;
    }
  this->PropertyProxy->UpdateVTKObjects();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // I do not want to create properties for specular, ambient and diffuse.
  int i, num;
  num = this->PropertyProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
          << "SetSpecular" << 0.1 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
          << "SetSpecularPower" << 100.0 << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
          << "SetSpecularColor" << 1.0 << 1.0 << 1.0 << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}  

//----------------------------------------------------------------------------
void vtkSMPartDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorProxy != 0 )
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->UpdateSuppressorProxy->GetID(0) << "ForceUpdate" 
           << vtkClientServerStream::End;
    this->SendForceUpdate(&stream);
    this->GeometryIsValid = 1;
    this->GeometryInformationIsValid = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::RemoveAllCaches()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorProxy->GetID(0)
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
}

//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMPartDisplay::CacheUpdate(int idx, int total)
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0)
         << "CacheUpdate" 
         << idx 
         << total 
         << vtkClientServerStream::End;
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  stream << vtkClientServerStream::Invoke 
         << this->MapperProxy->GetID(0) << "Modified"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetScalarVisibility(int val)
{  
  this->ScalarVisibilityProperty->SetElement(0,val);
  if (this->MapperProxy)
    {
    this->MapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMPartDisplay::GetScalarVisibility()
{  
  return this->ScalarVisibilityProperty->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetDirectColorFlag(int val)
{
  if (val)
    {
    val = VTK_COLOR_MODE_DEFAULT;
    }
  else
    {
    val = VTK_COLOR_MODE_MAP_SCALARS;
    }
    
  this->DirectColorFlagProperty->SetElement(0,val);
  if (this->MapperProxy)
    {
    this->MapperProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMPartDisplay::GetDirectColorFlag()
{
  int mode = this->DirectColorFlagProperty->GetElement(0);
  if (mode == VTK_COLOR_MODE_DEFAULT)
    {
    return 1;
    }
  else if (mode == VTK_COLOR_MODE_MAP_SCALARS)
    {
    return 0;
    }
  vtkErrorMacro("Unknown color mode: " << mode );
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetInterpolateColorsFlag(int val)
{
  // This is "InterpolateColors" while VTK method is
  // "InterpolateColorsBeforeMapping". These are opposite concepts.
  if (val)
    {
    val = 0;
    }
  else
    {
    val = 1;
    }
  this->InterpolateColorsFlagProperty->SetElement(0,val);
  if (this->MapperProxy)
    {
    this->MapperProxy->UpdateVTKObjects();
    }
}
  
//----------------------------------------------------------------------------
int vtkSMPartDisplay::GetInterpolateColorsFlag()
{
  int val = this->InterpolateColorsFlagProperty->GetElement(0);

  return ! val;
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetLineWidth(double w)
{
  this->LineWidthProperty->SetElement(0,w);
  if (this->PropertyProxy)
    {
    this->PropertyProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
double vtkSMPartDisplay::GetLineWidth()
{
  return this->LineWidthProperty->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetPointSize(double s)
{  
  this->PointSizeProperty->SetElement(0,s);
  if (this->PropertyProxy)
    {
    this->PropertyProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
double vtkSMPartDisplay::GetPointSize()
{
  return this->PointSizeProperty->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetInterpolation(int interpolation)
{
  this->InterpolationProperty->SetElement(0,interpolation);
  if (this->PropertyProxy)
    {
    this->PropertyProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMPartDisplay::GetInterpolation()
{
  return this->InterpolationProperty->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::ColorByArray(vtkSMProxy* colorMap,
                                    int field)
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

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

  this->ScalarVisibilityProperty->SetElement(0, 1);
  this->MapperProxy->UpdateVTKObjects();
  // Turn off the specualr so it does not interfere with data.
  int num, i;
  num = this->MapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
          << "SetSpecular" << 0.0 << vtkClientServerStream::End;
    if (colorMap->GetSubProxy("LookupTable"))
      {
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i)
          << "SetLookupTable" 
          << colorMap->GetSubProxy("LookupTable")->GetID(0)
          << vtkClientServerStream::End;
      }
    else
      {
      vtkErrorMacro("LookupTable not present as a subproxy for colorMap");
      }
    if (field == vtkDataSet::CELL_DATA_FIELD)
      { 
      stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i)
            << "SetScalarModeToUseCellFieldData" << vtkClientServerStream::End;
      }
    else if (field == vtkDataSet::POINT_DATA_FIELD)
      {
      stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i)
            << "SetScalarModeToUsePointFieldData" << vtkClientServerStream::End;
      }
    else
      {
      vtkErrorMacro("Only point or cell field please.");
      }
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetUseTriangleStrips(int val)
{
  this->UseTriangleStripsProperty->SetElement(0,val);

  if (this->GeometryProxy)
    {
    this->GeometryProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMPartDisplay::GetUseTriangleStrips()
{
  return this->UseTriangleStripsProperty->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMPartDisplay::SaveInBatchScript(ofstream *file, vtkSMSourceProxy* pvs) 
{
  *file << endl;
  *file << "set pvTemp" <<  this->GeometryProxy->GetID(0)
        << " [$proxyManager NewProxy rendering DefaultDisplayer]"
        << endl;
  *file << "  $proxyManager RegisterProxy rendering pvTemp"
        << this->GeometryProxy->GetID(0) << " $pvTemp" << this->GeometryProxy->GetID(0) 
        << endl;
  *file << "  $pvTemp" << this->GeometryProxy->GetID(0) << " UnRegister {}" << endl;

  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) << " GetProperty Input] "
        << " AddProxy $pvTemp" << pvs->GetID(0)
        << endl;

  *file << "  [$Ren1 GetProperty Displayers] AddProxy $pvTemp" 
        << this->GeometryProxy->GetID(0) << endl;
    
  if ( this->Representation == VTK_OUTLINE )
    {
    *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
          << " GetProperty DisplayAsOutline] SetElements1 1"  << endl;
    }
  else
    {
    *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
          << " GetProperty DisplayAsOutline] SetElements1 0"  << endl;
    }
  
  // Always use immediate mode rendering with batch.
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty ImmediateModeRendering] SetElements1 "  
        << 1 << endl;
 
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty ScalarVisibility] SetElements1 "  
        << this->GetScalarVisibility()
        << endl;

  int colorMode = VTK_COLOR_MODE_MAP_SCALARS;
  if (this->GetDirectColorFlag())
    {
    colorMode = VTK_COLOR_MODE_DEFAULT;
    }
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty ColorMode] SetElements1 "  
        << colorMode << endl;
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty InterpolateColorsBeforeMapping] SetElements1 "  
        << ! this->GetInterpolateColorsFlag()
        << endl;
  // I use a special value as outline representaiton. Displayer does n ot reconize it.
  if ( this->Representation != VTK_OUTLINE )
    {
    *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
          << " GetProperty Representation] SetElements1 "  
          << this->GetRepresentation()
          << endl;
    }
    
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Interpolation] SetElements1 "  
        << this->GetInterpolation()
        << endl;
  
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty LineWidth] SetElements1 "  
        << this->GetLineWidth()
        << endl;
    
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty PointSize] SetElements1 "  
        << this->GetPointSize()
        << endl;

  double tmp[3];
  this->GetTranslate(tmp);
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Position] SetElements3 "  
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << " " << endl;

  this->GetScale(tmp);
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Scale] SetElements3 "  
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << " " << endl;

  this->GetOrientation(tmp);
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Orientation] SetElements3 "  
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << " " << endl;

  this->GetOrigin(tmp);
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Origin] SetElements3 "  
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << " " << endl;

  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Opacity] SetElements1 "  
        << this->GetOpacity() << endl;
  
  *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty Color] SetElements3 "  
        << this->ColorProperty->GetElement(0) << " " 
        << this->ColorProperty->GetElement(1) << " " 
        << this->ColorProperty->GetElement(2) << " " << endl;

   if (this->ColorMap && this->GetScalarVisibility())
    {
    *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
          << " GetProperty LookupTable] AddProxy" 
          << " $pvTemp" << this->ColorMap->GetID(0)
         // << " [[$pvTemp" << this->ColorMap->GetID(0)
         // << " GetProperty LookupTableProxy] GetProxy 0]"
          << endl;
    int scalarMode = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
    if (this->ColorField == vtkDataSet::CELL_DATA_FIELD)
      {
      scalarMode = VTK_SCALAR_MODE_USE_CELL_FIELD_DATA;
      }
    *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
          << " GetProperty ScalarMode] SetElement 0 " 
          << scalarMode << endl;
   
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->ColorMap->GetProperty("ArrayName"));
    if (svp)
      {
      *file << "  [$pvTemp" << this->GeometryProxy->GetID(0) 
        << " GetProperty ColorArray] SetElement 0 {" 
        << svp->GetElement(0)
        << "}" << endl;
      }
    }

  *file << "  $pvTemp" << this->GeometryProxy->GetID(0) << " UpdateVTKObjects" 
        << endl;
}

//-----------------------------------------------------------------------------
void vtkSMPartDisplay::SaveGeometryInBatchFile(ofstream *file, 
                                               const char* fileName,
                                               int timeIdx) 
{
  //law int fixme;  //Make sure this works.  timeIdx is not used.
  timeIdx = timeIdx;
  *file << "GeometryWriter SetInput [pvTemp" 
        << this->GeometryProxy->GetID(0) << " GetOutput]\n";
  *file << "if {$numberOfProcs > 1} {\n";
  *file << "\tGeometryWriter SetFileName {" << fileName << ".pvtp}\n";
  *file << "} else {\n";
  *file << "\tGeometryWriter SetFileName {" << fileName << ".vtp}\n";
  *file << "}\n";
  *file << "GeometryWriter Write\n";

  *file << "CollectionFilter SetInput [pvTemp" 
        << this->GeometryProxy->GetID(0) << " GetOutput]\n";
  *file << "[CollectionFilter GetOutput] Update\n";
  *file << "TempPolyData ShallowCopy [CollectionFilter GetOutput]\n";
  *file << "if {$myProcId == 0} {\n";
  *file << "\tGeometryWriter SetFileName {" << fileName << ".vtp}\n";
  *file << "\tGeometryWriter Write\n";
  *file << "}\n";
}


//----------------------------------------------------------------------------
void vtkSMPartDisplay::VolumeRenderModeOn()
{
  if ( this->Visibility )
    {
    // This may not be proper to share visibility property.
    if ( this->PropProxy )
      {
      this->PropVisibilityProperty->SetElement(0,0);
      this->PropProxy->UpdateVTKObjects();
      }
    if ( this->VolumeProxy )
      {
      this->VolumeVisibilityProperty->SetElement(0,1);
      this->VolumeProxy->UpdateVTKObjects();
      }
    }
  
  this->VolumeRenderMode = 1;
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::VolumeRenderModeOff()
{
  if ( this->Visibility )
    {
    if ( this->PropProxy )
      {
      this->PropVisibilityProperty->SetElement(0,1);
      this->PropProxy->UpdateVTKObjects();
      }
    if ( this->VolumeProxy )
      {
      this->VolumeVisibilityProperty->SetElement(0,0);
      this->VolumeProxy->UpdateVTKObjects();
      }
    }
  
  this->VolumeRenderMode = 0;
}


//----------------------------------------------------------------------------
void vtkSMPartDisplay::InitializeTransferFunctions(vtkPVArrayInformation *arrayInfo,
                                                   vtkPVDataInformation *dataInfo )
{
  // need to initialize only if there are no points in the function
  if ( this->VolumeOpacity->GetSize() == 0 )
    {
    this->ResetTransferFunctions(arrayInfo, dataInfo);
    }
  
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::ResetTransferFunctions(vtkPVArrayInformation *arrayInfo, 
                                              vtkPVDataInformation *dataInfo)
{
  this->SetVolumeRenderField(arrayInfo->GetName());

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
  double unitDistance = diameter;
  if (linearNumCells != 0.0)
    {
    unitDistance = diameter / linearNumCells;
    }
  this->OpacityUnitDistance = unitDistance;
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityProxy->GetID(0)
         << "RemoveAllPoints" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityProxy->GetID(0)
         << "AddPoint" << range[0] << 0.0 << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeOpacityProxy->GetID(0)
         << "AddPoint" << range[1] << 1.0 << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(0)
         << "RemoveAllPoints" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(0)
         << "AddHSVPoint" << range[0] << 0.667 << 1.0 << 1.0 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(0)
         << "AddHSVPoint" << range[1] << 0.0 << 1.0 << 1.0 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(0)
         << "SetColorSpaceToHSV" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->VolumeColorProxy->GetID(0)
          << "HSVWrapOff"  << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke << this->VolumePropertyProxy->GetID(0)
         << "SetScalarOpacityUnitDistance" << unitDistance << vtkClientServerStream::End;
  
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::VolumeRenderPointField(const char *name)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  vtkClientServerStream stream;

  this->ColorField = vtkDataSet::POINT_DATA_FIELD;

  int i, num;
  num = this->VolumeMapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << this->VolumeMapperProxy->GetID(i)
           << "SetScalarModeToUsePointFieldData"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeMapperProxy->GetID(i)
           << "SelectScalarArray" << name << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::VolumeRenderCellField(const char *name)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if ( !pm )
    {
    vtkErrorMacro("Set the ProcessModule before you connect.");
    return;
    }
  vtkClientServerStream stream;

  this->ColorField = vtkDataSet::CELL_DATA_FIELD;

  int i, num;
  num = this->VolumeMapperProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << this->VolumeMapperProxy->GetID(i)
           << "SetScalarModeToUseCellFieldData"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->VolumeMapperProxy->GetID(i)
           << "SelectScalarArray" << name << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

}



//----------------------------------------------------------------------------
// Setting representation involves too many 
// objects and methods to use properties.
void vtkSMPartDisplay::SetRepresentation(int rep)
{
  if (rep == this->Representation)
    {
    return;
    }
  if (this->GeometryProxy == 0)
    {
    return;
    }

  // Volume is handled separately.
  if (rep == VTK_VOLUME)
    {
    this->VolumeRenderModeOn();
    this->Representation = rep;
    return;
    }
  this->VolumeRenderModeOff();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  int num, i;
  num = this->GeometryProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Geometry filter handles outline representation (not property).
    if ( this->Representation == VTK_OUTLINE)
      { // Changing from outline.
      stream 
          << vtkClientServerStream::Invoke
          << this->GeometryProxy->GetID(i)
          << "SetUseOutline" << 0 << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      this->InvalidateGeometry();
      }
    if (rep == VTK_OUTLINE)
      { // Moving to outline
      stream 
          << vtkClientServerStream::Invoke
          << this->GeometryProxy->GetID(i)
          << "SetUseOutline" << 1 << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      this->InvalidateGeometry();
      }
    // Handle specularity and lighting. All but surface turns shading off.
    float diffuse = 0.0;
    float ambient = 1.0;
    float specularity = 0.0;
    if (rep == VTK_SURFACE)
      {
      diffuse = 1.0;
      ambient = 0.0;
      // Turn on specularity when coloring by property.
      if ( ! this->GetScalarVisibility())
        {

        specularity = 0.1;
        }
      }
    stream 
      << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
      << "SetAmbient" << ambient << vtkClientServerStream::End;
    stream 
      << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
      << "SetDiffuse" << diffuse << vtkClientServerStream::End;
    stream 
      << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
      << "SetSpecular" << specularity << vtkClientServerStream::End;
        
    // Surface 
    stream 
      << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
      << "SetRepresentationToSurface" << vtkClientServerStream::End;

    // Wireframe
    if (rep == VTK_WIREFRAME)
      {
      stream 
        << vtkClientServerStream::Invoke << this->PropertyProxy->GetID(i)
        << "SetRepresentationToWireframe" << vtkClientServerStream::End;
      }
      
    // Points
    if (rep == VTK_POINTS)
      {
      stream 
        << vtkClientServerStream::Invoke 
        << this->PropertyProxy->GetID(i)
        << "SetRepresentationToPoints" << vtkClientServerStream::End;
      }
      
    // All the changes to property get sent to render server and client.  
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    }
  

  // At the end on purpose.  Eliminates unecessary sends.
  this->Representation = rep;
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetTranslate(double x, double y, double z)
{
  this->TranslateProperty->SetElement(0, x);
  this->TranslateProperty->SetElement(1, y);
  this->TranslateProperty->SetElement(2, z);
  if (this->PropertyProxy)
    {
    this->PropProxy->UpdateVTKObjects();
    }
  if (this->VolumeProxy)
    {
    this->VolumeProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
void vtkSMPartDisplay::GetTranslate(double *position)
{
  position[0] = this->TranslateProperty->GetElement(0);
  position[1] = this->TranslateProperty->GetElement(1);
  position[2] = this->TranslateProperty->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetOpacity(double opacity)
{
  this->OpacityProperty->SetElement(0, opacity);
  if (this->PropertyProxy)
    {
    this->PropertyProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
double vtkSMPartDisplay::GetOpacity()
{
  return this->OpacityProperty->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetScale(double x, double y, double z)
{
  this->ScaleProperty->SetElement(0, x);
  this->ScaleProperty->SetElement(1, y);
  this->ScaleProperty->SetElement(2, z);
  if (this->PropertyProxy)
    {
    this->PropProxy->UpdateVTKObjects();
    }
  if (this->VolumeProxy)
    {
    this->VolumeProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
void vtkSMPartDisplay::GetScale(double *scale)
{
  scale[0] = this->ScaleProperty->GetElement(0);
  scale[1] = this->ScaleProperty->GetElement(1);
  scale[2] = this->ScaleProperty->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetOrientation(double x, double y, double z)
{
  this->OrientationProperty->SetElement(0, x);
  this->OrientationProperty->SetElement(1, y);
  this->OrientationProperty->SetElement(2, z);
  if (this->PropertyProxy)
    {
    this->PropProxy->UpdateVTKObjects();
    }
  if (this->VolumeProxy)
    {
    this->VolumeProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
void vtkSMPartDisplay::GetOrientation(double *orientation)
{
  orientation[0] = this->OrientationProperty->GetElement(0);
  orientation[1] = this->OrientationProperty->GetElement(1);
  orientation[2] = this->OrientationProperty->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SetOrigin(double x, double y, double z)
{
  this->OriginProperty->SetElement(0, x);
  this->OriginProperty->SetElement(1, y);
  this->OriginProperty->SetElement(2, z);
  if (this->PropertyProxy)
    {
    this->PropProxy->UpdateVTKObjects();
    }
  if (this->VolumeProxy)
    {
    this->VolumeProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
void vtkSMPartDisplay::GetOrigin(double *origin)
{
  origin[0] = this->OriginProperty->GetElement(0);
  origin[1] = this->OriginProperty->GetElement(1);
  origin[2] = this->OriginProperty->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::ConnectGeometryForWriting(vtkClientServerID consumerID,
                                                 const char* methodName,
                                                 vtkClientServerStream* stream)
{
  //law int fixme;  
  // We need to write all the parts! Create a special SM object to do this.
  *stream << vtkClientServerStream::Invoke 
          << this->GeometryProxy->GetID(0)
          << "GetOutput"
         << vtkClientServerStream::End;
  *stream << vtkClientServerStream::Invoke << consumerID
          << methodName
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::AddToRenderer(vtkPVRenderModule* rm)
{
  vtkClientServerID rendererID = rm->GetRendererID();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int num, i; 
  num = this->PropProxy->GetNumberOfIDs();
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << rendererID << "AddViewProp"
            << this->PropProxy->GetID(i) << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << rendererID << "AddViewProp"
            << this->VolumeProxy->GetID(i) << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::RemoveFromRenderer(vtkPVRenderModule* rm)
{
  vtkClientServerID rendererID = rm->GetRendererID();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int num, i; 
  num = this->PropProxy->GetNumberOfIDs();
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke << rendererID 
           << "RemoveViewProp"
           << this->PropProxy->GetID(i) << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << rendererID 
           << "RemoveViewProp"
           << this->VolumeProxy->GetID(i) << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMPartDisplay::SendForceUpdate(vtkClientServerStream* stream)
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, *stream);
}


//----------------------------------------------------------------------------
vtkPVGeometryInformation* vtkSMPartDisplay::GetGeometryInformation()
{
  if (this->GeometryInformationIsValid == 0)
    {
    this->GatherGeometryInformation();
    }
  return this->GeometryInformation;
}

//----------------------------------------------------------------------------
// vtkPVPart used to update before gathering this information ...
void vtkSMPartDisplay::GatherGeometryInformation()
{
  this->GeometryInformation->Initialize();
  if (this->GeometryProxy->GetNumberOfIDs() < 1)
    {
    vtkErrorMacro("Display has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  pm->SendPrepareProgress();
  this->Update();
  pm->SendCleanupPendingProgress();

  int num, i;
  vtkPVGeometryInformation* information;
  num = this->GeometryProxy->GetNumberOfIDs();
  information = vtkPVGeometryInformation::New();
  for (i = 0; i < num; ++i)
    {
    pm->GatherInformation(information, this->GeometryProxy->GetID(i));
    this->GeometryInformation->AddInformation(information);
    }
  information->Delete();
  // Skip generation of names.
  this->GeometryInformationIsValid = 1;
}


//----------------------------------------------------------------------------
// This should be handle the same way batch is (in the future).
void vtkSMPartDisplay::SavePVState(ostream *file, const char* tclName, 
                                 vtkIndent indent)
{
  float rgb[3];
  this->GetColor(rgb);
  *file << indent << tclName << " SetColor " 
        << rgb[0] << " " << rgb[1] << " " << rgb[2] << endl;
     
  *file << indent << tclName << " SetRepresentation " << 
    this->GetRepresentation() << endl; 
  *file << indent << tclName << " SetUseImmediateMode " << 
    this->UseImmediateModeProperty->GetElement(0) << endl; 
  *file << indent << tclName << " SetScalarVisibility " << 
    this->GetScalarVisibility() << endl; 
  *file << indent << tclName << " SetDirectColorFlag " << 
    this->GetDirectColorFlag() << endl; 
  *file << indent << tclName << " SetInterpolateColorsFlag " << 
    this->GetInterpolateColorsFlag() << endl; 
  *file << indent << tclName << " SetInterpolation " << 
    this->GetInterpolation() << endl; 
  *file << indent << tclName << " SetLineWidth " << 
    this->GetLineWidth() << endl; 
  *file << indent << tclName << " SetPointSize " << 
    this->GetPointSize() << endl; 
  double tmp[3];
  this->GetTranslate(tmp);
  *file << indent << tclName << " SetTranslate " 
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
  this->GetScale(tmp);
  *file << indent << tclName << " SetScale " 
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
  this->GetOrientation(tmp);
  *file << indent << tclName << " SetOrientation " 
        << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
  this->GetOrigin(tmp);
    *file << indent << tclName << " SetOrigin " 
          << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
  *file << indent << tclName << " SetOpacity " << this->GetOpacity() << endl; 
}



//----------------------------------------------------------------------------
void vtkSMPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: "   << this->GetVisibility()    << endl;
  os << indent << "Source: "       << this->Source          << endl;
  os << indent << "MapperProxy: "  << this->MapperProxy   << endl;
  os << indent << "PropProxy: "    << this->PropProxy     << endl;
  os << indent << "PropertyProxy: " << this->PropertyProxy << endl;

  os << indent << "LineWidth: "    << this->GetLineWidth() << endl;
  os << indent << "PointSize: "    << this->GetPointSize() << endl;
  os << indent << "Interpolation: " << this->GetInterpolation() << endl;
  os << indent << "Representation: " << this->Representation << endl;
  os << indent << "UseTriangleStrips: " << this->GetUseTriangleStrips() << endl;
  
  double tmp[3];
  this->GetScale(tmp);
  os << indent << "Scale: " << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << endl;
  this->GetTranslate(tmp);
  os << indent << "Translate: " << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << endl;
  this->GetOrientation(tmp);
  os << indent << "Orientation: " << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << endl;
  this->GetOrigin(tmp);
  os << indent << "Origin: " << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << endl;
  os << indent << "Color: " << this->ColorProperty->GetElement(0) << ", " 
     << this->ColorProperty->GetElement(1) << ", " 
     << this->ColorProperty->GetElement(2) << endl;
  
  os << indent << "ColorField: " << this->ColorField << endl;
  os << indent << "DirectColorFlag: "  << this->GetDirectColorFlag()       << endl;
  os << indent << "InterpolateColorsFlag: " << this->GetInterpolateColorsFlag() << endl;
  os << indent << "OpacityUnitDistance: " << this->OpacityUnitDistance << endl;
  os << indent << "UpdateSuppressor: " << this->UpdateSuppressorProxy << endl;
  
  os << indent << "VolumeProxy: "            << this->VolumeProxy            << endl;
  os << indent << "VolumeMapperProxy: "      << this->VolumeMapperProxy      << endl;
  os << indent << "VolumePropertyProxy: "    << this->VolumePropertyProxy    << endl;
  os << indent << "VolumeOpacityProxy: "     << this->VolumeOpacityProxy     << endl;
  os << indent << "VolumeColorProxy: "       << this->VolumeColorProxy       << endl;
  os << indent << "VolumeTetraFilterProxy: " << this->VolumeTetraFilterProxy << endl;

  os << indent << "VolumeRenderMode: " << this->VolumeRenderMode << endl;
  os << indent << "VolumeRenderField: "
     << (this->VolumeRenderField ? this->VolumeRenderField : "(NULL)") << endl;
}

