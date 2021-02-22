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

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVClassNameInformation);

//----------------------------------------------------------------------------
vtkPVClassNameInformation::vtkPVClassNameInformation()
{
  this->RootOnly = 1;
  this->VTKClassName = nullptr;
  this->PortNumber = -1;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation::~vtkPVClassNameInformation()
{
  this->SetVTKClassName(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PortNumber: " << this->PortNumber << endl;
  os << indent << "VTKClassName: " << (this->VTKClassName ? this->VTKClassName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
  {
    vtkErrorMacro("Cannot get class name from NULL object.");
    return;
  }
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(obj);
  if (algo == nullptr || this->PortNumber == -1)
  {
    this->SetVTKClassName(obj->GetClassName());
  }
  else
  {
    vtkDataObject* dobj = algo->GetOutputDataObject(this->PortNumber);
    if (dobj)
    {
      this->SetVTKClassName(dobj->GetClassName());
    }
    else
    {
      vtkErrorMacro("Cannot get data-object class name from NULL object.");
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::AddInformation(vtkPVInformation* info)
{
  if (vtkPVClassNameInformation::SafeDownCast(info))
  {
    this->SetVTKClassName(vtkPVClassNameInformation::SafeDownCast(info)->GetVTKClassName());
  }
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->VTKClassName << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* cname = nullptr;
  css->GetArgument(0, 0, &cname);
  this->SetVTKClassName(cname);
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 829992 << this->PortNumber;
}

//----------------------------------------------------------------------------
void vtkPVClassNameInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->PortNumber;
  if (magic_number != 829992)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}
