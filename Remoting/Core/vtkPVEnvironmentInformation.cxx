// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVEnvironmentInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVEnvironmentInformation);

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformation::vtkPVEnvironmentInformation()
{
  this->RootOnly = 1;
  this->Variable = nullptr;
}

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformation::~vtkPVEnvironmentInformation()
{
  this->SetVariable(nullptr);
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformation::CopyFromObject(vtkObject* object)
{
  vtkPVEnvironmentInformationHelper* helper =
    vtkPVEnvironmentInformationHelper::SafeDownCast(object);
  if (!helper)
  {
    vtkErrorMacro("Can collect information only from a vtkPVEnvironmentInformationHelper.");
    return;
  }
  this->SetVariable(vtksys::SystemTools::GetEnv(helper->GetVariable()));
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformation::CopyToStream(vtkClientServerStream* stream)
{
  *stream << vtkClientServerStream::Reply << this->Variable;
  *stream << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* temp = nullptr;
  if (!css->GetArgument(0, 0, &temp))
  {
    vtkErrorMacro("Error parsing Variable.");
    return;
  }
  this->SetVariable(temp);
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Variable: " << (this->Variable ? this->Variable : "(none)") << endl;
}
