/*=========================================================================

  Program:   ParaView
  Module:    vtkTextSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextSourceRepresentation
// .SECTION Description
// vtkTextSourceRepresentation is a representation to show text. The input is
// expected to a vtkTable with a single row and column (atleast on the data
// server nodes). The content of this entry in the table is shown as text on the
// rendering nodes.

#ifndef __vtkTextSourceRepresentation_h
#define __vtkTextSourceRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtk3DWidgetRepresentation;
class vtkPolyData;
class vtkPVCacheKeeper;
class vtkUnstructuredDataDeliveryFilter;

class VTK_EXPORT vtkTextSourceRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkTextSourceRepresentation* New();
  vtkTypeMacro(vtkTextSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the text widget.
  void SetTextWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(TextWidgetRepresentation, vtk3DWidgetRepresentation);

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
  vtkTextSourceRepresentation();
  ~vtkTextSourceRepresentation();

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
  vtkUnstructuredDataDeliveryFilter* DataCollector;
  vtkPolyData* DummyPolyData;
  vtk3DWidgetRepresentation* TextWidgetRepresentation;

  vtkTimeStamp DeliveryTimeStamp;

private:
  vtkTextSourceRepresentation(const vtkTextSourceRepresentation&); // Not implemented
  void operator=(const vtkTextSourceRepresentation&); // Not implemented
//ETX
};

#endif
