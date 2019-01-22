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

// We'll use the VTKm decimation filter if TBB is enabled, otherwise we'll
// fallback to vtkQuadricClustering, since vtkmLevelOfDetail is slow on the
// serial backend.
#ifndef __VTK_WRAP__
#if VTK_MODULE_ENABLE_VTK_vtkm
#include "vtkmConfig.h" // for VTKM_ENABLE_TBB
#endif

#if defined(VTKM_ENABLE_TBB) && VTK_MODULE_ENABLE_VTK_AcceleratorsVTKm
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
};
vtkStandardNewMacro(DecimationFilterType)
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

  DecimationFilterType()
  {
    this->SetUseInputPoints(1);
    this->SetCopyCellData(1);
    this->SetUseInternalTriangles(0);
  }
};
vtkStandardNewMacro(DecimationFilterType)
}
#endif // VTKM_ENABLE_TBB
#endif // __VTK_WRAP__
// VTK-HeaderTest-Exclude: vtkGeometryRepresentationInternal.h
