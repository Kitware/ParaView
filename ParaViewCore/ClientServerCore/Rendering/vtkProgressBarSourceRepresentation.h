/*=========================================================================

  Program:   ParaView
  Module:    vtkProgressBarSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProgressBarSourceRepresentation
// .SECTION Description
// vtkProgressBarSourceRepresentation is a representation to show ProgressBar. The input is
// expected to a vtkTable with a single row and column (atleast on the data
// server nodes). The content of this entry in the table is shown as ProgressBar on the
// rendering nodes.

#ifndef vtkProgressBarSourceRepresentation_h
#define vtkProgressBarSourceRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h" // for DummyPolyData

class vtk3DWidgetRepresentation;
class vtkPolyData;
class vtkPVCacheKeeper;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkProgressBarSourceRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkProgressBarSourceRepresentation* New();
  vtkTypeMacro(vtkProgressBarSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProgressBar widget.
  void SetProgressBarWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(ProgressBarWidgetRepresentation, vtk3DWidgetRepresentation);

  // Description:
  virtual void MarkModified();

  // Description:
  // Set the visibility.
  virtual void SetVisibility(bool);

  // Description:
  // Set the interactivity.
  void SetInteractivity(bool);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  int ProcessViewRequest(
    vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

//BTX
protected:
  vtkProgressBarSourceRepresentation();
  ~vtkProgressBarSourceRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Overridden to invoke vtkCommand::UpdateDataEvent.
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  vtkPVCacheKeeper* CacheKeeper;
  vtkSmartPointer<vtkPolyData> DummyPolyData;
  vtk3DWidgetRepresentation* ProgressBarWidgetRepresentation;

private:
  vtkProgressBarSourceRepresentation(const vtkProgressBarSourceRepresentation&); // Not implemented
  void operator=(const vtkProgressBarSourceRepresentation&); // Not implemented
//ETX
};

#endif
