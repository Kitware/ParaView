/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include <stdarg.h>
#include "vtkObjectFactory.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStringList* vtkStringList::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStringList");
  if(ret)
    {
    return (vtkStringList*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStringList;
}

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
