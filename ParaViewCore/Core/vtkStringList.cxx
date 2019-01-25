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

#include <stdarg.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStringList);

//----------------------------------------------------------------------------
vtkStringList::vtkStringList()
{
  this->NumberOfStrings = 0;
  this->StringArrayLength = 0;
  this->Strings = NULL;
}

//----------------------------------------------------------------------------
vtkStringList::~vtkStringList()
{
  this->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkStringList::RemoveAllItems()
{
  int i;

  for (i = 0; i < this->NumberOfStrings; ++i)
  {
    if (this->Strings[i])
    {
      delete[] this->Strings[i];
      this->Strings[i] = NULL;
    }
  }
  if (this->Strings)
  {
    delete[] this->Strings;
    this->Strings = NULL;
    this->NumberOfStrings = 0;
    this->StringArrayLength = 0;
  }
}

//----------------------------------------------------------------------------
int vtkStringList::GetIndex(const char* str)
{
  if (!str)
  {
    return -1;
  }
  int idx;
  for (idx = 0; idx < this->NumberOfStrings; ++idx)
  {
    if (strcmp(str, this->Strings[idx]) == 0)
    {
      return idx;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
const char* vtkStringList::GetString(int idx)
{
  if (idx < 0 || idx >= this->NumberOfStrings)
  {
    return NULL;
  }

  return this->Strings[idx];
}

//----------------------------------------------------------------------------
void vtkStringList::AddString(const char* str)
{
  if (!str)
  {
    return;
  }

  // Check to see if we need to extent to array of commands.
  if (this->StringArrayLength <= this->NumberOfStrings)
  {
    // Yes.
    this->Reallocate(this->StringArrayLength + 20);
  }

  // Allocate the string for and set the new command.
  this->Strings[this->NumberOfStrings] = new char[strlen(str) + 2];
  strcpy(this->Strings[this->NumberOfStrings], str);
  this->NumberOfStrings += 1;
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
  int j;

  if (idx >= this->StringArrayLength)
  {
    this->Reallocate(idx + 20);
  }

  // Expand the command list to include idx.
  // Add NULL entries if necessary.
  if (idx >= this->NumberOfStrings)
  {
    for (j = this->NumberOfStrings; j <= idx; ++j)
    {
      this->Strings[j] = NULL;
    }
    this->NumberOfStrings = idx + 1;
  }

  // Delete old command
  if (this->Strings[idx])
  {
    delete[] this->Strings[idx];
    this->Strings[idx] = NULL;
  }
  if (str == NULL)
  {
    return;
  }

  // Copy the string into the array.
  this->Strings[idx] = new char[strlen(str) + 2];
  strcpy(this->Strings[idx], str);
}

//----------------------------------------------------------------------------
void vtkStringList::Reallocate(int num)
{
  int i;

  // Check to see if we need to extent to array of commands.
  if (num < 0 || this->StringArrayLength >= num)
  { // No
    return;
  }

  // Allocate a new array
  this->StringArrayLength = num;
  char** tmp = new char*[this->StringArrayLength];
  // Copy array elements.
  for (i = 0; i < this->NumberOfStrings; ++i)
  {
    tmp[i] = this->Strings[i];
  }
  // Delete the old array.
  if (this->Strings)
  {
    delete[] this->Strings;
    this->Strings = NULL;
  }
  // Set the new array.
  this->Strings = tmp;
  tmp = NULL;
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
