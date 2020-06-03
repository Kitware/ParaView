/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCinemaExtractWriterProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMCinemaExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMCinemaExtractWriterProxy::vtkSMCinemaExtractWriterProxy()
{
}

//----------------------------------------------------------------------------
vtkSMCinemaExtractWriterProxy::~vtkSMCinemaExtractWriterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMCinemaExtractWriterProxy::Write(vtkSMExtractsController* extractor)
{
  vtkErrorMacro("Cinema extracts are not yet supported!");
  return true;
}

//----------------------------------------------------------------------------
void vtkSMCinemaExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
