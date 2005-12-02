/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationCueManipulatorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationCueManipulatorProxy.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSMAnimationCueManipulatorProxy, "1.5");

//----------------------------------------------------------------------------
vtkSMAnimationCueManipulatorProxy::vtkSMAnimationCueManipulatorProxy()
{
  this->ObjectsCreated = 1; //since this class create no serverside objects.
}

//----------------------------------------------------------------------------
vtkSMAnimationCueManipulatorProxy::~vtkSMAnimationCueManipulatorProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::Copy(vtkSMProxy* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  this->Superclass::Copy(src, exceptionClass, proxyPropertyCopyFlag);
  this->MarkAllPropertiesAsModified();
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  *file << "set " << this->GetName() 
    << " [$proxyManager NewProxy " << this->GetXMLGroup()
    << " " << this->GetXMLName() << "]" << endl;
  *file << "$" << this->GetName() << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
