/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCaveRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCaveRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"

vtkStandardNewMacro(vtkSMCaveRenderViewProxy);

//-----------------------------------------------------------------------------
vtkSMCaveRenderViewProxy::vtkSMCaveRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMCaveRenderViewProxy::~vtkSMCaveRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::SetStereoRender(int val)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RendererProxy->GetID()
         << "GetRenderWindow"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "SetStereoRender"
         << val
         << vtkClientServerStream::End;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream( this->ConnectionID, vtkProcessModule::RENDER_SERVER |
                 vtkProcessModule::CLIENT, stream );
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::SetStereoTypeToAnaglyph()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RendererProxy->GetID()
         << "GetRenderWindow"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "SetStereoTypeToAnaglyph"
         << vtkClientServerStream::End;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream( this->ConnectionID, vtkProcessModule::RENDER_SERVER |
                 vtkProcessModule::CLIENT, stream );
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::HeadTracked(int val)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->RendererProxy->GetID()
         << "GetActiveCamera"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "SetHeadTracked"
         << val
         << vtkClientServerStream::End;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream( this->ConnectionID, vtkProcessModule::RENDER_SERVER , stream);
  // vtkProcessModule::CLIENT, stream );
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::SetHeadPose(
  double x00, double x01, double x02, double x03,
  double x10, double x11, double x12, double x13,
  double x20, double x21, double x22, double x23,
  double x30, double x31, double x32, double x33)
{
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->RendererProxy->GetID()
           << "GetActiveCamera"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "SetHeadPose"
           << x00 << x01 << x02 << x03
           << x10 << x11 << x12 << x13
           << x20 << x21 << x22 << x23
           << x30 << x31 << x32 << x33
           << vtkClientServerStream::End;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream( this->ConnectionID, vtkProcessModule::RENDER_SERVER |
                    vtkProcessModule::CLIENT, stream );
    this->StillRender();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::ConfigureRenderManagerFromServerInformation()
{
  this->HeadTracked( 1 );
  this->SetStereoRender(1);
  this->SetStereoTypeToAnaglyph();
  // this->SetHeadPose( 1.0 , 0.0, 0.0, 0.0,
  //                    0.0 , 1.0, 0.0, 0.0,
  //                    0.0 , 0.0, 1.0, 0.0,
  //                    0.0 , 0.0, 0.0, 1.0 );
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(
    this->ConnectionID);

  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("NumberOfDisplays"));
  if (ivp)
    {
    ivp->SetElements1(numMachines);
    }
  // the property must be applied to the vtkobject before
  // the property Displays is set.
  this->ParallelRenderManager->UpdateProperty("NumberOfDisplays");

  double* displays = new double[ numMachines*10];
  for (idx = 0; idx < numMachines; idx++)
    {
    displays[idx*10+0] = idx;
    displays[idx*10+1] = serverInfo->GetLowerLeft(idx)[0];
    displays[idx*10+2] = serverInfo->GetLowerLeft(idx)[1];
    displays[idx*10+3] = serverInfo->GetLowerLeft(idx)[2];
    displays[idx*10+4] = serverInfo->GetLowerRight(idx)[0];
    displays[idx*10+5] = serverInfo->GetLowerRight(idx)[1];
    displays[idx*10+6] = serverInfo->GetLowerRight(idx)[2];
    displays[idx*10+7] = serverInfo->GetUpperLeft(idx)[0];
    displays[idx*10+8] = serverInfo->GetUpperLeft(idx)[1];
    displays[idx*10+9] = serverInfo->GetUpperLeft(idx)[2];
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ParallelRenderManager->GetProperty("Displays"));
  if (dvp)
    {
    dvp->SetElements(displays, numMachines*10);
    }
  this->ParallelRenderManager->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->ConfigureRenderManagerFromServerInformation();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
