// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDocumentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

#include <algorithm>
#include <regex>

vtkStandardNewMacro(vtkSMDocumentation);
vtkCxxSetObjectMacro(vtkSMDocumentation, DocumentationElement, vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMDocumentation::vtkSMDocumentation()
{
  this->DocumentationElement = nullptr;
}

//-----------------------------------------------------------------------------
vtkSMDocumentation::~vtkSMDocumentation()
{
  this->SetDocumentationElement(nullptr);
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetLongHelp()
{
  if (!this->DocumentationElement)
  {
    return nullptr;
  }
  return this->DocumentationElement->GetAttribute("long_help");
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetShortHelp()
{
  if (!this->DocumentationElement)
  {
    return nullptr;
  }
  return this->DocumentationElement->GetAttribute("short_help");
}

//-----------------------------------------------------------------------------
const char* vtkSMDocumentation::GetDescription()
{
  if (!this->DocumentationElement)
  {
    return nullptr;
  }
  if (this->Description.empty())
  {
    // Record string from XML
    this->Description = std::string(this->DocumentationElement->GetCharacterData());

    // Replace eol by space
    std::replace(this->Description.begin(), this->Description.end(), '\n', ' ');

    // Once GCC 4.8 support is dropped, following block can be replace by this one liner:
    //  Remove leading, trailing and extra spaces
    //  this->Description = std::regex_replace(this->Description, std::regex("^ +| +$|( ) +"),
    //  "$1");

    // Remove duplicated spaces
    std::string::iterator new_end = std::unique(this->Description.begin(), this->Description.end(),
      [](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); });
    this->Description.erase(new_end, this->Description.end());

    // Remove single leading/trailing space
    if (this->Description.size() >= 2)
    {
      this->Description.erase(
        std::remove(this->Description.begin(), this->Description.begin() + 1, ' '),
        this->Description.begin() + 1);
      this->Description.erase(
        std::remove(this->Description.end() - 1, this->Description.end(), ' '),
        this->Description.end());
    }
  }
  return this->Description.c_str();
}

//-----------------------------------------------------------------------------
void vtkSMDocumentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DocumentationElement: " << this->DocumentationElement << endl;
  const char* long_help = this->GetLongHelp();
  const char* short_help = this->GetShortHelp();
  const char* text = this->GetDescription();
  os << indent << "Long Help: " << (long_help ? long_help : "(none)") << endl;
  os << indent << "Short Help: " << (short_help ? short_help : "(none)") << endl;
  os << indent << "Description: " << (text ? text : "(none)") << endl;
}
