/*=========================================================================

  Program:   ParaView
  Module:    vtkStringList.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStringList.h"

#include "vtkObjectFactory.h"

#include <algorithm>
#include <iterator>
#include <stdarg.h>
#include <vector>

class vtkStringList::vtkInternals
{
public:
  std::vector<std::string> Strings;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStringList);

//----------------------------------------------------------------------------
vtkStringList::vtkStringList()
  : Internals(new vtkStringList::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkStringList::~vtkStringList() = default;

//----------------------------------------------------------------------------
void vtkStringList::RemoveAllItems()
{
  this->Internals->Strings.clear();
}

//----------------------------------------------------------------------------
int vtkStringList::GetIndex(const char* str)
{
  if (!str)
  {
    return -1;
  }
  const auto& internals = (*this->Internals);
  auto iter = std::find(internals.Strings.begin(), internals.Strings.end(), std::string(str));
  return (iter == internals.Strings.end() ? -1 : static_cast<int>(
                                                   std::distance(internals.Strings.begin(), iter)));
}

//----------------------------------------------------------------------------
const char* vtkStringList::GetString(int idx)
{
  const auto& internals = (*this->Internals);
  if (idx < 0 || idx >= static_cast<int>(internals.Strings.size()))
  {
    return nullptr;
  }

  return internals.Strings[idx].c_str();
}

//----------------------------------------------------------------------------
void vtkStringList::AddString(const char* str)
{
  if (!str)
  {
    return;
  }

  auto& internals = (*this->Internals);
  internals.Strings.push_back(str);
}

//----------------------------------------------------------------------------
void vtkStringList::AddUniqueString(const char* str)
{
  if (this->GetIndex(str) >= 0)
  {
    return;
  }
  this->AddString(str);
}

//----------------------------------------------------------------------------
void vtkStringList::AddFormattedString(const char* format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->AddString(event);
}

//----------------------------------------------------------------------------
void vtkStringList::SetString(int idx, const char* str)
{
  if (str == nullptr)
  {
    return;
  }

  auto& internals = (*this->Internals);
  if (idx >= static_cast<int>(internals.Strings.size()))
  {
    internals.Strings.resize(idx + 1);
  }
  internals.Strings[idx] = str;
}

//----------------------------------------------------------------------------
int vtkStringList::GetNumberOfStrings()
{
  const auto& internals = (*this->Internals);
  return static_cast<int>(internals.Strings.size());
}

//----------------------------------------------------------------------------
void vtkStringList::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx, num;

  this->Superclass::PrintSelf(os, indent);
  num = this->GetNumberOfStrings();
  os << indent << "NumberOfStrings: " << num << endl;
  for (idx = 0; idx < num; ++idx)
  {
    os << idx << ": " << this->GetString(idx) << endl;
  }
}
