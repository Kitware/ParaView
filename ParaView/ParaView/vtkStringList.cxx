/*=========================================================================

  Program:   ParaView
  Module:    vtkStringList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
      delete [] this->Strings[i];
      this->Strings[i] = NULL;
      }
    }
  if (this->Strings)
    {
    delete [] this->Strings;
    this->Strings = NULL;
    this->NumberOfStrings = 0;
    this->StringArrayLength = 0;
    }
}

//----------------------------------------------------------------------------
char *vtkStringList::GetString(int idx)
{
  if (idx < 0 || idx >= this->NumberOfStrings)
    {
    return NULL;
    }
  
  return this->Strings[idx];
}

//----------------------------------------------------------------------------
void vtkStringList::AddString(const char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  // Check to see if we need to extent to array of commands.
  if (this->StringArrayLength <= this->NumberOfStrings)
    { // Yes.
    this->Reallocate(this->StringArrayLength + 20);
    }

  // Allocate the string for and set the new command.
  this->Strings[this->NumberOfStrings] 
              = new char[strlen(event) + 2];
  strcpy(this->Strings[this->NumberOfStrings], event);
  this->NumberOfStrings += 1;
}

//----------------------------------------------------------------------------
void vtkStringList::SetString(int idx, const char *str)
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
    delete [] this->Strings[idx];
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
  if (this->StringArrayLength >= num)
    { // No
    return;
    }

  // Allocate a new array
  this->StringArrayLength = num;
  char **tmp = new char* [this->StringArrayLength];
  // Copy array elements.
  for (i = 0; i < this->NumberOfStrings; ++i)
    {
    tmp[i] = this->Strings[i];
    }
  // Delete the old array.
  if (this->Strings)
    {
    delete [] this->Strings;
    this->Strings = NULL;
    }
  // Set the new array.
  this->Strings = tmp;
  tmp = NULL;
}

//----------------------------------------------------------------------------
void vtkStringList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfStrings: " << this->GetNumberOfStrings() << endl;
}
