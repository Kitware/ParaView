/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCubeAxesDisplay.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModule.h"
#include "vtkCubeAxesActor2D.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCubeAxesDisplay);
vtkCxxRevisionMacro(vtkSMCubeAxesDisplay, "1.4");


//----------------------------------------------------------------------------
vtkSMCubeAxesDisplay::vtkSMCubeAxesDisplay()
{
  this->Visibility = 1;
  this->GeometryIsValid = 0;
  this->Input = 0;
  this->Caches = 0;
  this->NumberOfCaches = 0;

  this->CubeAxesProxy = vtkSMProxy::New();
  this->CubeAxesProxy->SetVTKClassName("vtkCubeAxesActor2D");
  this->CubeAxesProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkSMCubeAxesDisplay::~vtkSMCubeAxesDisplay()
{
  this->CubeAxesProxy->Delete();
  this->CubeAxesProxy = 0;
  
  // No reference counting for this ivar.
  this->Input = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::CreateVTKObjects(int num)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (num != 1)
    {
    vtkErrorMacro("Only one cube axes per source.");
    }
  if (this->CubeAxesProxy->GetNumberOfIDs() > 0)
    {
    vtkErrorMacro("Display already created.");
    return;
    }  
    
  this->CubeAxesProxy->CreateVTKObjects(1);
  vtkClientServerStream stream; 
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesProxy->GetID(0) << "SetFlyModeToOuterEdges"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
    << this->CubeAxesProxy->GetID(0) << "SetInertia" << 20
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::SetInput(vtkSMSourceProxy* input)
{  
  this->CreateVTKObjects(1);
  input->AddConsumer(0, this);
  
  // Hang onto the input since cube axes bounds are set manually.
  this->Input = input;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::AddToRenderer(vtkPVRenderModule* rm)
{
  if (!rm)
    {
    return;
    }  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerID rendererID = rm->GetRendererID();

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  int i, num;
  num = this->CubeAxesProxy->GetNumberOfIDs();
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke
           << rendererID << "AddViewProp"
           << this->CubeAxesProxy->GetID(i) << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << rendererID << "GetActiveCamera"
           << vtkClientServerStream::End;    
    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(i) << "SetCamera"
           << vtkClientServerStream::LastResult 
           << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::RemoveFromRenderer(vtkPVRenderModule* rm)
{
  if (!rm)
    {
    return;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerID rendererID = rm->GetRendererID();

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  vtkClientServerID nullId;
  nullId.ID = 0;
  int i, num;
  num = this->CubeAxesProxy->GetNumberOfIDs();
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke
          << rendererID << "RemoveViewProp"
           << this->CubeAxesProxy->GetID(i) << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(i) << "SetCamera"
           << nullId 
           << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::SetVisibility(int v)
{
  if (v)
    {
    v = 1;
    }
  if (v == this->Visibility)
    {
    return;
    }    
  this->GeometryIsValid = 0;  // so we can change the color
  this->Visibility = v;
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->CubeAxesProxy->GetNumberOfIDs() > 0)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(0) 
           << "SetVisibility" << v << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::Update()
{
  if (this->GeometryIsValid)
    {
    return;
    }
    
  double bounds[6];
  vtkPVProcessModule *pm;
  pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  vtkClientServerStream stream;
  vtkPVRenderModule* rm = pm->GetRenderModule();
  float rgb[3];
  float background[3];
  rgb[0] = rgb[1] = rgb[2] = 1.0;
  rm->GetBackgroundColor(background);
  // Change the color of the cube axes if the background is light.
  if (background[0] + background[1] + background[2] > 2.2)
    {
    rgb[0] = rgb[1] = rgb[2] = 0.0;
    }

  if (this->Input == 0)
    {
    return;
    }
  this->Input->UpdatePipeline();    
  vtkPVDataInformation* dataInfo = this->Input->GetDataInformation();
  dataInfo->GetBounds(bounds);
  int i, num;
  num = this->CubeAxesProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke 
           << this->CubeAxesProxy->GetID(i) << "SetBounds"
           << bounds[0] << bounds[1] << bounds[2]
           << bounds[3] << bounds[4] << bounds[5]
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(0) << "GetProperty"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult << "SetColor"
           << rgb[0] << rgb[1] << rgb[2]
           << vtkClientServerStream::End;
           
    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(0) << "GetAxisTitleTextProperty"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult << "SetColor"
           << rgb[0] << rgb[1] << rgb[2]
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke
           << this->CubeAxesProxy->GetID(0) << "GetAxisLabelTextProperty"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult << "SetColor"
           << rgb[0] << rgb[1] << rgb[2]
           << vtkClientServerStream::End;  
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
  this->GeometryIsValid = 1;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::RemoveAllCaches()
{
  if (this->NumberOfCaches == 0)
    {
    return;
    }
  int i;
  for (i = 0; i < this->NumberOfCaches; ++i)
    {
    if (this->Caches[i])
      {
      delete [] this->Caches[i];
      this->Caches[i] = 0;
      }
    }
  delete [] this->Caches;
  this->Caches = 0;
  this->NumberOfCaches = 0;
}

//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMCubeAxesDisplay::CacheUpdate(int idx, int total)
{
  int i;
  if (total != this->NumberOfCaches)
    {
    this->RemoveAllCaches();
    this->Caches = new double*[total];
    for (i = 0; i < total; ++i)
      {
      this->Caches[i] = 0;
      }
    this->NumberOfCaches = total;
    }

  if (this->Caches[idx] == 0)
    {
    this->Input->UpdatePipeline();
    vtkPVDataInformation* info = this->Input->GetDataInformation();
    this->Caches[idx] = new double[6];
    info->GetBounds(this->Caches[idx]);
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int num;
  num = this->CubeAxesProxy->GetNumberOfIDs();
  vtkClientServerStream stream; 
  for (i = 0; i < num; ++i)
    {
    stream << vtkClientServerStream::Invoke 
           << this->CubeAxesProxy->GetID(i) << "SetBounds"
           << this->Caches[idx][0] << this->Caches[idx][1] 
           << this->Caches[idx][2] << this->Caches[idx][3] 
           << this->Caches[idx][4] << this->Caches[idx][5]
           << vtkClientServerStream::End;
    }
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "CubeAxesProxy: " << this->CubeAxesProxy << endl;
}

