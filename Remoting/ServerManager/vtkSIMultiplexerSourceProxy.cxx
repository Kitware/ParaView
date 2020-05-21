/*=========================================================================

  Program:   ParaView
  Module:    vtkSIMultiplexerSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIMultiplexerSourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIMultiplexerSourceProxy);
//----------------------------------------------------------------------------
vtkSIMultiplexerSourceProxy::vtkSIMultiplexerSourceProxy()
{
}

//----------------------------------------------------------------------------
vtkSIMultiplexerSourceProxy::~vtkSIMultiplexerSourceProxy()
{
}

//----------------------------------------------------------------------------
void vtkSIMultiplexerSourceProxy::Select(vtkSISourceProxy* subproxy)
{
  auto self = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  auto active_algo = vtkAlgorithm::SafeDownCast(subproxy->GetVTKObject());
  self->SetInputConnection(active_algo->GetOutputPort(0));
}

//----------------------------------------------------------------------------
void vtkSIMultiplexerSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
