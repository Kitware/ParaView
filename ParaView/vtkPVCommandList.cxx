/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCommandList.cxx
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
#include "vtkObjectFactory.h"
#include "vtkPVCommandList.h"

//----------------------------------------------------------------------------
vtkPVCommandList* vtkPVCommandList::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVCommandList");
  if(ret)
    {
    return (vtkPVCommandList*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVCommandList;
}

//----------------------------------------------------------------------------
vtkPVCommandList::vtkPVCommandList()
{  
  this->NumberOfCommands = 0;
  this->CommandArrayLength = 0;
  this->Commands = NULL;
}

//----------------------------------------------------------------------------
vtkPVCommandList::~vtkPVCommandList()
{
  int i;

  for (i = 0; i < this->NumberOfCommands; ++i)
    {
    if (this->Commands[i])
      {
      delete [] this->Commands[i];
      this->Commands[i] = NULL;
      }
    }
  if (this->Commands)
    {
    delete [] this->Commands;
    this->Commands = NULL;
    this->NumberOfCommands = 0;
    this->CommandArrayLength = 0;
    }
}


//----------------------------------------------------------------------------
char *vtkPVCommandList::GetCommand(int idx)
{
  if (idx < 0 || idx >= this->NumberOfCommands)
    {
    return NULL;
    }
  
  return this->Commands[idx];
}

//----------------------------------------------------------------------------
void vtkPVCommandList::AddCommand(const char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  // Check to see if we need to extent to array of commands.
  if (this->CommandArrayLength <= this->NumberOfCommands)
    { // Yes.
    int i;
    // Allocate a new array
    this->CommandArrayLength += 20;
    char **tmp = new char* [this->CommandArrayLength];
    // Copy array elements.
    for (i = 0; i < this->NumberOfCommands; ++i)
      {
      tmp[i] = this->Commands[i];
      }
    // Delete the old array.
    if (this->Commands)
      {
      delete [] this->Commands;
      this->Commands = NULL;
      }
    // Set the new array.
    this->Commands = tmp;
    tmp = NULL;
    }

  // Allocate the string for and set the new command.
  this->Commands[this->NumberOfCommands] 
              = new char[strlen(event) + 2];
  strcpy(this->Commands[this->NumberOfCommands], event);
  this->NumberOfCommands += 1;
}
