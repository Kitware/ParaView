/*=========================================================================

  Program:   ParaView
  Module:    vtkPistonRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonRepresentation - representation for showing vtkPistonDataObjects.
// .SECTION Description
// vtkPistonRepresentation is a representation for showing vtkPistonDataObjects.
// It uses vtkPistonMapper to draw the data while keeping it on the GPU.

#ifndef vtkPistonRepresentation_h
#define vtkPistonRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtkPistonMapper;
class vtkActor;

class VTK_EXPORT vtkPistonRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkPistonRepresentation* New();
  vtkTypeMacro(vtkPistonRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(
    vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val);

protected:
  vtkPistonRepresentation();
  ~vtkPistonRepresentation();

  // Description:
  // This method is called in the constructor. If the subclasses override any of
  // the iVar vtkObject's of this class e.g. the Mappers, GeometryFilter etc.,
  // they should call this method again in their constructor. It must be totally
  // safe to call this method repeatedly.
  virtual void SetupDefaults();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Subclasses should override this to connect inputs to the internal pipeline
  // as necessary. Since most representations are "meta-filters" (i.e. filters
  // containing other filters), you should create shallow copies of your input
  // before connecting to the internal pipeline. The convenience method
  // GetInternalOutputPort will create a cached shallow copy of a specified
  // input for you. The related helper functions GetInternalAnnotationOutputPort,
  // GetInternalSelectionOutputPort should be used to obtain a selection or
  // annotation port whose selections are localized for a particular input data object.
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Description:
  // Overridden to request correct ghost-level to avoid internal surfaces.
  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

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

  vtkPistonMapper* Mapper;
  vtkActor* Actor;

private:
  vtkPistonRepresentation(const vtkPistonRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonRepresentation&) VTK_DELETE_FUNCTION;

  char* DebugString;
  vtkSetStringMacro(DebugString);
};

#endif
