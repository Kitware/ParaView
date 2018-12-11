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
/**
 * @class   vtkHybridProbeFilter
 *
 * vtkHybridProbeFilter is a combination of vtkExtractSelection at a specific
 * location and vtkProbeFilter. Since the 'Selection Inspector' in ParaView was
 * removed, we were missing ability to extract cells based on location. This
 * filter fills that gap until we get the change to extend "Find Data" mechanism
 * to support location based selections.
 *
 * This filter also "probes" just as a convenience since the user may not know
 * exactly what he/she is looking for -- interpolate at point location (probe)
 * or extract cell containing the point (extract selection).
 *
 * Internally this filter uses vtkPProbeFilter and vtkExtractSelection.
*/

#ifndef vtkHybridProbeFilter_h
#define vtkHybridProbeFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkUnstructuredGrid;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkHybridProbeFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkHybridProbeFilter* New();
  vtkTypeMacro(vtkHybridProbeFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ModeType
  {
    INTERPOLATE_AT_LOCATION,
    EXTRACT_CELL_CONTAINING_LOCATION
  };

  vtkSetClampMacro(Mode, int, INTERPOLATE_AT_LOCATION, EXTRACT_CELL_CONTAINING_LOCATION);
  vtkGetMacro(Mode, int);
  void SetModeToInterpolateAtLocation() { this->SetMode(INTERPOLATE_AT_LOCATION); }
  void SetModeToExtractCellContainingLocation() { this->SetMode(EXTRACT_CELL_CONTAINING_LOCATION); }

  //@{
  /**
   * Get/Set the location to probe/pick at.
   */
  vtkSetVector3Macro(Location, double);
  vtkGetVector3Macro(Location, double);
  //@}

protected:
  vtkHybridProbeFilter();
  ~vtkHybridProbeFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  bool InterpolateAtLocation(vtkDataObject* input, vtkUnstructuredGrid* output);
  bool ExtractCellContainingLocation(vtkDataObject* input, vtkUnstructuredGrid* output);

  double Location[3];
  int Mode;

private:
  vtkHybridProbeFilter(const vtkHybridProbeFilter&) = delete;
  void operator=(const vtkHybridProbeFilter&) = delete;
};

#endif
