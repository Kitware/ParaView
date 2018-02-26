/*=========================================================================

  Program:   ParaView
  Module:    vtkBoundedPlaneSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundedPlaneSource.h"

#include "vtkBoundedVolumeSource.h"
#include "vtkBoundingBox.h"
#include "vtkFlyingEdgesPlaneCutter.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkBoundedPlaneSource);
//----------------------------------------------------------------------------
vtkBoundedPlaneSource::vtkBoundedPlaneSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Normal[0] = this->Normal[1] = 0.0;
  this->Normal[1] = 1.0;
  this->Resolution = 100;
  this->RefinementMode = USE_RESOLUTION;
  this->CellSize = 1.0;
  this->Padding = 0;
}

//----------------------------------------------------------------------------
vtkBoundedPlaneSource::~vtkBoundedPlaneSource()
{
}

//----------------------------------------------------------------------------
int vtkBoundedPlaneSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  vtkBoundingBox bbox(this->BoundingBox);
  bbox.Inflate(this->Padding);
  if (!bbox.IsValid())
  {
    vtkErrorMacro("Invalid bounding box specified. Please choose a valid BoundingBox.");
    return 0;
  }

  vtkNew<vtkImageData> image;

  if (this->RefinementMode == USE_RESOLUTION)
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(image, bbox, vtkVector3i(this->Resolution)))
    {
      vtkErrorMacro("Failed to determine image parameters.");
      return 0;
    }
  }
  else
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(image, bbox, this->CellSize))
    {
      vtkErrorMacro("Failed to determine image parameters.");
      return 0;
    }
  }

  // AllocateScalars, alas, is needed since vtkFlyingEdgesPlaneCutter cannot work without
  // scalars.
  image->AllocateScalars(VTK_CHAR, 1);

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(this->Center);
  plane->SetNormal(this->Normal);

  vtkNew<vtkFlyingEdgesPlaneCutter> cutter;
  cutter->SetPlane(plane.Get());
  cutter->SetInputDataObject(image.Get());
  cutter->ComputeNormalsOff();
  cutter->InterpolateAttributesOff();
  cutter->Update();
  output->CopyStructure(cutter->GetOutput(0));
  return 1;
}

//----------------------------------------------------------------------------
void vtkBoundedPlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << endl;
  os << indent << "Normal: " << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << endl;
  os << indent << "BoundingBox: " << this->BoundingBox[0] << ", " << this->BoundingBox[1] << ", "
     << this->BoundingBox[2] << ", " << this->BoundingBox[3] << ", " << this->BoundingBox[4] << ", "
     << this->BoundingBox[5] << endl;
  os << indent << "RefinementMode: " << this->RefinementMode << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "CellSize: " << this->CellSize << endl;
  os << indent << "Padding: " << this->Padding << endl;
}
