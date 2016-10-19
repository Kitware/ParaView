// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSamplePlaneSource.cxx

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

#include "vtkSamplePlaneSource.h"

#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDummyController.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
vtkStandardNewMacro(vtkSamplePlaneSource);

vtkCxxSetObjectMacro(vtkSamplePlaneSource, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkSamplePlaneSource::vtkSamplePlaneSource()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->Resolution = 100;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (!this->Controller)
  {
    VTK_CREATE(vtkDummyController, dummyController);
    this->SetController(dummyController);
  }
}

vtkSamplePlaneSource::~vtkSamplePlaneSource()
{
  this->SetController(NULL);
}

void vtkSamplePlaneSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")" << endl;
  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")" << endl;
  ;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
int vtkSamplePlaneSource::FillInputPortInformation(int port, vtkInformation* info)
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
int vtkSamplePlaneSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  double bounds[6];
  this->ComputeLocalBounds(input, bounds);
  this->ResolveParallelBounds(bounds);
  this->CreatePlane(bounds, output);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSamplePlaneSource::ComputeLocalBounds(vtkDataObject* input, double bounds[6])
{
  bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (dsInput)
  {
    dsInput->GetBounds(bounds);
    return;
  }

  vtkCompositeDataSet* cInput = vtkCompositeDataSet::SafeDownCast(input);
  if (cInput)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      double subbounds[6];
      this->ComputeLocalBounds(iter->GetCurrentDataObject(), subbounds);
      if (bounds[0] > subbounds[0])
        bounds[0] = subbounds[0];
      if (bounds[1] < subbounds[1])
        bounds[1] = subbounds[1];
      if (bounds[2] > subbounds[2])
        bounds[2] = subbounds[2];
      if (bounds[3] < subbounds[3])
        bounds[3] = subbounds[3];
      if (bounds[4] > subbounds[4])
        bounds[4] = subbounds[4];
      if (bounds[5] < subbounds[5])
        bounds[5] = subbounds[5];
    }
    return;
  }

  vtkWarningMacro(<< "Unknown data type: " << input->GetClassName());
}

//-----------------------------------------------------------------------------
void vtkSamplePlaneSource::ResolveParallelBounds(double bounds[6])
{
  this->Controller->AllReduce(&bounds[0], &bounds[0], 1, vtkCommunicator::MIN_OP);
  this->Controller->AllReduce(&bounds[1], &bounds[1], 1, vtkCommunicator::MAX_OP);
  this->Controller->AllReduce(&bounds[2], &bounds[2], 1, vtkCommunicator::MIN_OP);
  this->Controller->AllReduce(&bounds[3], &bounds[3], 1, vtkCommunicator::MAX_OP);
  this->Controller->AllReduce(&bounds[4], &bounds[4], 1, vtkCommunicator::MIN_OP);
  this->Controller->AllReduce(&bounds[5], &bounds[5], 1, vtkCommunicator::MAX_OP);
}

//-----------------------------------------------------------------------------
void vtkSamplePlaneSource::CreatePlane(const double bounds[6], vtkPolyData* output)
{
  double dims[3];
  for (int i = 0; i < 3; i++)
  {
    dims[i] = bounds[2 * i + 1] - bounds[2 * i];
    if (dims[i] < 0)
      dims[i] = 0;
  }

  double diagonal = sqrt(dims[0] * dims[0] + dims[1] * dims[1] + dims[2] * dims[2]);
  if (diagonal <= 0)
  {
    // No space.  Just do nothing.
    return;
  }

  // Use the plane source to create the geometry.
  VTK_CREATE(vtkPlaneSource, planeSource);
  planeSource->SetXResolution(2 * this->Resolution);
  planeSource->SetYResolution(2 * this->Resolution);

  planeSource->SetOrigin(0.0, 0.0, 0.0);
  planeSource->SetPoint1(2 * diagonal, 0.0, 0.0);
  planeSource->SetPoint2(0.0, 2 * diagonal, 0.0);

  planeSource->SetCenter(this->Center);
  planeSource->SetNormal(this->Normal);

  planeSource->Update();
  output->ShallowCopy(planeSource->GetOutput());
}
