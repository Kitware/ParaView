/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataReductionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAttributeDataReductionFilter
 * @brief   Reduces cell/point attribute data
 * with different modes to combine cell/point data.
 *
 * Filter that takes data with same structure on multiple input connections to
 * produce a reduced dataset with cell/point data summed/maxed/minned for
 * all cells/points. Data arrays not available in all inputs
 * are discarded. The attribute to reduce can be set to point or cell or field
 * or all. Only selected attributes will be reduced according to the
 * type chosen, all other attributes are those at the first input.
*/

#ifndef vtkAttributeDataReductionFilter_h
#define vtkAttributeDataReductionFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkAttributeDataReductionFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkAttributeDataReductionFilter* New();
  vtkTypeMacro(vtkAttributeDataReductionFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ReductionTypes
  {
    ADD = 1,
    MAX = 2,
    MIN = 3
  };

  enum AttributeTypes // These can be 'or'-ed together.
  {
    POINT_DATA = 0x01,
    CELL_DATA = 0x02,
    FIELD_DATA = 0x04, // FIXME: Field data not supported yet.
    ROW_DATA = 0x08
  };

  // Set the attributes to reduce. Only the chosen type of attributes will be
  // reduced by this filter. The not-chosen attributes are passed through
  // from the first input unchanged. Default is (POINT_DATA|CELL_DATA)
  // i.e. point data and cell data will be reduced.
  // FIXME: Field data not supported yet.
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);

  // Set the reduction type. Reduction type dictates how overlapping cell/point
  // data is combined. Default is ADD.
  vtkSetMacro(ReductionType, int);
  vtkGetMacro(ReductionType, int);
  const char* GetReductionTypeAsString()
  {
    switch (this->ReductionType)
    {
      case ADD:
        return "ADD";
      case MAX:
        return "MAX";
      case MIN:
        return "MIN";
    }
    return "";
  }
  void SetReductionTypeToAdd() { this->SetReductionType(vtkAttributeDataReductionFilter::ADD); }
  void SetReductionTypeToMax() { this->SetReductionType(vtkAttributeDataReductionFilter::MAX); }
  void SetReductionTypeToMin() { this->SetReductionType(vtkAttributeDataReductionFilter::MIN); }
protected:
  vtkAttributeDataReductionFilter();
  ~vtkAttributeDataReductionFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int ReductionType;
  int AttributeType;

private:
  vtkAttributeDataReductionFilter(const vtkAttributeDataReductionFilter&) = delete;
  void operator=(const vtkAttributeDataReductionFilter&) = delete;
};

#endif
