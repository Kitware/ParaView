/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAxesProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAxesProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMAxesProxy);
vtkCxxRevisionMacro(vtkSMAxesProxy, "1.1");


//---------------------------------------------------------------------------
vtkSMAxesProxy::vtkSMAxesProxy()
{
}

//---------------------------------------------------------------------------
vtkSMAxesProxy::~vtkSMAxesProxy()
{
}


//---------------------------------------------------------------------------
void vtkSMAxesProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Superclass::CreateVTKObjects(numObjects);
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  int cc;

  vtkClientServerStream str;
  for (cc=0; cc< numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke << id 
      << "SymmetricOn" << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke << id
      << "ComputeNormalsOff" << vtkClientServerStream::End;
    }
  vtkSMProxy* actor = this->GetSubProxy("actor");
  if (!actor)
    {
    vtkErrorMacro("No actor sub-proxy was defined. Please make sure that"
      "the configuration file defines it.");
    }
  else
    {
    unsigned int i;
    unsigned int num = actor->GetNumberOfIDs();
    for (i=0; i < num; i++)
      {
      vtkClientServerID id = actor->GetID(i);
      str << vtkClientServerStream::Invoke << id
        << "VisibilityOff" << vtkClientServerStream::End;
      if (pm->GetRenderModule())
        {
        str << vtkClientServerStream::Invoke 
          << pm->GetRenderModule()->GetRendererID()
          << "AddActor" << id
          << vtkClientServerStream::End;
        }
      }
    }
  if (str.GetNumberOfMessages() > 0)
    {

    pm->SendStream(this->Servers,str,0);
    }
}

//---------------------------------------------------------------------------
void vtkSMAxesProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;
  
  for (cc=0; cc < numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << "set pvTemp" << id
      << " [$proxyManager NewProxy axes Axes]" << endl;
    *file << "  $proxyManager RegisterProxy axes pvTemp"
      << id << " $pvTemp" << id << endl;
    *file << "  $pvTemp" << id << " UnRegister {}" << endl;
    *file << "  [$Ren1 GetProperty Displayers] AddProxy $pvTemp"
      << id << endl;
  
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("Visibility"));
    if (ivp)
      {
      *file << "  [$pvTemp" << id << " GetProperty Visibility]"
        << " SetElements1 " << ivp->GetElement(0) << endl;
      }
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Position"));
    if (dvp)
      {
      *file << "  [$pvTemp" << id << " GetProperty Position]"
        << " SetElements3 " << dvp->GetElement(0) << " "
        << dvp->GetElement(1) << " " << dvp->GetElement(2) << endl;
      }
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Scale"));
    if (dvp)
      {
      *file << "  [$pvTemp" << id << " GetProperty Scale]"
        << " SetElements3 " << dvp->GetElement(0) << " "
        << dvp->GetElement(1) << " " << dvp->GetElement(2) << endl;
      }
    *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
    }
}

//---------------------------------------------------------------------------
void vtkSMAxesProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
