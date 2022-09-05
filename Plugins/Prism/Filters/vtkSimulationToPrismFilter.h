/*=========================================================================

  Program:   ParaView
  Module:    vtkSimulationToPrismFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimulationToPrismGeometryFilter
 * @brief   Convert from Simulation Space to Prism Space for Prism View
 *
 * Given a vtkDataset and an Attribute type, such as, PointData or Cell Data, that has 3+ scalar
 * arrays, you can create a PolyData with vertices that their coordinates are the values of the
 * 3 selected arrays.
 *
 * Examples:
 * 1) If input is a dataset that has 1000 cells and the 3 chosen scalar Cell arrays are "a", "b",
 * "c", then the output will have 1000 vertices with "a" as the x-coordinate, "b" as the
 * y-coordinate, and "c" as the z-coordinate.
 *
 * 2) If the input is a dataset that has 1000 points and the 3 chosen scalar Point arrays are "a",
 * "b", "c", then the output will have 1000 vertices with "a" as the x-coordinate, "b" as the
 * y-coordinate, and "c" as the z-coordinate.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkSimulationToPrismGeometryFilter_h
#define vtkSimulationToPrismGeometryFilter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkPrismFiltersModule.h" // for export macro

class VTKPRISMFILTERS_EXPORT vtkSimulationToPrismFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkSimulationToPrismFilter* New();
  vtkTypeMacro(vtkSimulationToPrismFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Control which AttributeType the filter operates on (point data or cell data
   * for vtkDataSets). The default value is vtkDataObject::POINT. The input value for
   * this function should be either vtkDataObject::POINT or vtkDataObject::CELL.
   *
   * The default is vtkDataObject::CELL.
   */
  vtkSetClampMacro(AttributeType, int, vtkDataObject::POINT, vtkDataObject::CELL);
  void SetAttributeTypeToPointData() { this->SetAttributeType(vtkDataObject::POINT); }
  void SetAttributeTypeToCellData() { this->SetAttributeType(vtkDataObject::CELL); }
  vtkGetMacro(AttributeType, int);
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the X coordinate.
   */
  vtkSetStringMacro(XArrayName);
  vtkGetStringMacro(XArrayName);
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Y coordinate.
   */
  vtkSetStringMacro(YArrayName);
  vtkGetStringMacro(YArrayName);
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Z coordinate.
   */
  vtkSetStringMacro(ZArrayName);
  vtkGetStringMacro(ZArrayName);
  ///@}

protected:
  vtkSimulationToPrismFilter();
  ~vtkSimulationToPrismFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int AttributeType;
  char* XArrayName;
  char* YArrayName;
  char* ZArrayName;

private:
  vtkSimulationToPrismFilter(const vtkSimulationToPrismFilter&) = delete;
  void operator=(const vtkSimulationToPrismFilter&) = delete;
};

#endif // vtkSimulationToPrismGeometryFilter_h
