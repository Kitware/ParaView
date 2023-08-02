// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDisableStackTraceSignalHandler.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemInformation.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDisableStackTraceSignalHandler);

//----------------------------------------------------------------------------
void vtkPVDisableStackTraceSignalHandler::CopyFromObject(vtkObject* obj)
{
  (void)obj;
  vtksys::SystemInformation::SetStackTraceOnError(0);
}

//----------------------------------------------------------------------------
void vtkPVDisableStackTraceSignalHandler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
