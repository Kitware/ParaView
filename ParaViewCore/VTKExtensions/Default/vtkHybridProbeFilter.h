/*=========================================================================

  Program:   ParaView
  Module:    vtkHybridProbeFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHybridProbeFilter
// .SECTION Description
// vtkHybridProbeFilter is a combination of vtkExtractSelection at a specific
// location and vtkProbeFilter. Since the 'Selection Inspector' in ParaView was
// removed, we were missing ability to extract cells based on location. This
// filter fills that gap until we get the change to extend "Find Data" mechanism
// to support location based selections.
//
// This filter also "probes" just as a convenience since the user may not know
// exactly what he/she is looking for -- interpolate at point location (probe)
// or extract cell containing the point (extract selection).
//
// Internally this filter uses vtkPProbeFilter and vtkExtractSelection.

#ifndef __vtkHybridProbeFilter_h
#define __vtkHybridProbeFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkDataObjectAlgorithm.h"

class vtkUnstructuredGrid;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkHybridProbeFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkHybridProbeFilter* New();
  vtkTypeMacro(vtkHybridProbeFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum ModeType
    {
    INTERPOLATE_AT_LOCATION,
    EXTRACT_CELL_CONTAINING_LOCATION
    };

  vtkSetClampMacro(Mode, int, INTERPOLATE_AT_LOCATION, EXTRACT_CELL_CONTAINING_LOCATION);
  vtkGetMacro(Mode, int);
  void SetModeToInterpolateAtLocation() { this->SetMode(INTERPOLATE_AT_LOCATION); }
  void SetModeToExtractCellContainingLocation() { this->SetMode(EXTRACT_CELL_CONTAINING_LOCATION); }

  // Description:
  // Get/Set the location to probe/pick at.
  vtkSetVector3Macro(Location, double);
  vtkGetVector3Macro(Location, double);

//BTX
protected:
  vtkHybridProbeFilter();
  ~vtkHybridProbeFilter();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  bool InterpolateAtLocation(vtkDataObject* input, vtkUnstructuredGrid* output);
  bool ExtractCellContainingLocation(vtkDataObject* input, vtkUnstructuredGrid* output);

  double Location[3];
  int Mode;

private:
  vtkHybridProbeFilter(const vtkHybridProbeFilter&); // Not implemented
  void operator=(const vtkHybridProbeFilter&); // Not implemented
//ETX
};

#endif
