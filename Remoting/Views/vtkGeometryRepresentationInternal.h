/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"

// We'll use the VTKm decimation filter if TBB is enabled, otherwise we'll
// fallback to vtkQuadricClustering, since vtkmLevelOfDetail is slow on the
// serial backend.
#ifndef __VTK_WRAP__
#if VTK_MODULE_ENABLE_VTK_vtkm
#include "vtkmConfigFilters.h" // for VTKM_ENABLE_TBB
#endif

#if defined(VTKM_ENABLE_TBB) && VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters
#include "vtkCellArray.h"
#include "vtkQuadricClustering.h"
#include "vtkmLevelOfDetail.h"
namespace vtkGeometryRepresentation_detail
{
class DecimationFilterType : public vtkmLevelOfDetail
{
public:
  static DecimationFilterType* New();
  vtkTypeMacro(DecimationFilterType, vtkmLevelOfDetail)

    // See note on the vtkQuadricClustering implementation below.
    void SetLODFactor(double factor)
  {
    factor = vtkMath::ClampValue(factor, 0., 1.);

    // This produces the following number of divisions for 'factor':
    // 0.0 --> 64
    // 0.5 --> 256 (default)
    // 1.0 --> 1024
    int divs = static_cast<int>(std::pow(2, 4. * factor + 6.));
    this->SetNumberOfDivisions(divs, divs, divs);
  }

protected:
  DecimationFilterType()
  {
    this->Fallback->SetUseInputPoints(1);
    this->Fallback->SetCopyCellData(1);
    this->Fallback->SetUseInternalTriangles(0);
  }

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    // The accelerated implementation only supports triangle meshes. Fallback to
    // vtkQuadricClustering if needed:
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!input)
    {
      vtkErrorMacro("Expected polydata.");
      return 0;
    }

    // This filter only handles triangles. This code to detect all triangles is
    // adapted from tovtkm::Convert for vtkPolyData:
    vtkCellArray* polys = input->GetPolys();
    const int maxPolySize = polys->GetMaxCellSize();
    const vtkIdType numCells = input->GetNumberOfCells();
    // deduce if we only have triangles. We use maxCellSize+1 so
    // that we handle the length entry in the cell array for each cell
    polys->Squeeze();
    const bool allSameType = ((numCells * (maxPolySize + 1)) == polys->GetSize());
    if (allSameType && maxPolySize == 3)
    { // All triangles. Try the accelerated implementation:
      if (this->Superclass::RequestData(request, inputVector, outputVector))
      {
        return 1;
      }
    }

    // Otherwise fallback to quadric clustering:
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    this->Fallback->SetInputData(input);
    this->Fallback->Update();
    output->ShallowCopy(this->Fallback->GetOutput(0));

    return 1;
  }

  vtkNew<vtkQuadricClustering> Fallback;
};
vtkStandardNewMacro(DecimationFilterType);
}
#else // VTKM_ENABLE_TBB
#include "vtkQuadricClustering.h"
namespace vtkGeometryRepresentation_detail
{
class DecimationFilterType : public vtkQuadricClustering
{
public:
  static DecimationFilterType* New();
  vtkTypeMacro(DecimationFilterType, vtkQuadricClustering)

    // This version gets slower as the grid increases, while the VTKM version
    // scales with number of points. This means we can get away with a much finer
    // grid with the VTKM filter, so we'll just reduce the mesh quality a bit
    // here.
    void SetLODFactor(double factor)
  {
    factor = vtkMath::ClampValue(factor, 0., 1.);

    // This is the same equation used in the old implementation:
    // 0.0 --> 10
    // 0.5 --> 85 (default)
    // 1.0 --> 160
    int divs = static_cast<int>(150 * factor) + 10;
    this->SetNumberOfDivisions(divs, divs, divs);
  }

protected:
  DecimationFilterType()
  {
    this->SetUseInputPoints(1);
    this->SetCopyCellData(1);
    this->SetUseInternalTriangles(0);
  }
};
vtkStandardNewMacro(DecimationFilterType);
}
#endif // VTKM_ENABLE_TBB
#endif // __VTK_WRAP__
// VTK-HeaderTest-Exclude: vtkGeometryRepresentationInternal.h
