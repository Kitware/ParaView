/*=========================================================================

  Program:   ParaView
  Module:    vtkImageSliceDataDeliveryFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSliceDataDeliveryFilter
// .SECTION Description
// vtkImageSliceDataDeliveryFilter is a helper filter that can be used in
// representations rendering 2D image slices to deliver the data to the
// process(es) doing the rendering. Use this filter in
// vtkView::REQUEST_PREPARE_FOR_RENDER() pass to deliver the data to the correct
// node for rendering. Simply put this filter in your rendering-pipeline (for
// example after the geometry filter and before the mapper) and then pass the
// request-information object to ProcessViewRequest() call. This filter will
// lookup keys in the information object and update its state as needed to
// ensure that the data is available in the form/shape requested on the
// rendering nodes.
// .SECTION See Also
// vtkUnstructuredDataDeliveryFilter

#ifndef __vtkImageSliceDataDeliveryFilter_h
#define __vtkImageSliceDataDeliveryFilter_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkMPIMoveData;

class VTK_EXPORT vtkImageSliceDataDeliveryFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkImageSliceDataDeliveryFilter* New();
  vtkTypeMacro(vtkImageSliceDataDeliveryFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkImageSliceDataDeliveryFilter();
  ~vtkImageSliceDataDeliveryFilter();

  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  int RequestDataObject(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Initializes internal filters
  void InitializeForCommunication();

  // overridden to mark input as optional.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkMPIMoveData* MoveData;

private:
  vtkImageSliceDataDeliveryFilter(const vtkImageSliceDataDeliveryFilter&); // Not implemented
  void operator=(const vtkImageSliceDataDeliveryFilter&); // Not implemented
//ETX
};

#endif
