/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDatasetSpreadRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDatasetSpreadRepresentationProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMDatasetSpreadRepresentationProxy);
vtkCxxRevisionMacro(vtkSMDatasetSpreadRepresentationProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMDatasetSpreadRepresentationProxy::vtkSMDatasetSpreadRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMDatasetSpreadRepresentationProxy::~vtkSMDatasetSpreadRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMDatasetSpreadRepresentationProxy::EndCreateVTKObjects()
{
  // Create a strategy to provide data to the client. Ofcourse we need t
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMDatasetSpreadRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


