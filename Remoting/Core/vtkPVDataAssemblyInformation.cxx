// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDataAssemblyInformation.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkDataAssembly.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVDataAssemblyInformation);
vtkCxxSetObjectMacro(vtkPVDataAssemblyInformation, DataAssembly, vtkDataAssembly);
//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::vtkPVDataAssemblyInformation()
  : DataAssembly(nullptr)
  , MethodName(nullptr)
{
  this->SetRootOnly(1);
  this->SetMethodName("GetAssembly");
}

//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::~vtkPVDataAssemblyInformation()
{
  this->SetDataAssembly(nullptr);
  this->SetMethodName(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyParametersToStream(vtkMultiProcessStream& stream)
{
  this->Superclass::CopyParametersToStream(stream);

  std::string name(this->MethodName ? this->MethodName : "");
  stream << name;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyParametersFromStream(vtkMultiProcessStream& stream)
{
  this->Superclass::CopyParametersFromStream(stream);

  std::string name;
  stream >> name;
  this->SetMethodName(name.empty() ? nullptr : name.c_str());
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  if (this->DataAssembly)
  {
    *css << this->DataAssembly->SerializeToXML(vtkIndent());
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->SetDataAssembly(nullptr);
  if (css->GetNumberOfArguments(0) == 1)
  {
    std::string xml;
    css->GetArgument(0, 0, &xml);

    vtkNew<vtkDataAssembly> info;
    info->InitializeFromXML(xml.c_str());
    this->SetDataAssembly(info);
  }
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::CopyFromObject(vtkObject* obj)
{
  this->SetDataAssembly(nullptr);

  auto interp = vtkClientServerInterpreterInitializer::GetGlobalInterpreter();
  vtkClientServerStream command;
  command << vtkClientServerStream::Invoke << obj << this->GetMethodName()
          << vtkClientServerStream::End;
  if (interp && interp->ProcessStream(command))
  {
    auto& result = interp->GetLastResult();
    if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1)
    {
      vtkObjectBase* ptr = nullptr;
      result.GetArgument(0, 0, &ptr);
      if (auto assembly = vtkDataAssembly::SafeDownCast(ptr))
      {
        // we store a clone just to avoid the scenario where the `obj` modifies
        // the vtkDataAssembly under the covers.
        vtkNew<vtkDataAssembly> clone;
        clone->DeepCopy(assembly);
        this->SetDataAssembly(clone);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataAssembly: " << this->DataAssembly << endl;
  os << indent << "MethodName: " << (this->MethodName ? this->MethodName : "(nullptr)") << endl;
}
