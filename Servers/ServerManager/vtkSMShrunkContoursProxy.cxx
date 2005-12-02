/*=========================================================================

  Program:   ParaView
  Module:    vtkSMShrunkContoursProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMShrunkContoursProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMShrunkContoursProxy);
vtkCxxRevisionMacro(vtkSMShrunkContoursProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMShrunkContoursProxy::vtkSMShrunkContoursProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMShrunkContoursProxy::~vtkSMShrunkContoursProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMShrunkContoursProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  
  vtkSMProxy* shrink = vtkSMProxy::SafeDownCast(this->GetSubProxy("Shrink"));
  if (!shrink)
    {
    vtkErrorMacro("Subproxy Shrink must be defined in XML.");
    return;
    }
  
  this->Superclass::CreateVTKObjects(numObjects);
  
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  for (int i=0; i < numObjects; i++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(i)
      << "GetOutput" << 0
      << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << shrink->GetID(i)
      << "SetInput" 
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, this->Servers, str, 0);

}

//-----------------------------------------------------------------------------
void vtkSMShrunkContoursProxy::CreateParts()
{
  if (this->PartsCreated && this->GetNumberOfParts())
    {
    return;
    }
  this->CreateVTKObjects(1);
  if (!this->ObjectsCreated)
    {
    return;
    }
  this->CreatePartsInternal(this->GetSubProxy("Shrink"));
}

//-----------------------------------------------------------------------------
void vtkSMShrunkContoursProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

