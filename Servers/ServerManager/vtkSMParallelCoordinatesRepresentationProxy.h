/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParallelCoordinatesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMParallelCoordinatesRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMParallelCoordinatesRepresentationProxy_h
#define __vtkSMParallelCoordinatesRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMXYChartViewProxy;
class vtkChartParallelCoordinates;
class vtkAnnotationLink;

class VTK_EXPORT vtkSMParallelCoordinatesRepresentationProxy :
    public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMParallelCoordinatesRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMParallelCoordinatesRepresentationProxy,
                       vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the underlying VTK representation.
//BTX
  vtkChartParallelCoordinates* GetChart();
//ETX

  // Description:
  // Called when a representation is added to a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Superclass::Update(); }
  virtual void Update(vtkSMViewProxy* view);

  // Description:
  // Set visibility of the representation.
  // Don't call directly. This method must be called through properties alone.
  void SetVisibility(int visible);

  // Description:
  // Get the number of series in this representation
  int GetNumberOfSeries();

  // Description:
  // Get the name of the series with the given index.  Returns 0 if the index
  // is out of range.  The returned pointer is only valid until the next call
  // to GetSeriesName.
  const char* GetSeriesName(int series);

//BTX
protected:
  vtkSMParallelCoordinatesRepresentationProxy();
  ~vtkSMParallelCoordinatesRepresentationProxy();

  virtual bool BeginCreateVTKObjects();
  virtual bool EndCreateVTKObjects();
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  vtkSMClientDeliveryRepresentationProxy* SelectionRepresentation;

  vtkWeakPointer<vtkSMXYChartViewProxy> ChartViewProxy;
  int Visibility;
  vtkAnnotationLink *AnnLink;

private:
  vtkSMParallelCoordinatesRepresentationProxy(
      const vtkSMParallelCoordinatesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMParallelCoordinatesRepresentationProxy&); // Not implemented
//ETX
};

#endif

