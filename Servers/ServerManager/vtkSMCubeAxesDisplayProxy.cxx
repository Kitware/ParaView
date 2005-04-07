/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCubeAxesDisplayProxy.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCubeAxesDisplayProxy);
vtkCxxRevisionMacro(vtkSMCubeAxesDisplayProxy, "1.1.2.7");


//----------------------------------------------------------------------------
vtkSMCubeAxesDisplayProxy::vtkSMCubeAxesDisplayProxy()
{
  this->Visibility = 1;
  this->GeometryIsValid = 0;
  this->Input = 0;
  this->Caches = 0;
  this->NumberOfCaches = 0;

  this->CubeAxesProxy = 0;
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMCubeAxesDisplayProxy::~vtkSMCubeAxesDisplayProxy()
{
  this->CubeAxesProxy = 0;
  
  // No reference counting for this ivar.
  this->Input = 0;
  this->RemoveAllCaches();
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::CreateVTKObjects(int num)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  
  if (num != 1)
    {
    vtkErrorMacro("Only one cube axes per source.");
    }
  this->CubeAxesProxy = this->GetSubProxy("CubeAxes");
  if (!this->CubeAxesProxy)
    {
    vtkErrorMacro("SubProxy CubeAxes must be defined.");
    return;
    }
  
  this->CubeAxesProxy->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
 
  this->Superclass::CreateVTKObjects(1);

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("FlyMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FlyMode.");
    return;
    }
  ivp->SetElement(0, 0); // FlyToOuterEdges.
  

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Inertia"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Inertia.");
    return;
    }
  ivp->SetElement(0, 20);

  this->CubeAxesProxy->UpdateVTKObjects(); 
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::AddInput(vtkSMSourceProxy* input, const char*, 
  int, int)
{
  this->SetInput(input);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::SetInput(vtkSMProxy* input)
{  
  this->CreateVTKObjects(1);
  //input->AddConsumer(0, this); This will happen automatically when
  //the caller uses ProxyProperty to add the input.
  
  // Hang onto the input since cube axes bounds are set manually.
  this->Input = vtkSMSourceProxy::SafeDownCast(input);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (!rm)
    {
    return;
    }  
  if (this->RenderModuleProxy)
    {
    vtkErrorMacro("Can be added only to one render module.");
    return;
    }
  /*
  vtkSMProxyProperty* pp; 
  pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->AddProxy(this->CubeAxesProxy);
  rm->UpdateVTKObjects();
  */
  rm->AddPropToRenderer2D(this->CubeAxesProxy);

  // We don't set the Camera proxy for the cube axes actor using 
  // properties since the Camera Proxy provided by the RenderModule is only 
  // on the CLIENT, and CubeAxesActor needs the camera on the servers as well.
  vtkClientServerStream stream;
  vtkSMProxy* renderer = rm->GetRenderer2DProxy();
  for (unsigned int i=0; i < this->CubeAxesProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
      << renderer->GetID(0)
      << "GetActiveCamera" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->CubeAxesProxy->GetID(i)
      << "SetCamera" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(
      this->CubeAxesProxy->GetServers(), stream);
    }
  this->RenderModuleProxy = rm;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  if (!rm || this->RenderModuleProxy != rm)
    {
    return;
    }
   vtkSMProxyProperty* pp;
  /*
  pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetRenderer2DProxy()->GetProperty("ViewProps"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ViewProps on vtkSMRenderModuleProxy.");
    return;
    }
  pp->RemoveProxy(this->CubeAxesProxy);
  rm->UpdateVTKObjects();
  */
  rm->RemovePropFromRenderer2D(this->CubeAxesProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Camera"));
  pp->RemoveAllProxies();
  this->CubeAxesProxy->UpdateVTKObjects(); 
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::SetVisibility(int v)
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
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Visibility"));
  ivp->SetElement(0, v);
  this->CubeAxesProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateGeometry();
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::Update()
{
  if (this->GeometryIsValid || !this->RenderModuleProxy)
    {
    return;
    }
    
  double bounds[6];
  vtkPVProcessModule *pm;
  pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  
  vtkClientServerStream stream;
  
  double rgb[3];
  double *background;
  rgb[0] = rgb[1] = rgb[2] = 1.0;

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("Background"));
  if (!dvp)
    {
    background = rgb;
    }
  background = dvp->GetElements();
  
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
  pm->SendStream(this->CubeAxesProxy->GetServers(), stream);
  this->GeometryIsValid = 1;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::RemoveAllCaches()
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
void vtkSMCubeAxesDisplayProxy::CacheUpdate(int idx, int total)
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
  pm->SendStream(this->CubeAxesProxy->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "CubeAxesProxy: " << this->CubeAxesProxy << endl;
}

