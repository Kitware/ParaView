/*=========================================================================

  Program:   ParaView
  Module:    vtkUnstructuredDataDeliveryFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredDataDeliveryFilter
// .SECTION Description
// vtkUnstructuredDataDeliveryFilter is a helper filter that can be used in
// vtkPolyData or vtkUnstructuredGrid representations to deliver the data to the
// process(es) doing the rendering. Use this filter in
// vtkView::REQUEST_PREPARE_FOR_RENDER() pass to deliver the data to the correct
// node for rendering. Simply put this filter in your rendering-pipeline (for
// example after the geometry filter and before the mapper) and then pass the
// request-information object to ProcessViewRequest() call. This filter will
// lookup keys in the information object and update its state as needed to
// ensure that the data is available in the form/shape requested on the
// rendering nodes.
// .SECTION Implementation Details
// This filter uses two internal vtkMPIMoveData filters. The first one manages
// the movement across MPI processes and render-server (if applicable) while the
// second one manages the movement across client-server. That makes it possible
// to minimize interprocess data transfers whenever possible with multi-client
// configurations.
// .SECTION See Also
// vtkImageSliceDataDeliveryFilter

#ifndef __vtkUnstructuredDataDeliveryFilter_h
#define __vtkUnstructuredDataDeliveryFilter_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkMPIMoveData;

class VTK_EXPORT vtkUnstructuredDataDeliveryFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkUnstructuredDataDeliveryFilter* New();
  vtkTypeMacro(vtkUnstructuredDataDeliveryFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set this to true when this filter is used in a LOD pipeline. This ensures
  // that the respect the delivery flags correctly in ProcessViewRequest(). Some
  // keys are only to be handled when the filter is in a low-res pipeline.
  // Default is false.
  vtkSetMacro(LODMode, bool);
  vtkGetMacro(LODMode, bool);

  // Description:
  // Set whether the data is vtkPolyData or vtkUnstucturedGrid.
  // Note only VTK_POLY_DATA and VTK_UNSTRUCTURED_GRID are supported.
  void SetOutputDataType(int);
  vtkGetMacro(OutputDataType, int);

  // Description:
  // Process the view request.
  // Note that this may affect the mtime of the filter.
  void ProcessViewRequest(vtkInformation*);

  // Description:
  // Return this object's modified time.
  virtual unsigned long GetMTime();

  virtual void Modified();

//BTX
protected:
  vtkUnstructuredDataDeliveryFilter();
  ~vtkUnstructuredDataDeliveryFilter();

  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  int RequestDataObject(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Initializes internal filters
  void InitializeForCommunication();

  // overridden to mark input as optional.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkMPIMoveData* PassThroughMoveData;
  vtkMPIMoveData* ServerMoveData;
  vtkMPIMoveData* ClientMoveData;

  int OutputDataType;
  bool LODMode;
  bool UsePassThrough;

  // flag that keeps track of where the data has been "moved" since the last
  // Modified.
  int DataMoveState;

  enum
    {
    NO_WHERE = 0,
    PASS_THROUGH = 0x01,
    COLLECT_TO_ROOT = 0x02,
    };

private:
  vtkUnstructuredDataDeliveryFilter(const vtkUnstructuredDataDeliveryFilter&); // Not implemented
  void operator=(const vtkUnstructuredDataDeliveryFilter&); // Not implemented

  class VoidPtrSet;
  VoidPtrSet* HandledClients;
//ETX
};

#endif

