/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiDisplayRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMMultiDisplayRenderModuleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkPVProcessModule.h"
#include "vtkPVOptions.h"

vtkStandardNewMacro(vtkSMMultiDisplayRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMMultiDisplayRenderModuleProxy, "1.1.2.1");
//-----------------------------------------------------------------------------
vtkSMMultiDisplayRenderModuleProxy::vtkSMMultiDisplayRenderModuleProxy()
{
  this->SetDisplayXMLName("MultiDisplay");
}

//-----------------------------------------------------------------------------
vtkSMMultiDisplayRenderModuleProxy::~vtkSMMultiDisplayRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::CreateCompositeManager()
{
  // Created in XML.
  this->GetSubProxy("CompositeManager")->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER);
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::InitializeCompositingPipeline()
{
  if (!this->CompositeManagerProxy)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp;

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("TileDimensions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find proeprty TileDimensions on CompositeManagerProxy.");
    return;
    }
  int *tileDim = pm->GetOptions()->GetTileDimensions();
  unsigned int i;
  ivp->SetElements(tileDim);
  this->CompositeManagerProxy->UpdateVTKObjects();

  vtkClientServerStream stream;
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    if (pm->GetOptions()->GetClientMode())
      {
      // Clean up this mess !!!!!!!!!!!!!
      // Even a cast to vtkPVClientServerModule would be better than this.
      // How can we syncronize the process modules and render modules?
      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetClientMode" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i) 
        << "SetClientFlag"
        << vtkClientServerStream::LastResult << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetRenderServerSocketController" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i)
        << "SetSocketController" << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke
        << this->CompositeManagerProxy->GetID(i) << "SetZeroEmpty" << 0
        << vtkClientServerStream::End;
      }
    else
      {
      stream << vtkClientServerStream::Invoke
        << this->CompositeManagerProxy->GetID(i) << "SetZeroEmpty" << 1
        << vtkClientServerStream::End;     
      }
    stream << vtkClientServerStream::Invoke
      << this->CompositeManagerProxy->GetID(i) << "InitializeSchedule"
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->CompositeManagerProxy->GetServers(), stream);
 
  this->Superclass::InitializeCompositingPipeline();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
