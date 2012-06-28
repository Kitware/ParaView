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
// .NAME vtkPVPostFilter - Post Filter for on demand conversion
// .SECTION Description
// vtkPVPostFilter is a filter used for on demand conversion
// of properties
// Provide the ability to automatically use a vector component as a scalar
// input property.
//
//  Interpolate cell centered data to point data, and the inverse if needed
// by the filter.

#ifndef __vtkPVPostFilter_h
#define __vtkPVPostFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkStdString.h" // needed for: vtkStdString

class VTK_EXPORT vtkPVPostFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkPVPostFilter* New();
  vtkTypeMacro(vtkPVPostFilter,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // We need to override this method because the composite data pipeline
  // is not what we want. Instead we need the PVCompositeDataPipeline
  // so that we can figure out what we conversion(s) we need to do
  vtkExecutive* CreateDefaultExecutive();

  static vtkStdString DefaultComponentName(int componentNumber, int componentCount);

protected:
  vtkPVPostFilter();
  ~vtkPVPostFilter();


  virtual int FillInputPortInformation( int port, vtkInformation* info);
  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);


  int DoAnyNeededConversions(vtkDataObject* output);
  int DoAnyNeededConversions(vtkDataSet* output,
    const char* requested_name, int fieldAssociation,
    const char* demangled_name, const char* demagled_component_name);
  void CellDataToPointData(vtkDataSet* output);
  void PointDataToCellData(vtkDataSet* output);
  int ExtractComponent(vtkDataSetAttributes* dsa,
    const char* requested_name, const char* demangled_name,
    const char* demagled_component_name);

private:
  vtkPVPostFilter(const vtkPVPostFilter&);  // Not implemented.
  void operator=(const vtkPVPostFilter&);  // Not implemented.
};

#endif
