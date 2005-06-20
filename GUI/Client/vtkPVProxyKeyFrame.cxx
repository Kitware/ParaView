/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProxyKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProxyKeyFrame.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVProxyKeyFrame, "1.1");
//-----------------------------------------------------------------------------
vtkPVProxyKeyFrame::vtkPVProxyKeyFrame()
{
}

//-----------------------------------------------------------------------------
vtkPVProxyKeyFrame::~vtkPVProxyKeyFrame()
{
}

//-----------------------------------------------------------------------------
void vtkPVProxyKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
