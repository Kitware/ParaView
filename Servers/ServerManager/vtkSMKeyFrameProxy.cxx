/*=========================================================================

  Program:   ParaView
  Module:    vtkSMKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerID.h"

vtkCxxRevisionMacro(vtkSMKeyFrameProxy, "1.2");
vtkStandardNewMacro(vtkSMKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy::vtkSMKeyFrameProxy()
{
  this->KeyValue = 0.0;
  this->KeyTime = -1.0;
  this->ObjectsCreated = 1; //no serverside objects for this proxy.
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy::~vtkSMKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::UpdateValue(double vtkNotUsed(currenttime), 
  vtkSMAnimationCueProxy* vtkNotUsed(cueProxy), vtkSMKeyFrameProxy* vtkNotUsed(next))
{
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  vtkClientServerID id = this->SelfID;
  *file << "set pvTemp" << id
    << " [$proxyManager NewProxy " 
    << this->GetXMLGroup() <<" "
    << this->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy "
    << this->GetXMLName()
    <<" pvTemp" << id << " $pvTemp" << id << endl;
  *file << "  $pvTemp" << id << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << id << " GetProperty KeyValue]"
    << " SetElements1 " << this->KeyValue << endl;
  *file << "  [$pvTemp" << id << " GetProperty KeyTime]"
    << " SetElements1 " << this->KeyTime << endl;
  *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KeyValue: " << this->KeyValue << endl;
  os << indent << "KeyTime: " << this->KeyTime << endl;
}
