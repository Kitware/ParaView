/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnableStackTraceSignalHandler.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemInformation.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnableStackTraceSignalHandler);

//----------------------------------------------------------------------------
void vtkPVEnableStackTraceSignalHandler::CopyFromObject(vtkObject* obj)
{
  (void)obj;
  vtksys::SystemInformation::SetStackTraceOnError(1);
}

//----------------------------------------------------------------------------
void vtkPVEnableStackTraceSignalHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
