// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSimulationToPrismFilter
 * @brief   Convert from Simulation Space to Prism Space for Prism View
 *
 * Given a vtkDataset and an Attribute type, such as, PointData or Cell Data, that has 3+ scalar
 * arrays, you can create a PolyData with vertices that their coordinates are the values of the
 * 3 selected arrays.
 *
 * @warning
 * The input of this filter MUST be the output of vtkSimulationPointCloudFilter
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
 * @sa
 * vtkSimulationPointCloudFilter
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
