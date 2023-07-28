// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSimulationPointCloudFilter
 * @brief   Convert from Simulation Space to Prism Space for Prism View
 *
 * Given a vtkDataset and an Attribute type, such as, PointData or Cell Data, you can create a
 * PolyData with as many vertices as the number of tuples of of the selected Attribute Type.
 * It will also interpolate the point data/cell data depending on the Attribute type.
 *
 * @warning
 * The output points will be PURPOSELY empty because the output of this filter MUST used as input
 * to vtkSimulationToPrismFilter, which will set the points. This is done improve performance.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkSimulationToPrismFilter
 */

#ifndef vtkPrismPointOrCellCloudFilter_h
#define vtkPrismPointOrCellCloudFilter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkPrismFiltersModule.h" // for export macro

class VTKPRISMFILTERS_EXPORT vtkSimulationPointCloudFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkSimulationPointCloudFilter* New();
  vtkTypeMacro(vtkSimulationPointCloudFilter, vtkPolyDataAlgorithm);
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
protected:
  vtkSimulationPointCloudFilter();
  ~vtkSimulationPointCloudFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int AttributeType;

private:
  vtkSimulationPointCloudFilter(const vtkSimulationPointCloudFilter&) = delete;
  void operator=(const vtkSimulationPointCloudFilter&) = delete;
};

#endif // vtkPrismPointOrCellCloudFilter_h
