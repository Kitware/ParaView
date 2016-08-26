/*=========================================================================

  Program:   ParaView
  Module:    vtkRulerSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRulerSourceRepresentation
// .SECTION Description
// vtkRulerSourceRepresentation is a representation to show a ruler. The input
// is expected to be vtkPolyData with two points.

#ifndef vtkRulerSourceRepresentation_h
#define vtkRulerSourceRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkNew.h" //needed for instances of vtkPVCacheKeeper, vtkPolyData

class vtkDistanceRepresentation2D;
class vtkPolyData;
class vtkPVCacheKeeper;
class vtkTextProperty;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkRulerSourceRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkRulerSourceRepresentation* New();
  vtkTypeMacro(vtkRulerSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the text widget.
  void SetDistanceRepresentation(vtkDistanceRepresentation2D*);
  vtkGetObjectMacro(DistanceRepresentation, vtkDistanceRepresentation2D);

  // Description:
  // Set the text property for the printed distance.
  void SetTextProperty(vtkTextProperty* prop);

  // Description:
  virtual void MarkModified();

  // Description:
  // Set the visibility.
  virtual void SetVisibility(bool);

  // Description:
  // Specify the format to use for labelling the distance. Note that an empty
  // string results in no label, or a format string without a "%" character
  // will not print the distance value.
  void SetLabelFormat(char* labelFormat);

  // Description:
  // Enable or disable ruler mode. When enabled, the ticks on the distance
  // widget are separated by the amount specified by
  // vtkDistanceRepresentation::RulerDistance. Otherwise, the value
  // vtkDistanceRepresentation::NumberOfRulerTicks is used to draw the tick
  // marks.
  void SetRulerMode(int choice);

  // Description:
  // Specify the RulerDistance which indicates the spacing of the major ticks.
  // This ivar only has effect when the RulerMode is on.
  void SetRulerDistance(double distance);

  // Description:
  // Specify the number of major ruler ticks. Note: the number of ticks is the
  // number between the two handle endpoints. This ivar only has effect
  // when the RulerMode is off.
  void SetNumberOfRulerTicks(int numberOfRulerTicks);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  int ProcessViewRequest(
    vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

protected:
  vtkRulerSourceRepresentation();
  ~vtkRulerSourceRepresentation();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

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

  vtkDistanceRepresentation2D* DistanceRepresentation;
  vtkNew<vtkPVCacheKeeper> CacheKeeper;
  vtkNew<vtkPolyData> Clone;

private:
  vtkRulerSourceRepresentation(const vtkRulerSourceRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRulerSourceRepresentation&) VTK_DELETE_FUNCTION;

};

#endif
