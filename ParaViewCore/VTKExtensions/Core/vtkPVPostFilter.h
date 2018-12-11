/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPostFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPostFilter
 * @brief   Post Filter for on demand conversion
 *
 * vtkPVPostFilter is a filter used for on demand conversion
 * of properties
 * Provide the ability to automatically use a vector component as a scalar
 * input property.
 *
 *  Interpolate cell centered data to point data, and the inverse if needed
 * by the filter.
*/

#ifndef vtkPVPostFilter_h
#define vtkPVPostFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkStdString.h"                 // needed for: vtkStdString

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVPostFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkPVPostFilter* New();
  vtkTypeMacro(vtkPVPostFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * We need to override this method because the composite data pipeline
   * is not what we want. Instead we need the PVCompositeDataPipeline
   * so that we can figure out what we conversion(s) we need to do
   */
  vtkExecutive* CreateDefaultExecutive() override;

  static vtkStdString DefaultComponentName(int componentNumber, int componentCount);

protected:
  vtkPVPostFilter();
  ~vtkPVPostFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int DoAnyNeededConversions(vtkDataObject* output);
  int DoAnyNeededConversions(vtkDataSet* dataset);
  int DoAnyNeededConversions(vtkDataSet* output, const char* requested_name, int fieldAssociation,
    const char* demangled_name, const char* demagled_component_name);
  void CellDataToPointData(vtkDataSet* output, const char* name);
  void PointDataToCellData(vtkDataSet* output, const char* name);
  int ExtractComponent(vtkDataSetAttributes* dsa, const char* requested_name,
    const char* demangled_name, const char* demagled_component_name);

private:
  vtkPVPostFilter(const vtkPVPostFilter&) = delete;
  void operator=(const vtkPVPostFilter&) = delete;
};

#endif
