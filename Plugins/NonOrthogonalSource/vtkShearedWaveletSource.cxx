/*=========================================================================

  Program:   ParaView
  Module:    vtkShearedWaveletSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShearedWaveletSource.h"

#include "vtkDataSetTriangleFilter.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVChangeOfBasisHelper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>

vtkStandardNewMacro(vtkShearedWaveletSource);
//----------------------------------------------------------------------------
vtkShearedWaveletSource::vtkShearedWaveletSource()
{
  this->EnableAxisTitles = false;
  this->EnableTimeLabel = false;
  this->TimeLabel = NULL;
  this->AxisUTitle = NULL;
  this->AxisVTitle = NULL;
  this->AxisWTitle = NULL;

  this->ModelBoundingBox[0] = this->ModelBoundingBox[2] = this->ModelBoundingBox[4] = 0.0;
  this->ModelBoundingBox[1] = this->ModelBoundingBox[3] = this->ModelBoundingBox[5] = 1.0;

  this->BasisU[0] = 1;
  this->BasisU[1] = 0;
  this->BasisU[2] = 0;
  this->BasisV[0] = 0;
  this->BasisV[1] = 1;
  this->BasisV[2] = 0;
  this->BasisW[0] = 0;
  this->BasisW[1] = 0;
  this->BasisW[2] = 1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkShearedWaveletSource::~vtkShearedWaveletSource()
{
  this->SetTimeLabel(NULL);
  this->SetAxisUTitle(NULL);
  this->SetAxisVTitle(NULL);
  this->SetAxisWTitle(NULL);
}

//----------------------------------------------------------------------------
int vtkShearedWaveletSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

  vtkNew<vtkRTAnalyticSource> analyticSource;
  vtkNew<vtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(analyticSource->GetOutputPort());

  // This produces an image in bounds (-10, 10, -10, 10, -10, 10).
  // We will transform this to the bounds defined by ModelBoundingBox.
  // That will be our original input model.
  vtkNew<vtkTransformFilter> transformFilter;
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->PostMultiply();
  transform->Translate(10, 10, 10);
  transform->Scale(0.05, 0.05, 0.05);
  //  at this point, we have a unit box at origin. Now, scale/translate it
  //  based on ModelBoundingBox.
  transform->Scale(this->ModelBoundingBox[1] - this->ModelBoundingBox[0],
    this->ModelBoundingBox[3] - this->ModelBoundingBox[2],
    this->ModelBoundingBox[5] - this->ModelBoundingBox[4]);
  transform->Translate(
    this->ModelBoundingBox[0], this->ModelBoundingBox[2], this->ModelBoundingBox[4]);

  transformFilter->SetTransform(transform.GetPointer());
  transformFilter->SetInputConnection(tetrahedralize->GetOutputPort());
  transformFilter->Update();
  output->ShallowCopy(transformFilter->GetOutputDataObject(0));

  vtkSmartPointer<vtkMatrix4x4> cobMatrix = vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(
    vtkVector3d(this->BasisU), vtkVector3d(this->BasisV), vtkVector3d(this->BasisW));
  transform->SetMatrix(cobMatrix.GetPointer());
  transformFilter->SetInputDataObject(output);
  transformFilter->Update();

  output->ShallowCopy(transformFilter->GetOutputDataObject(0));

  vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(output, cobMatrix);
  vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(output, this->ModelBoundingBox);

  if (this->EnableAxisTitles)
  {
    vtkPVChangeOfBasisHelper::AddBasisNames(
      output, this->AxisUTitle, this->AxisVTitle, this->AxisWTitle);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkShearedWaveletSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
