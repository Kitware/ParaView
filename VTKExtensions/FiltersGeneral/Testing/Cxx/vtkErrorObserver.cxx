/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkErrorObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkErrorObserver.h"

//------------------------------------------------------------------------------
vtkErrorObserver* vtkErrorObserver::New()
{
  // do not use vtkStandardNewMacro - see vtkDebugLeaks.h for details
  return new vtkErrorObserver;
}

//------------------------------------------------------------------------------
bool vtkErrorObserver::GetError() const
{
  return this->Error;
}

//------------------------------------------------------------------------------
bool vtkErrorObserver::GetWarning() const
{
  return this->Warning;
}

//------------------------------------------------------------------------------
void vtkErrorObserver::Clear()
{
  this->Error = false;
  this->Warning = false;
  this->ErrorMessage = "";
  this->WarningMessage = "";
}

//------------------------------------------------------------------------------
void vtkErrorObserver::Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata)
{
  switch (event)
  {
    case vtkCommand::ErrorEvent:
      ErrorMessage = static_cast<char*>(calldata);
      this->Error = true;
      break;
    case vtkCommand::WarningEvent:
      WarningMessage = static_cast<char*>(calldata);
      this->Warning = true;
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
std::string vtkErrorObserver::GetErrorMessage() const
{
  return ErrorMessage;
}

//------------------------------------------------------------------------------
std::string vtkErrorObserver::GetWarningMessage() const
{
  return WarningMessage;
}
