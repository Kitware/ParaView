// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMObject.h"

#include "vtkObjectFactory.h"
#include <cctype>

vtkStandardNewMacro(vtkSMObject);
//---------------------------------------------------------------------------
vtkSMObject::vtkSMObject() = default;

//---------------------------------------------------------------------------
vtkSMObject::~vtkSMObject() = default;

//---------------------------------------------------------------------------
void vtkSMObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
std::string vtkSMObject::CreatePrettyLabel(const std::string& name)
{
  std::string label = " " + name + " ";
  for (std::string::iterator it = label.begin() + 1; it != label.end() - 1; ++it)
  {
    if (std::isspace(*(it - 1)) || std::isspace(*(it + 1)))
    {
      continue;
    }
    if (std::isupper(*it) && std::islower(*(it + 1)))
    {
      it = label.insert(it, ' ');
      continue;
    }
    if (std::islower(*it) && std::isupper(*(it + 1)))
    {
      it = label.insert(it + 1, ' ');
      continue;
    }
  }
  return label.substr(1, label.size() - 2);
}
