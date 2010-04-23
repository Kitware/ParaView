/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClassNameInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClassNameInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVClassNameInformation);

//----------------------------------------------------------------------------
vtkPVClassNameInformation::vtkPVClassNameInformation()
{
  this->VTKClassName = 0;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation::~vtkPVClassNameInformation()
{
  this->SetVTKClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VTKClassName: "
     << (this->VTKClassName?this->VTKClassName:"(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyFromObject(vtkObject* obj)
{
  if(!obj)
    {
    vtkErrorMacro("Cannot get class name from NULL object.");
    return;
    }
  this->SetVTKClassName(obj->GetClassName());
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::AddInformation(vtkPVInformation* info)
{
  if (vtkPVClassNameInformation::SafeDownCast(info))
    {
    this->SetVTKClassName(
      vtkPVClassNameInformation::SafeDownCast(info)->GetVTKClassName());
    }
  
}

//----------------------------------------------------------------------------
void
vtkPVClassNameInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->VTKClassName
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVClassNameInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* cname = 0;
  css->GetArgument(0, 0, &cname);
  this->SetVTKClassName(cname);
}
