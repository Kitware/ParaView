/*=========================================================================

  Program:   ParaView
  Module:    vtkSliceFriendGeometryRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliceFriendGeometryRepresentation
// .SECTION Description
// vtkSliceFriendGeometryRepresentation is a vtkGeometryRepresentationWithFaces
// which make our vtkCompositeSliceRepresentation a friend so it could access
// the protected method of it.

#ifndef __vtkSliceFriendGeometryRepresentation_h
#define __vtkSliceFriendGeometryRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkWeakPointer.h" // Needed

class vtkCubeAxesRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkSliceFriendGeometryRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkSliceFriendGeometryRepresentation* New();
  vtkTypeMacro(vtkSliceFriendGeometryRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(AllowInputConnectionSetting, bool);
  vtkGetMacro(AllowInputConnectionSetting, bool);
  vtkBooleanMacro(AllowInputConnectionSetting, bool);

  // Description:
  // Initialize mapper for slice selection
  void InitializeMapperForSliceSelection();

  // Description:
  // Overridden to be able to disable them when we don't want to change the input
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, int idx);

  // Description:
  // Override to provide input array name regardless if any slice cut the actual data.
  virtual vtkDataObject* GetRenderedDataObject(int port);

  // Description:
  // Create a dependancy link between this slice friendly representation
  // and the real parent representation that own the slice filter
  // so it could be skipped in order to be sure to provide the whole data array
  // names regardless if any data is actually cut or not.
  void SetRepresentationForRenderedDataObject(vtkPVDataRepresentation* rep);

  // Description:
  // Set visibility of the representation.
  // Overridden to update the cube-axes visibilities.
  virtual void SetVisibility(bool visible);

  // Description:
  // Propagate the modification to all internal representations.
  virtual void MarkModified();

  //vtkGetObjectMacro(CubeAxesRepresentation,vtkCubeAxesRepresentation);

  // Description:
  // Set the visibility for the cube-axis.
  void SetCubeAxesVisibility(bool visible);

  // Description:
  // Override because of internal composite representations that need to be
  // initilized as well.
  virtual unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable);

  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);

  vtkGetObjectMacro(CubeAxesRepresentation, vtkCubeAxesRepresentation);

//BTX
protected:
  vtkSliceFriendGeometryRepresentation();
  ~vtkSliceFriendGeometryRepresentation();

  friend class vtkCompositeSliceRepresentation;
  friend class vtkQuadRepresentation;

  bool AllowInputConnectionSetting;
  vtkWeakPointer<vtkPVDataRepresentation> RepresentationForRenderedDataObject;
  vtkCubeAxesRepresentation* CubeAxesRepresentation;
  bool CubeAxesVisibility;

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

private:
  vtkSliceFriendGeometryRepresentation(const vtkSliceFriendGeometryRepresentation&); // Not implemented
  void operator=(const vtkSliceFriendGeometryRepresentation&); // Not implemented
//ETX
};

#endif
