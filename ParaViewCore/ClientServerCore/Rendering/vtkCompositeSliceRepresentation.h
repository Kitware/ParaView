/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeSliceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeSliceRepresentation - a data-representation used by ParaView.
// .SECTION Description
// vtkCompositeMultiSliceRepresentation is similar to
// vtkPVCompositeRepresentation but with a MultiSlice Representation as only
// choice underneath.

#ifndef __vtkCompositeSliceRepresentation_h
#define __vtkCompositeSliceRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVCompositeRepresentation.h"

class vtkOutlineRepresentation;
class vtkSliceFriendGeometryRepresentation;
class vtkThreeSliceFilter;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkCompositeSliceRepresentation : public vtkPVCompositeRepresentation
{
public:
  static vtkCompositeSliceRepresentation* New();
  vtkTypeMacro(vtkCompositeSliceRepresentation, vtkPVCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to simply pass the input to the internal representations. We
  // won't need this if vtkDataRepresentation correctly respected in the
  // arguments passed to it during ProcessRequest() etc.
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, int index);

  // Description:
  // Set the visibility for the outline.
  void SetOutlineVisibility(bool visible);

  // Description:
  // Propagate the modification to all internal representations.
  virtual void MarkModified();

  // Description:
  // Set visibility of the representation.
  // Overridden to update the cube-axes and selection visibilities.
  virtual void SetVisibility(bool visible);

  // Description:
  // Passed on to internal representations as well.
  virtual void SetUpdateTime(double time);
  virtual void SetUseCache(bool val);
  virtual void SetCacheKey(double val);
  virtual void SetForceUseCache(bool val);
  virtual void SetForcedCacheKey(double val);

  // Description:
  // Override that method in order to retrieve the Slice representation one
  // so we can internally bind it to our slice filter.
  virtual void AddRepresentation(const char* key, vtkPVDataRepresentation* repr);

  // Description:
  // Override to provide input array name regardless if any slice cut the actual data.
  virtual vtkDataObject* GetRenderedDataObject(int port);

  // Description:
  // Override because of internal composite representations that need to be
  // initilized as well.
  virtual unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable);

//BTX
protected:
  vtkCompositeSliceRepresentation();
  ~vtkCompositeSliceRepresentation();

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

  vtkOutlineRepresentation* OutlineRepresentation;
  vtkSliceFriendGeometryRepresentation* Slices[4];
  vtkThreeSliceFilter* InternalSliceFilter;

  void ModifiedInternalCallback(vtkObject* src, unsigned long event, void* data);
  void UpdateSliceConfigurationCallBack(vtkObject* src, unsigned long event, void* data);
  // Update Show flag for Outline
  void UpdateFromViewConfigurationCallBack(vtkObject* view, unsigned long event, void* data);

  unsigned long ViewObserverId;
  bool OutlineVisibility;

private:
  vtkCompositeSliceRepresentation(const vtkCompositeSliceRepresentation&); // Not implemented
  void operator=(const vtkCompositeSliceRepresentation&); // Not implemented
//ETX
};

#endif
