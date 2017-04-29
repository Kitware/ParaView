/*=========================================================================

  Program:   ParaView
  Module:    vtkStructuredGridVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStructuredGridVolumeRepresentation
 * @brief   representation for showing
 * vtkStructuredGrid datasets as volumes.
 *
 * vtkStructuredGridVolumeRepresentation is a representation for volume
 * rendering vtkStructuredGrid datasets with one caveat: it assumes that the
 * structured grid is not "curved" i.e. bounding boxes of non-intersecting
 * extents don't intersect (or intersect negligibly). This is the default (and
 * faster) method. Alternatively, one can set UseDataPartitions to
 * off and the representation will simply rely on the view to build the sorting
 * order using the unstructured grid. In which case, however data will be
 * transferred among processing.
*/

#ifndef vtkStructuredGridVolumeRepresentation_h
#define vtkStructuredGridVolumeRepresentation_h

#include "vtkUnstructuredGridVolumeRepresentation.h"

class vtkTableExtentTranslator;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkStructuredGridVolumeRepresentation
  : public vtkUnstructuredGridVolumeRepresentation
{
public:
  static vtkStructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkStructuredGridVolumeRepresentation, vtkUnstructuredGridVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

protected:
  vtkStructuredGridVolumeRepresentation();
  ~vtkStructuredGridVolumeRepresentation();

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  bool UseDataPartitions;
  vtkTableExtentTranslator* TableExtentTranslator;

private:
  vtkStructuredGridVolumeRepresentation(
    const vtkStructuredGridVolumeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredGridVolumeRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
