// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACPlaneGlyphs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkSLACPlaneGlyphs.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSamplePlaneSource.h"
#include "vtkThresholdPoints.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
vtkStandardNewMacro(vtkSLACPlaneGlyphs);

//-----------------------------------------------------------------------------
vtkSLACPlaneGlyphs::vtkSLACPlaneGlyphs()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->Resolution = 10;
}

vtkSLACPlaneGlyphs::~vtkSLACPlaneGlyphs()
{
}

void vtkSLACPlaneGlyphs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")" << endl;
  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")" << endl;
  ;
  os << indent << "Resolution: " << this->Resolution << endl;
}

//-----------------------------------------------------------------------------
int vtkSLACPlaneGlyphs::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSLACPlaneGlyphs::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output objects.
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  // Make a shallow copy of the input so that we can give it to internal
  // filters.  It is NOT a good idea to feed the input directly into internal
  // filters because that can mess up the pipeline connections of this filter.
  vtkSmartPointer<vtkDataObject> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->ShallowCopy(input);

  // Create a plane that we will use to place the glyphs.
  VTK_CREATE(vtkSamplePlaneSource, plane);
  plane->SetInputData(inputCopy);
  plane->SetCenter(this->Center);
  plane->SetNormal(this->Normal);
  plane->SetResolution(this->Resolution);

  // Create a probe that will extract the points of the plane.
  VTK_CREATE(vtkCompositeDataProbeFilter, probe);
  probe->SetSourceData(inputCopy);
  probe->SetInputConnection(plane->GetOutputPort());

  // Extract the points that are actually in the geometry.
  VTK_CREATE(vtkThresholdPoints, threshold);
  threshold->SetExecutive(vtkSmartPointer<vtkCompositeDataPipeline>::New());
  threshold->SetInputConnection(probe->GetOutputPort());
  threshold->ThresholdByUpper(0.5);
  threshold->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "vtkValidPointMask");

  threshold->Update();
  output->ShallowCopy(threshold->GetOutput());
  output->GetPointData()->RemoveArray("vtkValidPointMask");

  return 1;
}
