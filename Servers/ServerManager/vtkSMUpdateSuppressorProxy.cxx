/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateSuppressorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUpdateSuppressorProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMUpdateSuppressorProxy);
vtkCxxRevisionMacro(vtkSMUpdateSuppressorProxy, "1.1");

//---------------------------------------------------------------------------
vtkSMUpdateSuppressorProxy::vtkSMUpdateSuppressorProxy()
{
  this->DoInsertExtractPieces = 0;
}

//---------------------------------------------------------------------------
vtkSMUpdateSuppressorProxy::~vtkSMUpdateSuppressorProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMUpdateSuppressorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}







