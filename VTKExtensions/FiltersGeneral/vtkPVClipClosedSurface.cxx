/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVClipClosedSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClipClosedSurface.h"

#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"

vtkStandardNewMacro(vtkPVClipClosedSurface);

//-----------------------------------------------------------------------------
vtkPVClipClosedSurface::vtkPVClipClosedSurface()
{
  this->InsideOut = 0;
  this->ClippingPlane = nullptr;
  this->ClippingPlanes = nullptr;
}

//-----------------------------------------------------------------------------
vtkPVClipClosedSurface::~vtkPVClipClosedSurface()
{
  this->ClippingPlane = nullptr;

  // leave the plane collection to be deleted by the parent class
}

//-----------------------------------------------------------------------------
void vtkPVClipClosedSurface::SetClippingPlane(vtkPlane* plane)
{
  this->ClippingPlane = plane;

  if (this->ClippingPlanes)
  {
    this->ClippingPlanes->Delete();
    this->ClippingPlanes = nullptr;
  }

  this->ClippingPlanes = vtkPlaneCollection::New();
  this->ClippingPlanes->AddItem(plane);
}

//-----------------------------------------------------------------------------
int vtkPVClipClosedSurface::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->InsideOut)
  {
    // the normal vector of the plane specified by the user
    double normalVec[3];
    this->ClippingPlane->GetNormal(normalVec);

    // the actual normal vector (of the plane) employed by the clipper
    double actualVec[3] = { -normalVec[0], -normalVec[1], -normalVec[2] };
    this->ClippingPlane->SetNormal(actualVec);

    int retVal = Superclass::RequestData(request, inputVector, outputVector);

    // restore the clipping plane with the normal vector specified by the user
    this->ClippingPlane->SetNormal(normalVec);

    return retVal;
  }
  else
  {
    return Superclass::RequestData(request, inputVector, outputVector);
  }
}

//-----------------------------------------------------------------------------
void vtkPVClipClosedSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Insideout: " << this->InsideOut << endl;
  os << indent << "Clipping Plane: " << this->ClippingPlane << endl;
}
