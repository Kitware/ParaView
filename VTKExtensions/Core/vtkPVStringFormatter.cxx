/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringFormatter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringFormatter.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

// initialize the Scope Stack
std::stack<std::shared_ptr<vtkPVStringFormatter::vtkArgumentScope>>
  vtkPVStringFormatter::ScopeStack =
    std::stack<std::shared_ptr<vtkPVStringFormatter::vtkArgumentScope>>();

vtkStandardNewMacro(vtkPVStringFormatter);

//----------------------------------------------------------------------------
vtkPVStringFormatter::vtkPVStringFormatter() = default;

//----------------------------------------------------------------------------
vtkPVStringFormatter::~vtkPVStringFormatter() = default;

//----------------------------------------------------------------------------
void vtkPVStringFormatter::PopScope()
{
  if (!vtkPVStringFormatter::ScopeStack.empty())
  {
    vtkPVStringFormatter::ScopeStack.pop();
  }
}

//----------------------------------------------------------------------------
std::string vtkPVStringFormatter::GetArgInfo()
{
  if (vtkPVStringFormatter::ScopeStack.empty())
  {
    return std::string("This argument scope does not include arguments.");
  }
  return std::string("This argument scope includes the following arguments:\n") +
    vtkPVStringFormatter::ScopeStack.top()->GetArgInfo();
}

//----------------------------------------------------------------------------
std::string vtkPVStringFormatter::Format(const std::string& formattableString)
{
  std::string result;

  // format string
  try
  {
    if (!vtkPVStringFormatter::ScopeStack.empty())
    {
      result = fmt::vformat(formattableString, vtkPVStringFormatter::ScopeStack.top()->GetArgs());
    }
    else
    {
      result = fmt::format(formattableString);
    }
  }
  catch (std::runtime_error& error)
  {
    // an object is used to print in the output window of ParaView since vtkLogF is not enough
    // vtkErrorMacro can not be used because this function is static
    auto object = vtkSmartPointer<vtkPVStringFormatter>::New();
    vtkErrorWithObjectMacro(object,
      "\nInvalid format specified '" << formattableString
                                     << "'\n"
                                        "Details: "
                                     << error.what() << "\n\n"
                                     << vtkPVStringFormatter::GetArgInfo());
    result = std::string();
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkPVStringFormatter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
