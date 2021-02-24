/*=========================================================================

Program:   ParaView
Module:    vtkSMPWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPWriterProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMPWriterProxy);
//-----------------------------------------------------------------------------
vtkSMPWriterProxy::vtkSMPWriterProxy()
{
  this->SupportsParallel = 1;
}

//-----------------------------------------------------------------------------
vtkSMPWriterProxy::~vtkSMPWriterProxy() = default;

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
