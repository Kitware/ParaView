/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImplicitPlaneProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMImplicitPlaneProxy);
vtkCxxRevisionMacro(vtkSMImplicitPlaneProxy, "1.4");

//----------------------------------------------------------------------------
vtkSMImplicitPlaneProxy::vtkSMImplicitPlaneProxy()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Offset = 0;
}

//----------------------------------------------------------------------------
vtkSMImplicitPlaneProxy::~vtkSMImplicitPlaneProxy()
{  
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  double origin[3];

  vtkSMDoubleVectorProperty* normal = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Normal"));
  if (!normal || normal->GetNumberOfElements() != 3)
    {
    vtkErrorMacro("A Normal property with 3 components could not be found. "
                  "Please make sure that the configuration file is correct.");
    return;
    }
  unsigned int i;
  for (i=0; i<3; i++)
    {
    origin[i] = this->Origin[i] + this->Offset*normal->GetElement(i);
    }

  vtkClientServerStream str;
  unsigned int numObjects = this->GetNumberOfIDs();
  for (i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke
        << this->GetID(i) << "SetOrigin" << origin[0] << origin[1] << origin[2]
        << vtkClientServerStream::End;
    }

  if (str.GetNumberOfMessages() > 0)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream(this->Servers, str);
    }

}
 
//----------------------------------------------------------------------------
void vtkSMImplicitPlaneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Offset: " << this->Offset << endl;
  os << indent << "Origin: " << this->Origin[0] << "," 
                             << this->Origin[1] << ","
                             << this->Origin[2] << endl;
}




