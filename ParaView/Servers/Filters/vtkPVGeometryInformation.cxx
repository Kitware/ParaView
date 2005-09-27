/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeometryInformation.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVGeometryInformation);
vtkCxxRevisionMacro(vtkPVGeometryInformation, "1.1");

//----------------------------------------------------------------------------
vtkPVGeometryInformation::vtkPVGeometryInformation()
{
}

//----------------------------------------------------------------------------
vtkPVGeometryInformation::~vtkPVGeometryInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVGeometryInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVGeometryInformation::CopyFromObject(vtkObject* object)
{
  vtkPVGeometryFilter* gf = vtkPVGeometryFilter::SafeDownCast(object);
  if (gf)
    {
    this->CopyFromDataSet(gf->GetOutput());
    return;
    }

  vtkErrorMacro("Cound not cast object to geometry filter.");
}

