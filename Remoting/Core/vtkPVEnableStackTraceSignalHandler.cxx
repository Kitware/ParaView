// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
