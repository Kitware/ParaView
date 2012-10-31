/*=========================================================================

  Program:   ParaView
  Module:    vtkQuadRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadRepresentation - a data-representation used by ParaView.
// .SECTION Description
// vtkQuadRepresentation extends vtkCompositeSliceRepresentation to add internal
// slices into internal view of the quad view.

#ifndef __vtkQuadRepresentation_h
#define __vtkQuadRepresentation_h

#include "vtkCompositeSliceRepresentation.h"
#include "vtkWeakPointer.h"

class vtkPVQuadRenderView;

class vtkQuadRepresentation : public vtkCompositeSliceRepresentation
{
public:
  static vtkQuadRepresentation* New();
  vtkTypeMacro(vtkQuadRepresentation, vtkCompositeSliceRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkQuadRepresentation();
  ~vtkQuadRepresentation();

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

  // Update slice axis label if any annotation available
  void UpdateDataEventCallBack(vtkObject* src, unsigned long event, void* data);

  // Update Show flag for CubeAxes and Outline
  void UpdateFromViewConfigurationCallBack(vtkObject* src, unsigned long event, void* data);

  vtkWeakPointer<vtkPVQuadRenderView> AssociatedView;
  vtkSetStringMacro(XLabel);
  vtkSetStringMacro(YLabel);
  vtkSetStringMacro(ZLabel);
  char *XLabel;
  char *YLabel;
  char *ZLabel;
  unsigned long ViewObserverId;
private:
  vtkQuadRepresentation(const vtkQuadRepresentation&); // Not implemented
  void operator=(const vtkQuadRepresentation&); // Not implemented
//ETX
};

#endif
