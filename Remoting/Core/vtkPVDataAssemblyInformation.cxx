/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAssemblyInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataAssemblyInformation.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerStreamInstantiator.h"
#include "vtkDataAssembly.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

vtkStandardNewMacro(vtkPVDataAssemblyInformation);
//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::vtkPVDataAssemblyInformation()
  : DataAssembly(nullptr)
  , PortNumber(0)
{
  this->RootOnly = 1;
}

//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::~vtkPVDataAssemblyInformation() = default;

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::Initialize()
{
  this->DataAssembly = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyFromObject(vtkObject* obj)
{
  this->DataAssembly = nullptr;
  if (auto algorithm = vtkAlgorithm::SafeDownCast(obj))
  {
    // since we don't want to cause `RequestDataObject` to be triggered
    // here (see #paraview/paraview#20016), we explicitly check if the data
    // object exists rather than using vtkAlgorithm::GetOutputDataObject().
    auto info = (this->PortNumber < algorithm->GetNumberOfOutputPorts())
      ? algorithm->GetOutputInformation(this->PortNumber)
      : nullptr;
    if (auto data = vtkPartitionedDataSetCollection::GetData(info))
    {
      this->DataAssembly = data->GetDataAssembly();
    }
  }
  else
  {
    vtkErrorMacro("Information can only be gathered from a 'vtkAlgorithm'.");
  }
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  if (this->DataAssembly)
  {
    *css << this->DataAssembly->SerializeToXML(vtkIndent{}).c_str();
  }
  else
  {
    *css << ""; // empty string.
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* xml = nullptr;
  if (css->GetArgument(0, 0, &xml))
  {
    if (xml != nullptr && xml[0] != '\0')
    {
      vtkNew<vtkDataAssembly> assembly;
      assembly->InitializeFromXML(xml);
      this->DataAssembly = assembly;
    }
    else
    {
      this->DataAssembly = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 828793 << this->PortNumber;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->PortNumber;
  if (magic_number != 828793)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkPVDataAssemblyInformation::GetDataAssembly()
{
  return this->DataAssembly;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PortNumber: " << this->PortNumber << endl;
  if (this->DataAssembly)
  {
    os << indent << "DataAssembly: " << endl;
    this->DataAssembly->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "DataAssembly: (none)" << endl;
  }
}
